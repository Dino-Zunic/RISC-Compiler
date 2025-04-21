#pragma once

#include "Parser.hpp"
#include <stack>
#include <unordered_map>

class Linker {
public:
    Linker(const std::string &rootFilename);
    bool link();
    const std::vector<Command> &getCommands() const;
    uint32_t getStartAddress() const;
    const std::vector<Error> &getErrors() const;
    bool hasErrors() const;
private:
    std::string rootFilename;
    std::vector<Command> commands;
    std::vector<Error> errors;
    std::unordered_set<std::string> includedFiles;
    std::unordered_map<std::string, uint32_t> symbolTable;
    std::stack<std::string> includeStack;
    bool startFound = false;
    uint32_t startAddress;

    bool resolveSymbolsAndStartAddress();
    bool resolveIncludes(const std::string &filename, std::vector<Command> &collectedCommands);
    bool hasCircularInclude(const std::string &filename);
};
