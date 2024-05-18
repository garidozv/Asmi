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
    delete symbol_table;
    delete relocation_table;
}

Assembler::Assembler() {
    LC = 0;
    current_section = -1;
    symbol_table = new std::vector<Symbol>();
    relocation_table = new std::vector<Reloc_Entry>();
    // add undefined symbol entry
    addSymbol("undefined", STB_LOCAL, true, false, 0, 0);
}


void Assembler::addLabel(std::string label_name) {
    if ( current_section == -1 ) {
        // Error, a label can not stand on its own, outside of a section
    } else if ( int entry = findSymbol(label_name) >= 0 ) {
        // Entry for this symbol already exists in symbol table as a result of forward referencing
        // Set all the fields and mark it as defined, forward references will be resolved at the end
        std::vector<Symbol>& symbol_table_ref = *symbol_table;
        symbol_table_ref[entry].section = current_section;
        symbol_table_ref[entry].offset = LC;
        symbol_table_ref[entry].bind = STB_LOCAL;
        symbol_table_ref[entry].defined = true;
        // flink and contents are already nullptr
    } else {
        // Entry for this symbol doesnt exist in symbol table so we have to add it
        addSymbol(label_name, STB_LOCAL, true);
        // Default bind is local
    }
}

int Assembler::findSymbol(std::string symbol_name) {
    std::vector<Symbol>& symbol_table_ref = *symbol_table;
    for ( int i = 0; i < symbol_table_ref.size(); i++ ) {
        if ( symbol_table_ref[i].name == symbol_name ) return i;
    }
    return -1;
}

void Assembler::addWordToCurentSection(uint32_t word) {
    std::vector<unsigned char>& contents_ref = *symbol_table->at(current_section).contents;
    for ( int i = 0; i < 4; i++) {
        // Data is stored in little endian, so we start from lowest byte
        unsigned char byte = word;
        word >>= 8;
        contents_ref.push_back(byte);
    }
    LC += 4;
}

int Assembler::addSymbol(std::string name, uint8_t bind, bool defined, bool is_section = false, uint32_t section = -1, uint32_t offset = -1) {
    int entry = symbol_table->size();

    Symbol sym;
    sym.name = name;
    sym.bind = bind;
    sym.defined = defined;
    sym.contents = nullptr;
    sym.literal_table = nullptr;
    sym.flink = nullptr;

    if ( is_section ) {
        sym.section = entry;
        sym.offset = 0;
        sym.contents = new std::vector<uint8_t>();
        sym.literal_table = new std::unordered_map<uint32_t, LiteralRef_Entry*>();
    } else {
        sym.section = (section == -1) ? current_section : section;
        sym.offset = (offset == -1) ? LC : offset;
    }

    symbol_table->push_back(sym);

    return entry;
}

