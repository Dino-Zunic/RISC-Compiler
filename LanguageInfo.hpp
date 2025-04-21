#pragma once

#include <string>
#include <unordered_set>
#include <array>

class LanguageInfo {
public:
    enum class Category : uint32_t {
        Arithmetic1,
        Arithmetic2,
        Arithmetic3,
        LoadStore,
        Jump0,
        Jump1,
        Jump2,
        Count
    };

    enum class AddressMode : uint32_t {
        Immediate = 0b100,
        RegisterDirect = 0b000,
        MemoryDirect = 0b110,
        RegisterIndirect = 0b010,
        RegisterIndirectWithDisplacement = 0b111
    };

    struct Instruction {
        std::string name;
        uint32_t opcode;

        bool operator==(const Instruction& other) const {
            return name == other.name;
        }
    };


    static uint32_t getRegisterIndex(const std::string& name);
    static uint32_t getOpcode(const std::string& name);

    static bool isInstructionInCategory(const std::string& name, Category category);

    static constexpr uint32_t REGISTER_INVALID = ~0;
    static constexpr uint32_t OPCODE_INVALID = ~0;

    static const std::unordered_set<std::string> keywords;

    static std::string normalize(const std::string& name);
private:
    static constexpr size_t CategorySize = static_cast<size_t>(Category::Count);

    static const std::array<std::vector<Instruction>, CategorySize> instructionSets;
};
