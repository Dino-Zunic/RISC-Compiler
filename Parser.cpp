#include "Parser.hpp"
#include "Token.hpp"
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include "LanguageInfo.hpp"

static uint32_t toIntFromHex(const std::string &s) {
    return static_cast<uint32_t>(std::stoul(s, nullptr, 16));
}

Parser::Parser(const std::string &filename) : filename(filename), currentTokenIndex(0) {
    Tokenizer tokenizer(filename);
    if (!tokenizer.tokenize()) {
        errors = tokenizer.getErrors();
    }
    else {
        tokens = tokenizer.getTokens();
    }
}

bool Parser::parse() {
    while (currentTokenIndex < tokens.size()) {
        parseCommand();
    }
    return errors.empty();
}

const std::vector<Command> &Parser::getCommands() const {
    return commands;
}

const std::vector<Error> &Parser::getErrors() const {
    return errors;
}

bool Parser::hasErrors() const {
    return !errors.empty();
}

bool Parser::parseCommand() {
    if (parseCommandUnaligned()) {
        return true;
    }
    while (!currentToken().value.empty() && currentToken().value != "\n") {
        nextToken();
    }
    nextToken(); // skip "\n"
    return false;
}

bool Parser::parseCommandUnaligned() {
    Token token = currentToken();
    if (token.value == "\n") {
        nextToken();
        return true; // skip empty line
    }
    if (token.type != Token::Type::Name) {
        addError("Command can't start with " + token.value);
        return false;
    }
    if (LanguageInfo::keywords.count(token.value)) {
        return parseDirective();
    }
    if (getToken(1).value == "def") {
        return parseSymbolDefinition();
    }
    if (getToken(1).type == Token::Type::Symbol && getToken(1).value == ":") {
        return parseLabel();
    }
    return parseInstruction();
}

bool Parser::parseInstruction() {
    Token token = currentToken();

    std::string name = token.value;
    nextToken(); // skip the name

    if (LanguageInfo::isInstructionInCategory(name, LanguageInfo::Category::Jump0)) {
        return parseInstruction0(name);
    }
    else if (
        LanguageInfo::isInstructionInCategory(name, LanguageInfo::Category::Jump1) || 
        LanguageInfo::isInstructionInCategory(name, LanguageInfo::Category::Arithmetic1)
        ) {
        return parseInstruction1(name);
    }
    else if (
        LanguageInfo::isInstructionInCategory(name, LanguageInfo::Category::Jump2) ||
        LanguageInfo::isInstructionInCategory(name, LanguageInfo::Category::Arithmetic2) ||
        LanguageInfo::isInstructionInCategory(name, LanguageInfo::Category::LoadStore)
        ) {
        return parseInstruction2(name);
    }
    else if (LanguageInfo::isInstructionInCategory(name, LanguageInfo::Category::Arithmetic3)) {
        return parseInstruction3(name);
    }
    else {
        addError("Unknown instruction: " + name);
        return false;
    }
    return parseNewline();
}

bool Parser::parseInstruction0(const std::string &name) {
    commands.push_back(Command::createInstruction0(name));
    return true;
}

bool Parser::parseInstruction1(const std::string &name) {
    if (!isOperand()) {
        addError("Expected an operand for " + name);
        return false;
    }
    if (LanguageInfo::isInstructionInCategory(name, LanguageInfo::Category::Arithmetic1) && !isRegister()) {
        addError("Invalid operand for " + name + ": " + currentToken().value + ", expected register");
        return false;
    }
    LanguageInfo::AddressMode mode;
    if (isRegister()) {

    }
    LanguageInfo::AddressMode addressMode;
    uint32_t number;
    std::string symbol;
    Command::Sign sign;
    int r;
    getOperatorInfo(addressMode, number, symbol, sign, r);

    Command command = Command::createInstruction(name, addressMode, number, symbol, sign, r);
    commands.push_back(command);
    consumeOperand();
    return true;
}

