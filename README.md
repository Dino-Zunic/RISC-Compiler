# Custom RISC‑Compiler

This project implements a simple compiler for a bespoke RISC‑style assembly language. Written in modern C++, it reads human‑readable `.asm` files, tokenizes them, parses them into an intermediate representation of instructions, resolves symbols and labels, and finally emits machine‑readable bytecode. The workflow is exposed through a command‑line interface that supports listing, tokenizing, parsing, linking, and compiling of assembly modules.

## Overview

At its heart, the compiler chain consists of four main stages: tokenization, parsing, linking, and code generation. The tokenizer breaks source text into `Token` objects representing names, numbers, and symbols. The parser turns those tokens into `Command` objects, each of which represents either an instruction (with up to three register operands, an immediate value, or a memory reference), a data directive, a symbol definition, or a label. The linker then inlines included files, checks for circular includes, and builds a global symbol table to assign addresses and detect an entry point. Finally, the compiler converts each `Command` into a 32‑bit word of RISC bytecode, emitting a Memory Initialization File (MIF) or directly printing hex words to standard output.

## Language Specification

The custom RISC dialect is defined centrally in `LanguageInfo`. Instruction names, reserved keywords, data types, and grammar rules live in this single class, so extending the language (for example, adding new instructions or addressing modes) requires changes only here. The class exposes static queries such as

```cpp
uint32_t idx = LanguageInfo::getRegisterIndex("r3");
uint32_t op  = LanguageInfo::getOpcode("add");
bool inCat   = LanguageInfo::isInstructionInCategory("lw", LanguageInfo::Category::LoadStore);
```

and a normalization routine that lowercases and trims instruction names for case‑insensitive matching.

## Instruction Categories

Instructions are grouped into logical families—arithmetic with one, two, or three operands; load/store; and three flavors of jump or branch. These categories drive opcode allocation and help the parser choose the correct operand pattern. For instance, a three‑register add (“add r1, r2, r3”) belongs to the `Arithmetic3` category, whereas a branch‑if‑equal (“beq r1, r2, label”) falls under `Jump1`.

## Addressing Modes

Memory references support five distinct modes, encoded in three‑bit fields in the machine word. Immediate values are recognized when a constant is prefixed with `#`, register‑direct mode refers to a register operand alone, memory‑direct uses a bare address constant, register‑indirect takes a register as a pointer, and register‑indirect with displacement combines a base register, an offset symbol or number, and a sign:

```cpp
enum class AddressMode : uint32_t {
    Immediate                     = 0b100,
    RegisterDirect                = 0b000,
    MemoryDirect                  = 0b110,
    RegisterIndirect              = 0b010,
    RegisterIndirectWithDisplacement = 0b111
};
```

Displacement mode commands carry additional fields for the sign (`+` or `–`), the numeric offset or symbol, and the base register index.

## Command Representation

Every executable or data statement is modeled by the `Command` class. Besides the type (instruction, directive, symbol definition, or label), each command holds its mnemonic, addressing mode, up to three register operands (`r1`, `r2`, `r3`), a 32‑bit immediate or data value, and an optional symbol name for relocatable references. The `toString()` method enables easy printing of each command in an assembly‑like syntax:

```cpp
Command c = Command::createInstruction3("add", 1, 2, 3);
std::cout << c.toString();  // “add r1, r2, r3”
```

## Building and Using the CLI

After cloning the repository and configuring your C++17 toolchain, compile with your preferred build system. Running the resulting binary presents a simple REPL prompt. Typing

```
$ tokenize example
```

will read `C:/assembly/example.asm`, emit the list of tokens, and report any lexical errors. The commands `parse`, `link`, and `compile` perform the subsequent stages. A successful `compile example` run prints each bytecode word in hexadecimal, prefixed by its instruction address.

## Project Structure

Source files are organized by feature: `Tokenizer` and `Token` handle lexical analysis; `Parser` produces the `Command` vector; `Linker` resolves includes and symbols; `Compiler` drives the code‑generation and output. The `LanguageInfo` and `Error` classes provide shared definitions for instruction metadata and error reporting, respectively. The `main.cpp` file wires these components into a user‑friendly CLI.

## Extending the Language

To add new instructions, simply append to the `instructionSets` array within `LanguageInfo.cpp`, assign a unique opcode, and categorize it appropriately. To support new data directives or syntax forms, extend the parser’s `parseDirective` or add new parsing methods, then update `Command` factory methods to represent the new constructs. With the language metadata all in one place, such changes propagate naturally through lexing, parsing, and code generation, keeping the compiler coherent and maintainable.
