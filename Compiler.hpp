#pragma once

#include <vector>
#include <string>
#include "Linker.hpp"

class Compiler {
public:
    Compiler(const std::string& rootFilename);

    bool compile();
    const std::vector<Error>& getErrors() const;
    bool hasErrors() const;
    const std::vector<uint32_t>& getMachineCode() const;
private:
    std::string rootFilename;
    std::vector<Error> errors;
    std::vector<uint32_t> machineCode;
    bool generateMIF(const std::string& outputFilename);

    bool generateMachineCode(const Command& command, std::vector<uint32_t>& output);
    bool processCommands(const std::vector<Command>& commands);
    std::string replaceFileExtension(const std::string& filename, const std::string& newExtension);
};