bool Parser::parseInstruction2(const std::string &name) {
    LanguageInfo::AddressMode addressMode;
    uint32_t number;
    std::string symbol;
    Command::Sign sign;
    int r1, r2;

    if (!isOperand()) {
        addError("Expected an operand for " + name);
        return false;
    }
    if (!isRegister()) {
        addError("First operand for " + name + " must be a register");
        return false;
    }
    r1 = LanguageInfo::getRegisterIndex(currentToken().value);
    consumeOperand();
    if (!currentToken().matchToken(Token::Type::Symbol, ",")) {
        addError("Expected ',' after first operand for " + name);
        return false;
    }
    nextToken(); // skip comma
    if (!isOperand()) {
        addError("Expected an operand for " + name);
        return false;
    }
    if (name == "store" && currentToken().value == "#") {
        addError("Can't store to a constant");
        return false;
    }
    getOperatorInfo(addressMode, number, symbol, sign, r2);
    commands.push_back(Command::createInstruction(name, addressMode, number, symbol, sign, r1, r2));
    consumeOperand();
    return true;
}

bool Parser::parseInstruction3(const std::string &name) {
    int r[3];
    for (int i = 0; i < 3; ++i) {
        if (!isRegister()) {
            addError("Invalid operand for " + name + ": " + currentToken().value + ", expected register");
            return false;
        }
        r[i] = LanguageInfo::getRegisterIndex(currentToken().value);
        consumeRegister();

        if (i < 2 && !currentToken().matchToken(Token::Type::Symbol, ",")) {
            addError("Expected ',' between operands for " + name);
            return false;
        }
        nextToken(); // skip comma
    }
    commands.push_back(Command::createInstruction3(name, r[0], r[1], r[2]));
    return true;
}

bool Parser::parseDup() {
    uint32_t number;
    std::string symbol;
    uint32_t dupNumber;
    nextToken(); // skip (
    if (!isNumberOrSymbol()) {
        addError("Expected number or symbol");
        return false;
    }
    getNumberOrSymbolInfo(number, symbol);
    consumeNumberOrSymbol();
    if (currentToken().value != "dup") {
        addError("Invalid format for 'dd', expected 'dup'");
        return false;
    }
    nextToken(); // skip dup
    if (!isNumber()) {
        addError("Expected number");
        return false;
    }
    dupNumber = static_cast<uint32_t>(std::stoul(currentToken().value, nullptr, 16));
    consumeNumber();
    if (currentToken().value != ")") {
        addError("Expected ')'");
        return false;
    }
    nextToken(); // )
    commands.push_back(Command::createDUP(number, symbol, dupNumber));
    return true;
}

bool Parser::parseDD() {
    if (currentToken().value == "(") {
        return parseDup();
    }
    if (!isNumberOrSymbol()) {
        addError("Expected number or symbol");
        return false;
    }
    uint32_t number;
    std::string symbol;
    getNumberOrSymbolInfo(number, symbol);
    commands.push_back(Command::createDirective("dd", number, symbol));
    consumeNumberOrSymbol();
    while (true) {
        if (currentToken().value != ",") {
            break;
        }
        nextToken();  // skip commas
        if (!isNumberOrSymbol()) {
            addError("Expected number or symbol");
            return false;
        }
        getNumberOrSymbolInfo(number, symbol);
        commands.push_back(Command::createDirective("dd", number, symbol));
        consumeNumberOrSymbol();
    }
    return parseNewline();
}

bool Parser::parseDirective() {
    Token token = currentToken();
    std::string directiveName = token.value;
    nextToken();

    if (directiveName == "include") {
        if (!isSymbol()) {
            addError("Expected filename after 'include' directive");
            return false;
        }
        commands.push_back(Command::createDirective(directiveName, 0, currentToken().value));
        nextToken(); // skip filename
        return parseNewline();
    }
    else if (directiveName == "start") {
        if (!isNumberOrSymbol()) {
            addError("Expected number or symbol for '" + directiveName + "'");
            return false;
        }
        uint32_t number;
        std::string symbol;
        getNumberOrSymbolInfo(number, symbol);
        commands.push_back(Command::createDirective(directiveName, number, symbol));
        consumeNumberOrSymbol();
        return parseNewline();
    }
    else if (directiveName == "org") {
        if (!isNumber()) {
            addError("Expected number for '" + directiveName + "'");
            return false;
        }
        commands.push_back(Command::createDirective(directiveName, toIntFromHex(currentToken().value), ""));
        consumeNumber();
        return parseNewline();
    }
    else if (directiveName == "dd") {
        return parseDD();
    }

    addError("Unknown directive: " + directiveName);
    return false;
}


