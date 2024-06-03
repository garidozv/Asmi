#ifndef ASSEMBLERDEFS_H
#define ASSEMBLERDEFS_H

#include <string>
#include <iostream>
#include <unordered_map>
#include <vector>

class EquDefinition;

namespace Types {
  enum Instruction_Type { HALT, INT, IRET, CALL, RET, JMP, BEQ, BNE, BGT, PUSH, POP, XCHG, ADD, SUB, MUL, DIV, NOT, AND, OR, XOR, SHL, SHR, LD, ST, CSRRD, CSRWR };
  enum Directive_Type { GLOBAL, EXTERN, SECTION, WORD, SKIP, ASCII, EQU, END };
  enum Operand_Type { LIT, SYM, REG, LIT_DIR, SYM_DIR, REG_DIR, REG_LIT, REG_SYM };
  enum { PLUS, MINUS };
};

struct Expression {
    uint8_t sign = Types::PLUS;
    std::string symbol;
    Expression* next;
};

struct TNSEntry {
    EquDefinition* def;
    TNSEntry* next;
};


//class Assembler;

// Depending on the operand type, some of the elements will be used ( literal and symbol could be in an union )
struct Operand {
    Types::Operand_Type type;
    int literal;
    std::string symbol;
    int reg = -1;
};

// Depending on the instruction type, the appropriate(if any) operand will be used
struct Instruction {
    Types::Instruction_Type type;
    int reg1 = -1;
    int reg2 = -1;
    Operand op;
};


struct Directive {
    Types::Directive_Type type;
    int literal;
    std::string symbol;
    Expression* expr;
};




enum ForwardRef_Type { REGULAR, OPERAND, OPERAND_JMP, OPERAND_CALL, OPERAND_BEQ,
                         OPERAND_BNE, OPERAND_BGT, OPERAND_LD, OPERAND_ST, CONSTANT };
// OPERAND_INSTR - symbols value is used as an operand in an INSTR instruction - backpatcher maybe has to modify written isntruction
// OPERAND - like previous, with only difference being that backpatcher wont be trying to fit the literal into instructions displacement field
// REGULAR - regular forward ref of symbol that has to be inserted at given offset
// CONSTANT - symbols value has to be inserted into instruction as displacement - check if constant and if it can fit in 12b

struct ForwardRef_Entry {
    uint32_t section;           // Section in which forward reference occured
    uint32_t offset;            // Offset at which forward reference occured
    ForwardRef_Type type;       // Indicates the type of forward reference that has to be resolved during backpatching
    Instruction instr;          // Instruction used for OPERAND type references  
    ForwardRef_Entry* next;
};

struct LiteralRef_Entry {
    uint32_t offset;
    LiteralRef_Entry* next;
};

// I won't be using PC relocation types, since all references to symbols in different sections will use absolute address
// and references in same section will be resolved by assembler and wont have relocation entry
struct Reloc_Entry {
    uint32_t section;
    uint32_t offset;
    uint8_t type;
    uint32_t symbol;
    uint32_t addend;
};

struct Symbol {
    std::string name;
    uint32_t section;
    uint32_t offset; // value
    uint8_t bind;
    bool defined;
    ForwardRef_Entry* flink;
    std::vector<uint8_t>* contents;   // Contents of a section if the symbol represents one
    std::vector<Reloc_Entry*>* reloc_table; // Relocation table 
    std::unordered_map<uint32_t, LiteralRef_Entry*>* literal_table; // Literal table of a section if symbol represents one
    std::unordered_map<uint32_t, LiteralRef_Entry*>* symbol_literal_table; // Table of literals originated from symbols - keys are symbol indexes

    uint32_t rel;   // for equ symbols
};





#endif
