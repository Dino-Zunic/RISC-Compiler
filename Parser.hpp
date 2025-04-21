#pragma once

#include "Command.hpp"
#include <unordered_map>
class Parser {
public:
    Parser(const std::string &filename);
    bool parse();
    const std::vector<Command> &getCommands() const;
    const std::vector<Error> &getErrors() const;
    bool hasErrors() const;
private:
    std::string filename;
    std::vector<Command> commands;
    std::vector<Error> errors;
    std::vector<Token> tokens;
    int currentTokenIndex;

    bool parseCommand();
    bool parseCommandUnaligned();
    bool parseInstruction();
    bool parseInstruction0(const std::string &name);
    bool parseInstruction1(const std::string &name);
    bool parseInstruction2(const std::string &name);
    bool parseInstruction3(const std::string &name);
    bool parseDirective();
    bool parseDup();
    bool parseDD();
    bool parseSymbolDefinition();
    bool parseLabel();
    bool parseNewline();

    void addError(const std::string &message);

    bool isRegister(int offset = 0) const;
    bool isSymbol(int offset = 0) const;
    bool isNumber(int offset = 0) const;
    bool isNumberOrSymbol(int offset = 0) const;
    bool isImmediate(int offset = 0) const;
    bool isRegisterIndirect(int offset = 0) const;
    bool isRegisterIndirectWithDisplacement(int offset = 0) const;
    bool isOperand(int offset = 0) const;

    void consumeNumber();
    void consumeSymbol();
    void consumeRegister();
    void consumeNumberOrSymbol();
    void consumeImmediate();
    void consumeRegisterIndirect();
    void consumeRegisterIndirectWithDisplacement();
    void consumeOperand();

    void getOperatorInfo(LanguageInfo::AddressMode &addressMode, uint32_t &number, std::string &symbol, Command::Sign &sign, int &r);
    void getNumberOrSymbolInfo(uint32_t& number, std::string& symbol, int offset = 0);

    Token getToken(int offset) const;
    Token currentToken() const;
    Token nextToken();
};
