#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include <vector>
#include <iostream>

#include "helper.hpp"

namespace Types {
  enum Instruction_Type { HALT, INT, IRET, CALL, RET, JMP, BEQ, BNE, BGT, PUSH, POP, XCHG, ADD, SUB, MUL, DIV, NOT, AND, OR, XOR, SHL, SHR, LD, ST, CSRRD, CSRWR };
  enum Directive_Type { GLOBAL, EXTERN, SECTION, WORD, SKIP, ASCII, EQU, END };
  enum Operand_Type { LIT, SYM, REG, LIT_DIR, SYM_DIR, REG_DIR, REG_LIT, REG_SYM, TST };
}
/*
enum Instruction_Type { HALT, INT, IRET, CALL, RET, JMP, BEQ, BNE, BGT, PUSH, POP, XCHG, ADD, SUB, MUL, DIV, NOT, AND, OR, XOR, SHL, SHR, LD, ST, CSRRD, CSRWR };
enum Directive_Type { GLOBAL, EXTERN, SECTION, WORD, SKIP, ASCII, EQU, END };
enum Operand_Type { LIT, SYM, REG, LIT_DIR, SYM_DIR, REG_DIR, REG_LIT, REG_SYM, TST };
*/
// TODO - Registers?

class Assembler;


// Depending on the operand type, some of the elements will be used ( literal and symbol could be in an union )
struct Operand {
    Types::Operand_Type type;
    int literal;
    std::string symbol;
    int reg = -1;

    friend std::ostream& operator<<(std::ostream& os, const Operand& op) {
        switch(op.type) {
            case Types::LIT: os << "$" << op.literal; break;
            case Types::SYM: os << "$" << op.symbol; break;
            case Types::REG: os << "%" << Helper::regToString(op.reg); break;
            case Types::LIT_DIR: os << op.literal; break;
            case Types::SYM_DIR: os << op.symbol; break;
            case Types::REG_DIR: os << "[%" << Helper::regToString(op.reg) << "]"; break;
            case Types::REG_LIT: os << "[%" << Helper::regToString(op.reg) << " + " << op.literal << "]"; break;
            case Types::REG_SYM: os << "[%" << Helper::regToString(op.reg) << " + " << op.symbol << "]"; break;
        }
        return os;
    }
};

typedef struct Operand Operand;

// Depending on the instruction type, the appropriate(if any) operand will be used
struct Instruction {
    Types::Instruction_Type type;
    int reg1 = -1;
    int reg2 = -1;
    Operand op;

    Instruction() {
        op.type = Types::TST;
    };

    friend std::ostream& operator<<(std::ostream& os, const Instruction& instr) {
        switch(instr.type) {
            case 0: os << "halt"; break;
            case 1: os << "int"; break;
            case 2: os << "iret"; break;
            case 3: os << "call"; break;
            case 4: os << "ret"; break;
            case 5: os << "jmp"; break;
            case 6: os << "beq"; break;
            case 7: os << "bne"; break;
            case 8: os << "bgt"; break;
            case 9: os << "push"; break;
            case 10: os << "pop"; break;
            case 11: os << "xchg"; break;
            case 12: os << "add"; break;
            case 13: os << "sub"; break;
            case 14: os << "mul"; break;
            case 15: os << "div"; break;
            case 16: os << "not"; break;
            case 17: os << "and"; break;
            case 18: os << "or"; break;
            case 19: os << "xor"; break;
            case 20: os << "shl"; break;
            case 21: os << "shr"; break;
            case 22: os << "ld"; break;
            case 23: os << "st"; break;
            case 24: os << "csrrd"; break;
            case 25: os << "csrrw"; break;
        } 
        std::string reg1 = Helper::regToString(instr.reg1);
        std::string reg2 = Helper::regToString(instr.reg2);
        if ( reg1 != "" && instr.type != Types::LD) os << " " << reg1;
        if ( reg2 != "") os << ", " << reg2;

        if ( instr.type == Types::LD ) {
            os << " " << instr.op << ", " << reg1;
        } else if ( instr.type == Types::CALL || instr.type == Types::JMP ) {
	    os << " " << instr.op;
	} else if ( instr.op.type != Types::TST ) {
	    os << ", " << instr.op;
	}

        return os;
    }
};



struct Directive {
    Types::Directive_Type type;
    int literal;
    std::string symbol;
    friend std::ostream& operator<<(std::ostream& os, const Directive& dir) {
        switch (dir.type) {
        case Types::GLOBAL: os << ".global " << dir.symbol; break;
        case Types::EXTERN: os << ".extern " << dir.symbol; break;
        case Types::SECTION: os << ".section " << dir.symbol; break;
        case Types::WORD: os << ".word " << dir.symbol; break;
        case Types::SKIP: os<< ".skip " << dir.literal; break;
        case Types::ASCII: os << ".ascii " << dir.symbol; break;
        case Types::EQU: os << ".equ "; break; // TODO - equ
        case Types::END: os << ".end"; break;
        }
        return os;
    }
};

class Assembler {

    int LC;
    std::vector<std::string> labels;
    std::vector<Instruction> instructions;
    std::vector<Directive> directives;

protected:

    Assembler() {};
    static Assembler* assembler;

public:

    Assembler(Assembler&) = delete;
    void operator=(const Assembler&) = delete;
    static Assembler* getInstance();

    ~Assembler();

    //void startPass();
    void addLabel(std::string label_name);
    void addInstruction(Instruction instruction);
    void addDirective(Directive directive);
    //void endPass();

    friend std::ostream& operator<<(std::ostream& os, const Assembler& as) {
        os << std::endl << "LABELS" << std::endl << "-------------------------------------------" << std::endl;
        for ( const std::string& lab : as.labels ) os << lab << std::endl;
        os << std::endl << "INSTRUCTIONS" << std::endl << "-------------------------------------------" << std::endl;
        for ( const Instruction& instr : as.instructions ) os << instr << std::endl;
        os << std::endl << "DIRECTIVES" << std::endl << "-------------------------------------------" << std::endl;
        for ( const Directive& dir : as.directives ) os << dir << std::endl;
        return os;
    }

};



#endif 
