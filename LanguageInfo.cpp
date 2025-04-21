#include "LanguageInfo.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_map>

const std::array<std::vector< LanguageInfo::Instruction>, LanguageInfo::CategorySize> LanguageInfo::instructionSets = { {
    { {"inc", 0x00},
      {"dec", 0x01},
      {"asl", 0x02},
      {"asr", 0x03} },

    { {"real_clamp", 0x10), 
      {"int_real", 0x11} },

    { {"add", 0x20},
      {"sub", 0x21},
      {"mul", 0x22},
      {"real_mul", 0x23},
      {"and", 0x24},
      {"or", 0x25},
      {"xor", 0x26} },

    { {"load", 0x30},
      {"store", 0x31} },

    { {"ret", 0x40} },

    { {"jmp", 0x41},
      {"call", 0x42} },

    { {"jz", 0x50},
      {"jnz", 0x51},
      {"jlz", 0x52},
      {"jlez", 0x53},
      {"jgz", 0x54},
      {"jgez", 0x55} }
} };

const std::unordered_set<std::string> LanguageInfo::keywords = { "def", "include", "start", "org", "dd", "dup" };

const std::unordered_map<std::string, uint32_t> specialRegisters = {
    {"zero", 0}, {"ra", 1}, {"sp", 2}, {"gp", 3},
    {"fp", 8}, {"s0", 8}, {"s1", 9}, {"a0", 10},
    {"a1", 11}, {"a2", 12}, {"a3", 13}, {"a4", 14},
    {"a5", 15}, {"t0", 5}, {"t1", 6}, {"t2", 7},
    {"t3", 28}, {"t4", 29}, {"t5", 30}, {"t6", 31},
    {"ivtp", 26}, {"imr", 27}
};


std::string LanguageInfo::normalize(const std::string& name) {
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return std::tolower(c);
        });
    return normalized;
}

uint32_t LanguageInfo::getRegisterIndex(const std::string& name) {
    std::string normalized = normalize(name);
    auto it = specialRegisters.find(normalized);
    if (it != specialRegisters.end()) {
        return it->second;
    }
    if (normalized[0] == 'r' && normalized.size() > 1) {
        try {
            int regIndex = std::stoi(normalized.substr(1));
            if (regIndex >= 0 && regIndex <= 31) {
                return static_cast<size_t>(regIndex);
            }
        }
        catch (const std::exception&) {
            return REGISTER_INVALID;
        }
    }
    return REGISTER_INVALID;
}

uint32_t LanguageInfo::getOpcode(const std::string& name) {
    std::string normalized = normalize(name);
    for (const auto& categorySet : instructionSets) {
        auto it = std::find_if(categorySet.begin(), categorySet.end(), [&normalized](const Instruction& instr) {
            return instr.name == normalized;
            });
        if (it != categorySet.end()) {
            return it->opcode;
        }
    }
    return OPCODE_INVALID;
}

bool LanguageInfo::isInstructionInCategory(const std::string& name, Category category) {
    std::string normalized = normalize(name);
    const auto& categorySet = instructionSets[static_cast<size_t>(category)];
    return std::find_if(categorySet.begin(), categorySet.end(), [&normalized](const Instruction& instr) {
        return instr.name == normalized;
        }) != categorySet.end();
}
