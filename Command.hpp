#pragma once

#include "Token.hpp"
#include <unordered_set>
#include "LanguageInfo.hpp"

class Command {
public:
    enum class Type {
        Instruction,
        Directive,
        SymbolDefinition,
        Label
    };
    enum class Sign {
        Plus,
        Minus
    };
    static Command createInstruction0(const std::string &name);
    static Command createInstruction(const std::string &name, LanguageInfo::AddressMode addressingMode, uint32_t number, const std::string &symbol, Sign sign, int r1, int r2 = 0);
    static Command createInstruction3(const std::string &name, int r1, int r2, int r3);
    static Command createDirective(const std::string &name, uint32_t number, const std::string& symbol);
    static Command createDUP(uint32_t number, const std::string& symbol, uint32_t dupNumber);
    static Command createSymbolDefinition(const std::string &name, uint32_t number);
    static Command createLabel(const std::string &name);
    Type getType() const;
    const std::string &getName() const;
    LanguageInfo::AddressMode getAddressMode() const;
    const std::string& getSymbol() const;
    uint32_t getNumber() const;
    uint32_t getDupNumber() const;
    int getR1() const;
    int getR2() const;
    int getR3() const;
    std::string toString() const;
    uint32_t getMemorySizeWords() const;
    void setNumber(uint32_t value);

    uint32_t address = 0;
private:
    Type type;
    LanguageInfo::AddressMode addressingMode = LanguageInfo::AddressMode::RegisterDirect;
    std::string name;
    uint32_t number = 0;
    std::string symbol;
    uint32_t dupNumber = 0; // only used for dup
    Sign sign = Sign::Plus; // used for register indirect with displacement
    int r1 = 0;
    int r2 = 0;
    int r3 = 0;

    Command(Type type, std::string name);

    Command(Type type, const std::string &name, LanguageInfo::AddressMode addressingMode, uint32_t number, 
        const std::string &symbol, Sign sign, int r1 = 0, int r2 = 0, int r3 = 0);
};