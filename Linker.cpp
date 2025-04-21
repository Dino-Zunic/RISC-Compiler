#pragma once
#include "Linker.hpp"
#include <unordered_set>
#include <vector>
#include <filesystem>

Linker::Linker(const std::string &rootFilename) : rootFilename(rootFilename) {}

bool Linker::link() {
    return 
        resolveIncludes(rootFilename, commands) && 
        resolveSymbolsAndStartAddress() &&
        errors.empty();
}

bool Linker::resolveIncludes(const std::string &filename, std::vector<Command> &collectedCommands) {
    if (hasCircularInclude(filename)) {
        errors.push_back(Error("Circular include detected: " + filename));
        return false;
    }
    if (includedFiles.count(filename) > 0) {
        return true;
    }
    includeStack.push(filename);

    if (!std::filesystem::exists(filename)) {
        errors.push_back(Error("File doesn't exist: " + filename));
        includeStack.pop();
        return false;
    }

    Parser parser(filename);
    if (!parser.parse()) {
        errors.insert(errors.end(), parser.getErrors().begin(), parser.getErrors().end());
        return false;
    }

    includedFiles.insert(filename);
    std::vector<Command> currentFileCommands = parser.getCommands();

    for (const Command &command : currentFileCommands) {
        if (command.getType() == Command::Type::Directive && command.getName() == "include") {
            std::string includeFilename = command.getSymbol();

            if (includeFilename.find(".asm") == std::string::npos) {
                includeFilename += ".asm";
            }

            std::filesystem::path rootPath = std::filesystem::path(rootFilename).parent_path();
            std::filesystem::path fullIncludePath = rootPath / includeFilename;

            std::vector<Command> includedFileCommands;
            if (!resolveIncludes(fullIncludePath.string(), includedFileCommands)) {
                return false;
            }

            collectedCommands.insert(collectedCommands.end(), includedFileCommands.begin(), includedFileCommands.end());
        }
        else {
            collectedCommands.push_back(command);
        }
    }


    includeStack.pop();
    return true;
}

bool Linker::hasCircularInclude(const std::string &filename) {
    std::stack<std::string> temp = includeStack;
    while (!temp.empty()) {
        if (temp.top() == filename) {
            return true;
        }
        temp.pop();
    }
    return false;
}

bool Linker::resolveSymbolsAndStartAddress() {
    // First pass: collect all symbol definitions and labels
    uint32_t memoryIndex = 0;
    for (size_t i = 0; i < commands.size(); i++) {
        Command& command = commands[i];
        if (command.getType() == Command::Type::SymbolDefinition) {
            const std::string &symbol = command.getName();
            if (symbolTable.count(symbol) > 0) {
                errors.push_back(Error("Symbol redefinition: " + symbol, i));
                return false;
            }
            symbolTable[symbol] = command.getNumber();
        }
        else if (command.getType() == Command::Type::Label) {
            const std::string &symbol = command.getName();
            if (symbolTable.count(symbol) > 0) {
                errors.push_back(Error("Symbol redefinition: " + symbol, i));
                return false;
            }
            symbolTable[symbol] = memoryIndex;
        }
        command.address = memoryIndex;
        memoryIndex += command.getMemorySizeWords();
        if (command.getName() == "org") {
            memoryIndex = command.getNumber();
        }
    }
    std::vector<Command> previousCommands = commands;
    commands.clear();
    // Second pass: resolve all symbol references
    for (size_t i = 0; i < previousCommands.size(); i++) {
        Command& command = previousCommands[i];
        if (command.getType() == Command::Type::Instruction || command.getType() == Command::Type::Directive) {
            if (!command.getSymbol().empty()) {
                const std::string &symbol = command.getSymbol();
                if (symbolTable.count(symbol) == 0) {
                    errors.push_back(Error("Undefined symbol: " + symbol));
                    return false;
                }
                command.setNumber(symbolTable.at(symbol));
            }
            if (command.getName() == "start") {
                if (startFound) {
                    errors.push_back(Error("Multiple start directives found."));
                    return false;
                }
                startFound = true;
                startAddress = command.getNumber();
            }
            if (command.getMemorySizeWords() != 0) {
                commands.push_back(command);
            }
        }
    }
    if (!startFound) {
        errors.push_back(Error("No start directive found."));
        return false;
    }

    return true;
}

const std::vector<Command> &Linker::getCommands() const {
    return commands;
}

uint32_t Linker::getStartAddress() const {
    return startAddress;
}

const std::vector<Error> &Linker::getErrors() const {
    return errors;
}

bool Linker::hasErrors() const {
    return !errors.empty();
}
