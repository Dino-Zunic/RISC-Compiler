#include "Command.hpp"
#include <sstream>
#include <string>
#include <unordered_map>
#include <iomanip>

Command Command::createInstruction0(const std::string &name) {
    return Command(Type::Instruction, name);
}

Command Command::createInstruction(const std::string &name, LanguageInfo::AddressMode addressingMode, 
    uint32_t number, const std::string& symbol, Sign sign, int r1, int r2) {
    return Command(Type::Instruction, name, addressingMode, number, symbol, sign, r1, r2);
}

Command Command::createInstruction3(const std::string &name, int r1, int r2, int r3) {
    return Command(Type::Instruction, name, LanguageInfo::AddressMode::RegisterDirect, 0, "", Sign::Plus, r1, r2, r3);
}

Command Command::createDirective(const std::string &name, uint32_t number, const std::string& symbol) {
    Command command(Type::Directive, name);
    command.number = number;
    command.symbol = symbol;
    return command;
}

Command Command::createDUP(uint32_t number, const std::string& symbol, uint32_t dupNumber) {
    Command command(Type::Directive, "dup");
    command.number = number;
    command.symbol = symbol;
    command.dupNumber = dupNumber;
    return command;
}

Command Command::createSymbolDefinition(const std::string &name, uint32_t number) {
    Command command(Type::SymbolDefinition, name);
    command.number = number;
    return command;
}

Command Command::createLabel(const std::string &name) {
    return Command(Type::Label, name);
}

Command::Type Command::getType() const { 
    return type; 
}

const std::string &Command::getName() const {
    return name; 
}

LanguageInfo::AddressMode Command::getAddressMode() const { 
    return addressingMode; 
}

const std::string &Command::getSymbol() const { 
    return symbol; 
}

uint32_t Command::getNumber() const {
    return number;
}

uint32_t Command::getDupNumber() const {
    return dupNumber;
}

int Command::getR1() const { 
    return r1; 
}

int Command::getR2() const { 
    return r2; 
}

int Command::getR3() const { 
    return r3; 
}

Command::Command(Type type, std::string name) : type(type), name(name) {}

Command::Command(Type type, const std::string &name, LanguageInfo::AddressMode addressingMode, 
    uint32_t number, const std::string &symbol, Sign sign, int r1, int r2, int r3)
    : type(type), name(name), addressingMode(addressingMode), 
    number(number), symbol(symbol), sign(sign), r1(r1), r2(r2), r3(r3) {}

static std::string toString(LanguageInfo::AddressMode mode) {
    switch (mode) {
    case LanguageInfo::AddressMode::Immediate: return "Immediate";
    case LanguageInfo::AddressMode::RegisterDirect: return "RegisterDirect";
    case LanguageInfo::AddressMode::MemoryDirect: return "MemoryDirect";
    case LanguageInfo::AddressMode::RegisterIndirect: return "RegisterIndirect";
    case LanguageInfo::AddressMode::RegisterIndirectWithDisplacement: return "RegisterIndirectWithDisplacement";
    default: return "Unknown";
    }
}

static std::string toString(Command::Type type) {
    switch (type) {
    case Command::Type::Instruction: return "Instruction";
    case Command::Type::Directive: return "Directive";
    case Command::Type::SymbolDefinition: return "SymbolDefinition";
    case Command::Type::Label: return "Label";
    default: return "Unknown";
    }
}

static std::string toString(Command::Sign sign) {
    return (sign == Command::Sign::Plus) ? "+" : "-";
}

std::string Command::toString() const {
    std::ostringstream oss;

    oss << "Addresss: " << address << "\n";
    oss << "Size: " << getMemorySizeWords() << "\n";
    oss << "Command: " << ::toString(type) << ", Name: " << name << "\n";

    if (type == Type::Instruction) {
        oss << "Addressing Mode: " << ::toString(addressingMode) << "\n";
    }
    oss << "Number: " << number << "\n";
    if (!symbol.empty()) {
        oss << "Symbol: " << symbol << "\n";
    }

    if (dupNumber) {
        oss << "Number2: " << dupNumber << "\n";
    }

    oss << "Register 1 (r1): " << r1 << "\n";
    oss << "Register 2 (r2): " << r2 << "\n";
    oss << "Register 3 (r3): " << r3 << "\n";

    if (addressingMode == LanguageInfo::AddressMode::RegisterIndirectWithDisplacement) {
        oss << "Displacement Sign: " << ::toString(sign) << "\n";
    }

    return oss.str();
}

uint32_t Command::getMemorySizeWords() const {
    if (type == Type::SymbolDefinition || type == Type::Label) {
        return 0;
    }
    if (type == Type::Directive) {
        if (name == "dup") {
            return dupNumber;
        }
        return name == "dd";
    }
    return
        addressingMode == LanguageInfo::AddressMode::RegisterDirect ||
        addressingMode == LanguageInfo::AddressMode::RegisterIndirect ? 1 : 2;
}

void Command::setNumber(uint32_t number) {
    this->number = number;
    this->symbol = "";
}
