#include "Assembler.hpp"

#include <cstdlib>

Assembler* Assembler::assembler = nullptr;

Assembler* Assembler::getInstance() {
    if ( assembler == nullptr ) {
        assembler = new Assembler();
    }

    return assembler;
}

Assembler::~Assembler() {
    if ( assembler != nullptr ) {
        delete assembler;
    }
}


void Assembler::addLabel(std::string label_name) {
    labels.push_back(label_name);
}

void Assembler::addInstruction(Instruction instruction) {
    instructions.push_back(instruction);
}

void Assembler::addDirective(Directive directive) {
    directives.push_back(directive);
}