bool Parser::parseSymbolDefinition() {
    std::string symbol = currentToken().value;
    nextToken();  // Skip symbol
    nextToken();  // Skip 'def'
    if (!isNumber()) {
        addError("Expected number after 'def'");
        return false;
    }
    std::string value = currentToken().value;
    consumeNumber();
    commands.push_back(Command::createSymbolDefinition(symbol, toIntFromHex(value)));
    return parseNewline();
}

bool Parser::parseNewline() {
    if (!currentToken().value.empty() && currentToken().value != "\n") {
        addError("Only one command per line");
        return false;
    }
    nextToken(); // skip "\n"
    return true;
}

bool Parser::parseLabel() {
    Token labelToken = currentToken();
    nextToken();  // Skip label
    nextToken();  // Skip ':'
    commands.push_back(Command::createLabel(labelToken.value));
    return true;
}

void Parser::addError(const std::string& message) {
    int errorLine = getToken(0).value.empty() ? getToken(-1).line : getToken(0).line;
    std::string errorLineTokens;

    for (const Token& token : tokens) {
        if (token.line == errorLine) {
            errorLineTokens += token.value + " ";
        }
        else if (token.line > errorLine) {
            break;
        }
    }

    if (!errorLineTokens.empty() && errorLineTokens.back() == ' ') {
        errorLineTokens.pop_back();
    }

    std::string errorMarker(message.size(), '~');
    std::string fullMessage = message + "\n" + errorLineTokens;

    if (!errorLineTokens.empty()) {
        fullMessage += "\n" + errorMarker;
    }

    errors.emplace_back(fullMessage, errorLine, filename);
}


Token Parser::getToken(int offset) const {
    int index = currentTokenIndex + offset;
    if (index < tokens.size()) {
        return tokens[index];
    }
    return { Token::Type::Unknown, "", 0 };
}

Token Parser::currentToken() const {
    return getToken(0);
}

Token Parser::nextToken() {
    currentTokenIndex++;
    return currentToken();
}

bool Parser::isRegister(int offset) const {
    return LanguageInfo::getRegisterIndex(getToken(offset).value) != LanguageInfo::REGISTER_INVALID;
}

bool Parser::isSymbol(int offset) const {
    return 
        getToken(offset).matchToken(Token::Type::Name) && 
        LanguageInfo::keywords.find(getToken(offset).value) == LanguageInfo::keywords.end() &&
        !isRegister(offset);
}

bool Parser::isNumber(int offset) const {
    return getToken(offset).matchToken(Token::Type::Number);
}

bool Parser::isNumberOrSymbol(int offset) const {
    return isSymbol(offset) || isNumber(offset);
}

bool Parser::isImmediate(int offset) const {
    return getToken(offset).value == "#" && isNumberOrSymbol(offset + 1);
}

bool Parser::isRegisterIndirect(int offset) const {
    return
        getToken(offset).matchToken(Token::Type::Symbol, "[") &&
        isRegister(offset + 1) &&
        getToken(offset + 2).matchToken(Token::Type::Symbol, "]");
}

