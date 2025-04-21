#include "Compiler.hpp"
#include <cstdint>
#include <cstring>

Compiler::Compiler(const std::string& rootFilename) : rootFilename(rootFilename) {}

bool Compiler::compile() {
    Linker linker(rootFilename);
    if (!linker.link()) {
        errors = linker.getErrors();
        return false;
    }

    const auto& commands = linker.getCommands();
    if (!processCommands(commands)) {
        return false;
    }

    std::string mifFilename = replaceFileExtension(rootFilename, ".mif");
    return generateMIF(mifFilename);
}

const std::vector<Error>& Compiler::getErrors() const {
    return errors;
}

bool Compiler::hasErrors() const {
    return !errors.empty();
}

const std::vector<uint32_t>& Compiler::getMachineCode() const {
    return machineCode;
}

bool Compiler::generateMachineCode(const Command& command, std::vector<uint32_t>& output) {
    uint32_t opcode = LanguageInfo::OPCODE_INVALID;
    uint32_t addressMode = static_cast<uint32_t>(command.getAddressMode());
    uint32_t address = command.address;
    uint32_t r1 = command.getR1();
    uint32_t r2 = command.getR2();
    uint32_t r3 = command.getR3();
    uint32_t word = 0;

    if (address == 0x00000100) {
        opcode = 6;
    }

    switch (command.getType()) {
    case Command::Type::Instruction: {
        opcode = LanguageInfo::getOpcode(command.getName());
        if (opcode == LanguageInfo::OPCODE_INVALID) {
            errors.emplace_back("Invalid opcode for instruction: " + command.getName());
            return false;
        }

        switch (command.getAddressMode()) {
        case LanguageInfo::AddressMode::RegisterDirect:
        case LanguageInfo::AddressMode::RegisterIndirect:
            // Format: Opcode | AddrMode | Reg1 | Reg2 | Reg3 | Rest Unused
        {
            word |= opcode << 24;
            word |= addressMode << 21;
            word |= r1 << 16;
            word |= r2 << 11;
            word |= r3 << 6;
            output[address] = word;
        }
        break;

        case LanguageInfo::AddressMode::Immediate:
        case LanguageInfo::AddressMode::MemoryDirect:
        case LanguageInfo::AddressMode::RegisterIndirectWithDisplacement:
            // Format: Opcode | AddrMode | Reg1 | Reg2 | Rest Unused
        {
            word |= opcode << 24;
            word |= addressMode << 21;
            word |= r1 << 16;
            word |= r2 << 11;
            output[address] = word;
            // Add 32-bit constant/address/displacement
            output[address + 1] = command.getNumber();
        }
        break;

        default:
            errors.emplace_back("Unsupported addressing mode for instruction: " + command.getName());
            return false;
        }
        break;
    }

    case Command::Type::Directive:
    {
        if (command.getName() == "dd") {
            output[address] = command.getNumber();
        }
        else if (command.getName() == "dup") {
            for (uint32_t i = 0; i < command.getDupNumber(); ++i) {
                output[address + i] = command.getNumber();
            }
        }
        else {
            errors.emplace_back("Unsupported directive: " + command.getName());
            return false;
        }
    }
    break;

    case Command::Type::SymbolDefinition:
    case Command::Type::Label:
        // Ignored in compile phase
        break;

    default:
        errors.emplace_back("Unsupported command type: " + command.getName());
        return false;
    }

    return true;
}

bool Compiler::processCommands(const std::vector<Command>& commands) {
    uint32_t maximumAddress = 0;
    for (const auto& command : commands) {
        if (command.address > maximumAddress) {
            maximumAddress = command.address;
        }
    }
    machineCode.resize(maximumAddress + 1, 0);
    for (const auto& command : commands) {
        if (!generateMachineCode(command, machineCode)) {
            return false;
        }
    }
    return true;
}

#include <fstream>
#include <iomanip>

bool Compiler::generateMIF(const std::string& outputFilename) {
    if (machineCode.empty()) {
        errors.emplace_back("No machine code to generate MIF.");
        return false;
    }

    uint32_t depth = static_cast<uint32_t>(machineCode.size());

    std::ofstream mifFile(outputFilename);
    if (!mifFile.is_open()) {
        errors.emplace_back("Unable to open MIF file for writing.");
        return false;
    }

    mifFile << "WIDTH=32;\n";
    mifFile << "DEPTH=" << depth << ";\n";
    mifFile << "ADDRESS_RADIX=HEX;\n";
    mifFile << "DATA_RADIX=HEX;\n";
    mifFile << "CONTENT BEGIN\n";

    for (uint32_t address = 0; address < depth; ++address) {
        mifFile << std::setw(4) << std::setfill('0') << std::hex << address << " : ";
        mifFile << std::setw(8) << std::setfill('0') << std::hex << machineCode[address] << ";\n";
    }

    mifFile << "END;\n";
    mifFile.close();

    return true;
}

std::string Compiler::replaceFileExtension(const std::string& filename, const std::string& newExtension) {
    std::string::size_type pos = filename.find_last_of('.');
    if (pos == std::string::npos) {
        return filename + newExtension;
    }
    return filename.substr(0, pos) + newExtension;
}
