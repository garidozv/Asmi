#include "Assembler.hpp"

#include <cstdlib>

void Assembler::addLabel(std::string label_name) {
    labels.push_back(label_name);
}

void Assembler::addInstruction(Instruction instruction) {
    instructions.push_back(instruction);
}

void Assembler::addDirective(Directive directive) {
    directives.push_back(directive);
}