void Assembler::addInstruction(Instruction instruction) {
    if ( current_section == -1 ) {
        // Error, an instruction can not stand on its own, outside of a section
    } else {
        switch (instruction.type) {
        case Types::HALT: {
            uint32_t opcode = 0x00000000;
            addWordToCurentSection(opcode);
            break;
        }
        case Types::INT: {
            uint32_t opcode = 0x10000000;
            addWordToCurentSection(opcode);
            break;
        }
        case Types::IRET: {
            uint32_t opcode1 = 0x93FE0004, opcode2 = 0x970E0004;
            addWordToCurentSection(opcode1);
            addWordToCurentSection(opcode2);
            break;
        }
        case Types::RET: {
            uint32_t opcode = 0x93FE0004;
            addWordToCurentSection(opcode);
            break;
        }
        case Types::PUSH: {
            // opcode is 0x81E0XFFC where X represents registar being pushed
            uint32_t opcode = makeOpcode(0x81, 14, 0, instruction.reg1, -4);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::POP: {
            // opcode is 0x93XE0004 where X represents registar being pushed
            uint32_t opcode = makeOpcode(0x93, instruction.reg1, 14, 0, 4);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::XCHG: {
            // opcode is 0x400XY000 where X represents first register and Y second register
            uint32_t opcode = makeOpcode(0x40, 0, instruction.reg1, instruction.reg2, 0);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::ADD: {
            // opcode is 0x50YYX000 where Y represents first operand and destionation register and X second operand 
            uint32_t opcode = makeOpcode(0x50, instruction.reg2, instruction.reg2, instruction.reg1, 0);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::SUB: {
            // opcode is 0x51YYX000 where Y represents first operand and destionation register and X second operand 
            uint32_t opcode = makeOpcode(0x51, instruction.reg2, instruction.reg2, instruction.reg1, 0);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::MUL: {
            // opcode is 0x52YYX000 where Y represents first operand and destionation register and X second operand 
            uint32_t opcode = makeOpcode(0x52, instruction.reg2, instruction.reg2, instruction.reg1, 0);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::DIV: {
            // opcode is 0x53YYX000 where Y represents first operand and destionation register and X second operand 
            uint32_t opcode = makeOpcode(0x53, instruction.reg2, instruction.reg2, instruction.reg1, 0);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::NOT: {
            // opcode is 0x60XX0000 where X represent register that's being complemented
            uint32_t opcode = makeOpcode(0x60, instruction.reg1, instruction.reg1, 0, 0);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::AND: {
            // opcode is 0x61YYX000 where Y represents first operand and destionation register and X second operand
            uint32_t opcode = makeOpcode(0x61, instruction.reg2, instruction.reg2, instruction.reg1, 0);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::OR: {
            // opcode is 0x62YYX000 where Y represents first operand and destionation register and X second operand
            uint32_t opcode = makeOpcode(0x62, instruction.reg2, instruction.reg2, instruction.reg1, 0);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::XOR: {
            // opcode is 0x63YYX000 where Y represents first operand and destionation register and X second operand
            uint32_t opcode = makeOpcode(0x63, instruction.reg2, instruction.reg2, instruction.reg1, 0);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::SHL: {
            // opcode is 0x70YYX000 where Y represents first operand and destionation register and X second operand
            uint32_t opcode = makeOpcode(0x70, instruction.reg2, instruction.reg2, instruction.reg1, 0);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::SHR: {
            // opcode is 0x71YYX000 where Y represents first operand and destionation register and X second operand
            uint32_t opcode = makeOpcode(0x71, instruction.reg2, instruction.reg2, instruction.reg1, 0);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::CSRRD: {
            // opcode is 0x90YX0000 where X represents system register and Y general purpose register
            uint32_t opcode = makeOpcode(0x90, instruction.reg2, instruction.reg1, 0, 0); 
            addWordToCurentSection(opcode);
            break;
        }
        case Types::CSRWR: {
            // opcode is 0x94YX0000 where Y represents system register and X general purpose register
            uint32_t opcode = makeOpcode(0x94, instruction.reg2, instruction.reg1, 0, 0); 
            addWordToCurentSection(opcode);
            break;
        }
        case Types::CALL: case Types::JMP: {
            if ( instruction.op.type == Types::LIT_DIR ) {
                storeLiteral(instruction.op.literal);
            } else {    // Types::SYM_DIR
                storeSymbolLiteral(instruction.op.symbol);
            }
            // opcode for JMP is 0x38F00DDD
            // opcode for CALL is 0x21F00DDD 
            // where D represents displacement to corresponding entry in literal pool which will be added during backpatching
            uint32_t ocmod = instruction.type == Types::CALL ? 0x21 : 0x38;
            uint32_t opcode = makeOpcode(ocmod, 15, 0, 0, 0);
            addWordToCurentSection(opcode);
            break;
        }
        case Types::BEQ: case Types::BGT: case Types::BNE: {
            uint32_t ocmod = 0x30 | (instruction.type == Types::BEQ ? 0x9 : ( instruction.type == Types::BNE ? 0xA : 0xB ) );
            switch (instruction.op.type) {
            case Types::LIT: case Types::SYM: {
                if (instruction.type == Types::LIT) {
                    storeLiteral(instruction.op.literal);
                } else {
                    storeSymbolLiteral(instruction.op.symbol);
                }
                // opcode for BTT with direct literal/symbol immediate addressing mode is 0x3TFXYDDD where X and Y represent the two registers being comapred
                // and D displacement to corresponding entry in literal pool which will be added during backpatching
                uint32_t opcode = makeOpcode(ocmod, 15, instruction.reg1, instruction.reg2, 0);
                addWordToCurentSection(opcode);
                break;
            }
            case Types::LIT_DIR: case Types::SYM_DIR: {
                if (instruction.type == Types::LIT) {
                    storeLiteral(instruction.op.literal);
                } else {
                    storeSymbolLiteral(instruction.op.symbol);
                }
                // opcode for BTT with direct literal/symbol addressing mode is 0x3TFXYDDD + 0x3TFXY000 where X and Y represent the two registers being comapred
                // and D displacement to corresponding entry in literal pool which will be added during backpatching
                uint32_t opcode = makeOpcode(ocmod, 15, instruction.reg1, instruction.reg2, 0);
                // This instruction/addresing mod combination takes up two bytes, the first one containing displacement to literal pool
                // and second one not, but since the displacement will be added later on, opcode are the same here
                // Forward literal reference entry will be created only for the first word(which is added at the current LC)
                addWordToCurentSection(opcode);
                addWordToCurentSection(opcode);
                break;
            }
            case Types::REG: {
                // opcode for BTT with register addressing mode is 0x3TZXY000 where X and Y represent the two registers being comapred
                // and Z the register which holds an address
                // this addessing mod is the only one with different mods for opcodes
                ocmod = 0x30 | (instruction.type == Types::BEQ ? 0x1 : ( instruction.type == Types::BNE ? 0x2 : 0x3 ) );
                uint32_t opcode = makeOpcode(ocmod, instruction.op.reg, instruction.reg1, instruction.reg2, 0);
                addWordToCurentSection(opcode);
                break;
            }
            case Types::REG_DIR: {
                // opcode for BTT with register indirect addresing mode is 0x3TZXY000 where X and Y represent the two registers being comapred
                // and Z the register which holds a memory address of an actual address
                uint32_t opcode = makeOpcode(ocmod, instruction.op.reg, instruction.reg1, instruction.reg2, 0);
                addWordToCurentSection(opcode);
                break;
            }
            case Types::REG_LIT: {
                // opcode for BTT with base register addressing mode is 0x3TZXYDDD where X and Y represent the two registers being comapred,
                // Z the base register, and D an immediate literal being added to base register
                if ( instruction.op.literal > 0xfff ) {
                    // Error, immediate value for this type of addressing has to fit in 12 bits
                }
                uint32_t opcode = makeOpcode(ocmod, instruction.op.reg, instruction.reg1, instruction.reg2, instruction.op.literal);
                addWordToCurentSection(opcode);
                break;
            }
            case Types::REG_SYM: {
                resolveSymbol(instruction.op.symbol, DELIMETER);

                // opcode for BTT with base register addressing mode is 0x3TZXYDDD where X and Y represent the two registers being comapred,
                // Z the base register, and D an immediate symbol literal being added to base register
                uint32_t opcode = makeOpcode(ocmod, instruction.op.reg, instruction.reg1, instruction.reg2, 0);
                addWordToCurentSection(opcode);
                break;
            }
            break;
            }
        }
        case Types::ST: {
            if ( instruction.op.type == Types::LIT || instruction.op.type == Types::SYM ) {
                // Error, store instruction can not be used in combination with immediate addressing
            } else {
                switch (instruction.op.type) {
                case Types::LIT_DIR: case Types::SYM_DIR: {
                    if (instruction.type == Types::LIT) {
                        storeLiteral(instruction.op.literal);
                    } else {
                        storeSymbolLiteral(instruction.op.symbol);
                    }
                    // opcode for ST with indirect literal/symbol addressing mode is 0x82F0XDDD where X represents a register being stored
                    // and D displacement to corresponding entry in literal pool which will be added during backpatching
                    uint32_t opcode = makeOpcode(0x82, 15, 0, instruction.reg1, 0);
                    addWordToCurentSection(opcode);
                    break;
                }
                case Types::REG: {
                    // opcode for ST with register addressing mode is 0x91YX0000 where X represents a register being stored and Y the destination register
                    uint32_t opcode = makeOpcode(0x91, instruction.op.reg, instruction.reg1, 0, 0);
                    addWordToCurentSection(opcode);
                    break;
                }
                case Types::REG_DIR: {
                    // opcode for ST with register indirect addresing mode is 0x80Y0X000 where X represents a register being stored and Y the register that holds the addres to be written to
                    uint32_t opcode = makeOpcode(0x80, instruction.op.reg, 0, instruction.reg1, 0);
                    addWordToCurentSection(opcode);
                    break;
                }
                case Types::REG_LIT: {
                    // opcode for ST with base register addressing mode is 0x80Y0XDDD where X represents a register being stored,
                    // Y the base register and D an immediate literal being added to base register 
                    if ( instruction.op.literal > 0xfff ) {
                        // Error, immediate value for this type of addressing has to fit in 12 bits
                    }
                    uint32_t opcode = makeOpcode(0x80, instruction.op.reg, 0, instruction.reg1, instruction.op.literal);
                    addWordToCurentSection(opcode);
                    break;
                }
                case Types::REG_SYM: {
                    resolveSymbol(instruction.op.symbol, DELIMETER);

                    // opcode for ST with base register addressing mode is 0x80Y0XDDD where X represents a register being stored,
                    // Y the base register and D an immediate symbol literal being added to base register 
                    uint32_t opcode = makeOpcode(0x80, instruction.op.reg, 0, instruction.reg1, 0);
                    addWordToCurentSection(opcode);
                    break;
                }
                }  
            }    
            break;
        }
        case Types::LD: {
            switch (instruction.op.type) {
            case Types::LIT: case Types::SYM: {
                if (instruction.type == Types::LIT) {
                    storeLiteral(instruction.op.literal);
                } else {
                    storeSymbolLiteral(instruction.op.symbol);
                }
                // opcode for LD with direct literal/symbol immediate addressing mode is 0x92XF0DDD where X represents the register being written to
                // and D displacement to corresponding entry in literal pool which will be added during backpatching
                uint32_t opcode = makeOpcode(0x92, instruction.reg1, 15, 0, 0);
                addWordToCurentSection(opcode);
                break;
            }
            case Types::LIT_DIR: case Types::SYM_DIR: {
                if (instruction.type == Types::LIT) {
                    storeLiteral(instruction.op.literal);
                } else {
                    storeSymbolLiteral(instruction.op.symbol);
                }
                // opcode for LD with direct literal/symbol addressing mode is 0x92XF0DDD + 0x92XX0000 where X represents the register being written to
                // and D displacement to corresponding entry in literal pool which will be added during backpatching
                uint32_t opcode1 = makeOpcode(0x92, instruction.reg1, 15, 0, 0),
                         opcode2 = makeOpcode(0x92, instruction.reg1, instruction.reg1, 0, 0);
                // Forward literal reference entry will be created only for the first word(which is added at the current LC)
                addWordToCurentSection(opcode1);
                addWordToCurentSection(opcode2);
                break;
            }
            case Types::REG: {
                // opcode for LD with register addressing mode is 0x91XY0000 where X represents the register being written to 
                // and Y the register being read from
                uint32_t opcode = makeOpcode(0x91, instruction.reg1, instruction.op.reg, 0, 0);
                addWordToCurentSection(opcode);
                break;
            }
            case Types::REG_DIR: {
                // opcode for LD with register indirect addresing mode is 0x92XY0000 where X represents the register being written to
                // and Y the register that holds the memory addres of value to be written
                uint32_t opcode = makeOpcode(0x92, instruction.reg1, instruction.op.reg, 0, 0);
                addWordToCurentSection(opcode);
                break;
            }
            case Types::REG_LIT: {
                // opcode for LD with base register addressing mode is 0x92XY0DDD where X represents the register being written to,
                // Y the base register, and D an immediate literal being added to base register
                if ( instruction.op.literal > 0xfff ) {
                    // Error, immediate value for this type of addressing has to fit in 12 bits
                }
                uint32_t opcode = makeOpcode(0x92, instruction.reg1, instruction.op.reg, 0, instruction.op.literal);
                addWordToCurentSection(opcode);
                break;
            }
            case Types::REG_SYM: {
                resolveSymbol(instruction.op.symbol, DELIMETER);

                // opcode for LD with base register addressing mode is 0x92XY0DDD where X represents the register being written to,
                // Y the base register, and D an immediate symbol literal being added to base register
                uint32_t opcode = makeOpcode(0x92, instruction.reg1, instruction.op.reg, 0, 0);
                addWordToCurentSection(opcode);
                break;
            }
            break;
        }
        }
        }
    }
    instructions.push_back(instruction); // testing
}

void Assembler::resolveSymbol(std::string symbol, ForwardRef_Type type) {
    // TODO - ..
    // Since our symbols can only have non constant values for now, this type of addressing will always require an relocation entry
    // because values are bound to change during the process of linking
    // Size of symbol value has to fit into 12 bits, so that has to be checked during backpatching as well

    // Check if symbol exists in symbol table, if it does(and is defined) we will use symbol value to initalize it
    // Since we are only working with non constant symbols(their value is their address), we will always have to have relocation for the symbols
    // TODO - constant symbols
    // If it exists but is not defined, we will add this location to the forward reference list of this symbol
    // If it does not exist we will add a new entry with this location in its forward reference list
    // During the process of backaptching the relocation entries will have to be added
    
    std::vector<Symbol> &symbol_table_ref = *symbol_table;

    if ( int entry = findSymbol(symbol) > 0 ) {                
        if ( symbol_table_ref[entry].defined ) {
            // If defined, we only have to create a relocation entry
            Reloc_Entry reloc;
            reloc.addend = 0;
            if ( symbol_table_ref[entry].bind == STB_GLOBAL ) {
                reloc.symbol = entry;
            } else {
                reloc.section = symbol_table_ref[entry].section;
                reloc.addend += symbol_table_ref[entry].offset;
            }
            reloc.offset = LC;
            reloc.section = current_section;
            reloc.type = type == DELIMETER ? R_12S : R_32;
            
            relocation_table->push_back(reloc);
        } else {
            // Not defined, we dont know the value, so we add it to forward ref list
            // If symbol is not defined even during backpatching, that's an error
            // Relocation entry added during backpatching
            ForwardRef_Entry* fr_entry = new ForwardRef_Entry();
            fr_entry->section = current_section;
            fr_entry->offset = LC;
            fr_entry->type = type;
            fr_entry->next = symbol_table_ref[entry].flink;
            symbol_table_ref[entry].flink = fr_entry;
        }
    } else {
        // Symbol does not exist yet, add an undefined symbol entry and forward reference
        int new_entry = addSymbol(symbol, 0, false, false, 0, 0);

        ForwardRef_Entry* fr_entry = new ForwardRef_Entry();
        fr_entry->section = current_section;
        fr_entry->offset = LC;
        fr_entry->next = nullptr;
        fr_entry->type = type;
        symbol_table_ref[new_entry].flink = fr_entry;
    }
}

void Assembler::storeLiteral(uint32_t literal) {
    std::unordered_map<uint32_t, LiteralRef_Entry*>& literal_table_ref = *symbol_table->at(current_section).literal_table; 
    if ( literal_table_ref.find(literal) != literal_table_ref.end() ) {
        // Literal already exist in literal table, add this location to reference list
        LiteralRef_Entry* lr = new LiteralRef_Entry();
        // Since data is stored in little endian, displacement bits will be at the lowest address of this isntructions opcode
        // First byte stores last byte of displacement
        // Second bytes high nibble stores first 4 bits of displacement
        lr->offset = LC;
        lr->next = literal_table_ref[literal];
        literal_table_ref[literal] = lr;
    } else {
        // Literal doesn't exist in literal table, so we have to add it with ref list having this location
        LiteralRef_Entry* lr = new LiteralRef_Entry();
        lr->next = nullptr;
        lr->offset = LC;
        literal_table_ref[literal] = lr;
    }
}

void Assembler::storeSymbolLiteral(std::string symbol) {
    // We will only be adding forward reference to this symbol here and nothing else
    // Symbols value will be added to literal table and displacement will be patched during backpatching
    if ( int entry = findSymbol(symbol) > 0 ) {
        std::vector<Symbol>& symbol_table_ref = *symbol_table;
        ForwardRef_Entry* fr_entry = new ForwardRef_Entry();
        fr_entry->section = current_section;
        fr_entry->offset = LC;
        fr_entry->next = symbol_table_ref[entry].flink;
        fr_entry->type = OPERAND;   // It referes to operand
        symbol_table_ref[entry].flink = fr_entry;
    } else {
        int new_entry = addSymbol(symbol, 0, false, false, 0, 0);

        ForwardRef_Entry* fr_entry = new ForwardRef_Entry();
        fr_entry->section = current_section;
        fr_entry->offset = LC;
        fr_entry->next = nullptr;
        fr_entry->type = OPERAND;   // It referes to operand
        symbol_table->at(new_entry).flink = fr_entry;
    }
}


void Assembler::addDirective(Directive directive) {
    if ( current_section == -1 ) {
        // Error, a directive can not stand on its own, outside of a section
    } else {
        switch (directive.type) {
        case Types::GLOBAL: {
            // If symbol is not present in symbol table, we will add an entry with bind set to global and mark it as undefined?
            // In opposite case, we will just set the bind in corresponding entry to global
            if ( int entry = findSymbol(directive.symbol) > 0 ) {
                std::vector<Symbol>& symbol_table_ref = *symbol_table;
                symbol_table_ref[entry].bind = STB_GLOBAL;
            } else {
                addSymbol(directive.symbol, STB_GLOBAL, false, false, 0, 0);
            }
            break;
        }
        case Types::EXTERN: {
            // Add a new entry in symbol table for this extern symbol
            // TODO - check if symbol was already defined
            addSymbol(directive.symbol, STB_GLOBAL, true, false, 0, 0);
            // defined is set to true - This is the difference between extern symbol and forward referenced global symbol
            break;
        }
        case Types::SECTION: {
            if ( int entry = findSymbol(directive.symbol) > 0 ) {
                // Error, section(or any other symbol) with same name already defined
            } {
                int entry = addSymbol(directive.symbol, STB_LOCAL, true, true);

                if ( current_section != -1 ) {
                    // TODO - add literal pool - no, it will be done at the end
                }

                // Reset LC and set current_section
                LC = 0;
                current_section = entry; 
            }
            break;
        }
        case Types::WORD: {
            std::vector<std::string> elems = Helper::splitString(directive.symbol, ',');

            // Get contents of current section, since we will be inserting words into it
            std::vector<unsigned char>& section_contents = *(symbol_table->at(current_section).contents);
            // Check if it's a symbol or literal list
            if ( std::isdigit(elems[0][0]) ) {  // We check it this way since symbols can't start with a number
                for ( std::string& literal : elems ) {
                    // For each literal from the list we have to insert 4 bytes into current section and initialize those 4 bytes with this literal value
                    addWordToCurentSection(std::stoi(literal));
                }
            } else {
                for ( std::string& symbol : elems ) {
                    resolveSymbol(symbol, WORD);
                    // Reserve space in section
                    addWordToCurentSection(0);
                }
            }
            break;
        }
        case Types::SKIP: {
            std::vector<unsigned char>& section_contents = *(symbol_table->at(current_section).contents);
            for ( int i = 0; i < directive.literal; i++ ) {
                section_contents.push_back(0);
            }
            LC += directive.literal;
            break;
        }
        case Types::ASCII: {
            // Does not null terminate the string
            std::vector<unsigned char>& section_contents = *(symbol_table->at(current_section).contents);
            // TODO - special symbols '/n' ...
            // Start from last character because of little endian
            for ( int i = directive.symbol.length(); i >= 0 ; i-- ) {
                section_contents.push_back((unsigned char)directive.symbol[i]);
            }
            LC += directive.symbol.length(); // if there were special chars, it will be less than length
            break;
        }
        case Types::EQU: {
            // TODO - equ
            break;
        }
        case Types::END: {
            // Ends the process of assembling
            // TODO - Start backpatching, finish up all the literal pools and make an object file
            break;
        }
        default:
            break;
        }
    }

    directives.push_back(directive);    // testing
}