bool Parser::isRegisterIndirectWithDisplacement(int offset) const {
    int currentDisplacement = offset;
    if (!getToken(currentDisplacement).matchToken(Token::Type::Symbol, "[")) {
        return false;
    }

    currentDisplacement++;

    if (!isRegister(currentDisplacement) && !isNumberOrSymbol(currentDisplacement)) {
        return false;
    }
    bool operand1IsRegister = isRegister(currentDisplacement);
    currentDisplacement++;

    std::string operand = getToken(currentDisplacement).value;
    if (operand != "+" && operand != "-") {
        return false;
    }
    currentDisplacement++;

    if (!isRegister(currentDisplacement) && !isNumberOrSymbol(currentDisplacement)) {
        return false;
    }
    bool operand2IsRegister = isRegister(currentDisplacement);
    currentDisplacement++;

    if (!getToken(currentDisplacement).matchToken(Token::Type::Symbol, "]")) {
        return false;
    }

    // Check for illegal cases:
    if (operand1IsRegister && operand2IsRegister) {
        return false; // both are registers
    }
    if (!operand1IsRegister && !operand2IsRegister) {
        return false; // both are displacements
    }
    if (operand == "-" && operand2IsRegister) {
        return false; // register is being subtracted
    }

    return true;
}

bool Parser::isOperand(int offset) const {
    return isRegister(offset) || isNumberOrSymbol(offset) || isRegisterIndirect(offset) || isRegisterIndirectWithDisplacement(offset) || isImmediate(offset);
}

void Parser::consumeNumber() {
    currentTokenIndex++;
}

void Parser::consumeSymbol() {
    currentTokenIndex++;
}

void Parser::consumeRegister() {
    currentTokenIndex++;
}

void Parser::consumeNumberOrSymbol() {
    if (isNumber()) {
        consumeNumber();
    }
    else {
        consumeSymbol();
    }
}

void Parser::consumeImmediate() {
    nextToken();
    consumeNumberOrSymbol();
}

void Parser::consumeRegisterIndirect() {
    currentTokenIndex += 3;
}

void Parser::consumeRegisterIndirectWithDisplacement() {
    currentTokenIndex += 5;
}

void Parser::consumeOperand() {
    if (isRegister()) {
        consumeRegister();
    }
    else if (isNumberOrSymbol()) {
        consumeNumberOrSymbol();
    }
    else if (isRegisterIndirect()) {
        consumeRegisterIndirect();
    }
    else if (isRegisterIndirectWithDisplacement()) {
        consumeRegisterIndirectWithDisplacement();
    }
    else if (isImmediate()) {
        consumeImmediate();
    }
}

void Parser::getOperatorInfo(LanguageInfo::AddressMode &addressMode, uint32_t &number, std::string &symbol, Command::Sign &sign, int &r) {
    sign = Command::Sign::Plus;
    number = 0;
    symbol = "";
    r = 0;
    if (isRegister()) {
        addressMode = LanguageInfo::AddressMode::RegisterDirect;
        r = LanguageInfo::getRegisterIndex(getToken(0).value);
    }
    else if (isNumberOrSymbol()) {
        addressMode = LanguageInfo::AddressMode::MemoryDirect;
        getNumberOrSymbolInfo(number, symbol);
    }
    else if (isImmediate()) {
        addressMode = LanguageInfo::AddressMode::MemoryDirect;
        getNumberOrSymbolInfo(number, symbol, 1);
    }
    else if (isRegisterIndirect()) {
        addressMode = LanguageInfo::AddressMode::RegisterIndirect;
        r = LanguageInfo::getRegisterIndex(getToken(1).value);
    }
    else if (isRegisterIndirectWithDisplacement()) {
        addressMode = LanguageInfo::AddressMode::RegisterIndirectWithDisplacement;
        if (isRegister(1)) {
            r = LanguageInfo::getRegisterIndex(getToken(1).value);
            getNumberOrSymbolInfo(number, symbol, 3);
            sign = getToken(2).value == "+" ? Command::Sign::Plus : Command::Sign::Minus;
        }
        else {
            r = LanguageInfo::getRegisterIndex(getToken(3).value);
            getNumberOrSymbolInfo(number, symbol, 1);
        }
    }
}

void Parser::getNumberOrSymbolInfo(uint32_t& number, std::string& symbol, int offset) {
    number = 0;
    symbol = "";
    if (isNumber(offset)) {
        number = toIntFromHex(getToken(offset).value);
    }
    else {
        symbol = getToken(offset).value;
    }
}
