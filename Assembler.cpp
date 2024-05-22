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

    std::vector<Symbol>& symbol_table_ref = *symbol_table;
    for ( int i = 0; i < symbol_table_ref.size(); i++ ) {
        ForwardRef_Entry* curr = symbol_table_ref[i].flink, *next = nullptr;
        while ( curr ) {
            next = curr->next;
            delete curr;
            curr = next;
        }

        if ( i == symbol_table_ref[i].section ) {
            // Symbol represents section so we have to delete contents vector, literal pools and relocation table

            delete symbol_table_ref[i].contents;

            for ( Reloc_Entry* reloc : *symbol_table_ref[i].reloc_table ) {
                delete reloc;
            }
            delete symbol_table_ref[i].reloc_table;

            for ( auto entry : *symbol_table_ref[i].literal_table ) {
                LiteralRef_Entry* curr = entry.second, *next = nullptr;
                while( curr ) {
                    next = curr->next;
                    delete curr;
                    curr = next;
                }
            }
            delete symbol_table_ref[i].literal_table;

            for ( auto entry : *symbol_table_ref[i].symbol_literal_table ) {
                LiteralRef_Entry* curr = entry.second, *next = nullptr;
                while( curr ) {
                    next = curr->next;
                    delete curr;
                    curr = next;
                }
            }
            delete symbol_table_ref[i].symbol_literal_table;
        }
    }

    delete symbol_table;

}

Assembler::Assembler() {
    LC = 0;
    current_section = -1;
    symbol_table = new std::vector<Symbol>();
    //relocation_table = new std::vector<Reloc_Entry>();
    // add undefined symbol entry
    addSymbol("undefined", STB_LOCAL, true, false, 0, 0);
}


void Assembler::addLabel(std::string label_name) {
    if ( current_section == -1 ) {
        // Error, a label can not stand on its own, outside of a section
        printError("the label '" + label_name + "' must be placed within a section");
    } else {
        int entry = findSymbol(label_name);
        if ( entry >= 0 ) {
            std::vector<Symbol>& symbol_table_ref = *symbol_table;
            if ( symbol_table_ref[entry].defined ) {
                printError("symbol '" + symbol_table_ref[entry].name + "' is already defined");
            } else {
                // Entry for this symbol already exists in symbol table as a result of forward referencing
                // Set all the fields and mark it as defined, forward references will be resolved at the end
                std::vector<Symbol>& symbol_table_ref = *symbol_table;
                symbol_table_ref[entry].section = current_section;
                symbol_table_ref[entry].offset = LC;
                // If the forward reference was made because of global directive, we have to preserve the global bind
                if ( symbol_table_ref[entry].bind != STB_GLOBAL ) symbol_table_ref[entry].bind = STB_LOCAL;
                symbol_table_ref[entry].defined = true;
                // flink and contents are already nullptr
            }
        } else {
            // Entry for this symbol doesnt exist in symbol table so we have to add it
            addSymbol(label_name, STB_LOCAL, true);
            // Default bind is local
        }
    }
    lineno++;
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

int Assembler::addSymbol(std::string name, uint8_t bind, bool defined, bool is_section, uint32_t section, uint32_t offset) {
    int entry = symbol_table->size();

    Symbol sym;
    sym.name = name;
    sym.bind = bind;
    sym.defined = defined;
    sym.contents = nullptr;
    sym.literal_table = nullptr;
    sym.symbol_literal_table = nullptr;
    sym.flink = nullptr;

    if ( is_section ) {
        sym.section = entry;
        sym.offset = 0;
        sym.contents = new std::vector<uint8_t>();
        sym.reloc_table = new std::vector<Reloc_Entry*>();
        sym.literal_table = new std::unordered_map<uint32_t, LiteralRef_Entry*>();
        sym.symbol_literal_table = new std::unordered_map<uint32_t, LiteralRef_Entry*>();
    } else {
        sym.section = (section == -1) ? current_section : section;
        sym.offset = (offset == -1) ? LC : offset;
    }

    symbol_table->push_back(sym);

    return entry;
}

void Assembler::addInstruction(Instruction instruction) {
    if ( current_section == -1 ) {
       printError("the instruction must be placed within a section");
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
            // - 16 for system register because my register name parser gives them values 16, 17 and 18 (should be 0, 1 and 2)
            uint32_t opcode = makeOpcode(0x90, instruction.reg2, instruction.reg1 - 16, 0, 0); 
            addWordToCurentSection(opcode);
            break;
        }
        case Types::CSRWR: {
            // opcode is 0x94YX0000 where Y represents system register and X general purpose register
            uint32_t opcode = makeOpcode(0x94, instruction.reg2 - 16, instruction.reg1, 0, 0); 
            addWordToCurentSection(opcode);
            break;
        }
        case Types::CALL: case Types::JMP: {
            uint32_t ocmod1 = instruction.type == Types::CALL ? 0x21 : 0x38;
            uint32_t ocmod2 = instruction.type == Types::CALL ? 0x20 : 0x30;
            ForwardRef_Type ref_type = instruction.type == Types::CALL ? OPERAND_CALL : OPERAND_JMP;
            if ( instruction.op.type == Types::LIT_DIR && instruction.op.literal <= 0x7ff && instruction.op.literal >= ~0x7ff ) {
                // Literal can fit in 12b
                // opcode for JMP where literal can fit in D is 0x30000DDD 
                // opcode for CALL where literal can fit in D is 0x20000DDD where D represents the literal
                uint32_t opcode = makeOpcode(ocmod2, 0, 0, 0, instruction.op.literal);
                addWordToCurentSection(opcode);
            } else {
                if ( instruction.op.type == Types::LIT_DIR ) {
                    storeLiteral(instruction.op.literal); 
                } else {    // Types::SYM_DIR
                    storeSymbolLiteral(instruction.op.symbol, ref_type, instruction);
                    // If during the process of backpatching it is determined that both symbol and this isntruction are in the same section
                    // offset between those two will be insereted into 12 dispalcement bits, and different opcode will be used
                }
                // opcode for JMP is 0x38F00DDD
                // opcode for CALL is 0x21F00DDD 
                // where D represents displacement to corresponding entry in literal pool which will be added during backpatching
                // This opcode will be overwriten in case described above
                uint32_t opcode = makeOpcode(ocmod1, 15, 0, 0, 0);
                addWordToCurentSection(opcode);
            }
            break;
        }
        case Types::BEQ: case Types::BGT: case Types::BNE: {
            uint32_t ocmod1 = 0x30 | (instruction.type == Types::BEQ ? 0x9 : ( instruction.type == Types::BNE ? 0xA : 0xB ) );
            uint32_t ocmod2 = 0x30 | (instruction.type == Types::BEQ ? 0x1 : ( instruction.type == Types::BNE ? 0x2 : 0x3 ) );
            ForwardRef_Type ref_type = (instruction.type == Types::BEQ ? OPERAND_BEQ : ( instruction.type == Types::BNE ? OPERAND_BNE : OPERAND_BGT ));

            if ( instruction.op.type == Types::LIT_DIR && instruction.op.literal <= 0x7ff && instruction.op.literal >= ~0x7ff ) {
                // Literal can fit in 12b
                // opcode for BTT when literal operand can fit in 12b is 0x3T0XYDDD where X and Y represent the two registers being comapred
                // and D the literal value
                uint32_t opcode = makeOpcode(ocmod2, 0, instruction.reg1, instruction.reg2, instruction.op.literal);
                addWordToCurentSection(opcode);
            } else {
                if ( instruction.op.type == Types::LIT_DIR ) {
                    storeLiteral(instruction.op.literal); 
                } else {    // Types::SYM_DIR
                    storeSymbolLiteral(instruction.op.symbol, ref_type, instruction);
                    // If during the process of backpatching it is determined that both symbol and this isntruction are in the same section
                    // offset between those two will be insereted into 12 dispalcement bits, and different opcode will be used
                }
                // opcode for BTT with direct literal/symbol addressing mode is 0x3TFXYDDD where X and Y represent the two registers being comapred
                // and D displacement to corresponding entry in literal pool which will be added during backpatching
                uint32_t opcode = makeOpcode(ocmod1, 15, instruction.reg1, instruction.reg2, 0);
                addWordToCurentSection(opcode);
            }   
            break;
        }
        case Types::ST: {
            if ( instruction.op.type == Types::LIT || instruction.op.type == Types::SYM ) {
                // Error, store instruction can not be used in combination with immediate addressing
                printError("store instruction can't be used in combination with immediate addressing");
            } else {
                switch (instruction.op.type) {
                case Types::LIT_DIR: case Types::SYM_DIR: {
                    if ( instruction.op.type == Types::LIT_DIR && instruction.op.literal <= 0x7ff && instruction.op.literal >= ~0x7ff ) {
                        // opcode for ST with direct literal addressing mode is 0x8000XDDD where X represents a register being stored
                        // and D the literal address 
                        uint32_t opcode = makeOpcode(0x80, 0, 0, instruction.reg1, instruction.op.literal);
                        addWordToCurentSection(opcode);
                    } else {
                        if (instruction.op.type == Types::LIT) {
                            storeLiteral(instruction.op.literal);
                        } else {
                            storeSymbolLiteral(instruction.op.symbol, OPERAND_ST, instruction);
                        }
                        // opcode for ST with indirect literal/symbol addressing mode is 0x82F0XDDD where X represents a register being stored
                        // and D displacement to corresponding entry in literal pool which will be added during backpatching
                        uint32_t opcode = makeOpcode(0x82, 15, 0, instruction.reg1, 0);
                        addWordToCurentSection(opcode);
                    }
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
                    if ( !(instruction.op.literal <= 0x7ff && instruction.op.literal >= ~0x7ff) ) {
                        // Error, immediate value for this type of addressing has to fit in 12 bits
                        printError("literal used as offset in base register addressing must fit in 12 displacement bits");
                    }
                    uint32_t opcode = makeOpcode(0x80, instruction.op.reg, 0, instruction.reg1, instruction.op.literal);
                    addWordToCurentSection(opcode);
                    break;
                }
                case Types::REG_SYM: {
                    checkSymbol(instruction.op.symbol);

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
                // TODO - emulator will have to fill with sign when extracting 12 bit signed value from instructions displacement bits
                if ( instruction.op.type == Types::LIT && instruction.op.literal <= 0x7ff && instruction.op.literal >= ~0x7ff ) {
                    // opcode for LD with literal immediate addressing mode is 0x91X00DDD where X represents the register being written to
                    // and D the literal being written
                    uint32_t opcode = makeOpcode(0x91, instruction.reg1, 0, 0, instruction.op.literal);
                    addWordToCurentSection(opcode);
                } else {
                    if (instruction.op.type == Types::LIT) {
                        storeLiteral(instruction.op.literal);
                    } else {
                        storeSymbolLiteral(instruction.op.symbol, OPERAND_LD, instruction);
                    }
                    // opcode for LD with direct literal/symbol immediate addressing mode is 0x92XF0DDD where X represents the register being written to
                    // and D displacement to corresponding entry in literal pool which will be added during backpatching
                    uint32_t opcode = makeOpcode(0x92, instruction.reg1, 15, 0, 0);
                    addWordToCurentSection(opcode);
                }
                break;
            }
            case Types::LIT_DIR: case Types::SYM_DIR: {

                if ( instruction.op.type == Types::LIT_DIR && instruction.op.literal <= 0x7ff && instruction.op.literal >= ~0x7ff ) {
                    // opcode for LD with direct literal addressing mode when literal can fit in 12b is 0x92X00DDD
                    // where X represents the register being written to and D the literal value
                    uint32_t opcode = makeOpcode(0x92, instruction.reg1, 0, 0, instruction.op.literal);
                } else {
                    // It is also possible to fit this instruction/addresing mode combination in 1 word if the operand is symbol if it is
                    // determined that symbol is in same section and offset can fit in 12 bits
                    // But, that would require for backpatcher to insert dummy instruction in place of instructions inserted here
                    // So, to keep it simple, if the operand is symbol, its value will always be in lietal pool and the isntruciton will tkae up two words
                    if (instruction.op.type == Types::LIT) {
                        storeLiteral(instruction.op.literal);
                    } else {
                        storeSymbolLiteral(instruction.op.symbol, OPERAND, instruction);
                    }
                    // opcode for LD with direct literal/symbol addressing mode is 0x92XF0DDD + 0x92XX0000 where X represents the register being written to
                    // and D displacement to corresponding entry in literal pool which will be added during backpatching
                    uint32_t opcode1 = makeOpcode(0x92, instruction.reg1, 15, 0, 0),
                            opcode2 = makeOpcode(0x92, instruction.reg1, instruction.reg1, 0, 0);
                    // Forward literal reference entry will be created only for the first word(which is added at the current LC)
                    addWordToCurentSection(opcode1);
                    addWordToCurentSection(opcode2);
                }
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
                if ( !(instruction.op.literal <= 0x7ff && instruction.op.literal >= ~0x7ff) ) {
                    printError("literal used as offset in base register addressing must fit in 12 displacement bits");
                }
                uint32_t opcode = makeOpcode(0x92, instruction.reg1, instruction.op.reg, 0, instruction.op.literal);
                addWordToCurentSection(opcode);
                break;
            }
            case Types::REG_SYM: {
                checkSymbol(instruction.op.symbol);

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
    lineno++;
}

void Assembler::printError(std::string message) {
    std::cout << "assembler:" << lineno << ": error : " << message << std::endl;
    exit(-1);
}

void Assembler::checkSymbol(std::string symbol) {
    std::vector<Symbol> &symbol_table_ref = *symbol_table;
    int entry = findSymbol(symbol);
    if ( entry > 0 ) {                
        if ( symbol_table_ref[entry].defined && symbol_table_ref[entry].section != -1) {
            // -1 is just a placeholder, it should check if symbol belongs to ABS section, or in other words, check if symbol is constant
            // Symbol is not constant, so we report an error
            printError("non constant symbol can't be offset in base register addressing");
        }    
        else {
            // In every other case we add forward reference and move on
            // Even if defined, backpatcher will resolve it
            ForwardRef_Entry* fr_entry = new ForwardRef_Entry();
            fr_entry->section = current_section;
            fr_entry->offset = LC;
            fr_entry->type = CONSTANT;
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
        fr_entry->type = CONSTANT;
        symbol_table_ref[new_entry].flink = fr_entry;
    }
}

void Assembler::resolveSymbol(std::string symbol) {
    // Since we are only working with non constant symbols(their value is their address), we will always have to have relocation for the symbols
    // TODO - constant symbols
    // If it exists but is not defined, we will add this location to the forward reference list of this symbol
    // If it does not exist we will add a new entry with this location in its forward reference list
    // During the process of backaptching the relocation entries will have to be added

    // The previous comment is wrong
    // We won't be checking if symbols value exists in literal table, because that value belongs to real literal and should not change
    // whereas literal belonging to symbol will always change and will have relocation entry refering to it
    // So, when processing symbol values that should be stored in a pool we don't check already existing literals
    // but we put it in pool right away and assign a relocation entry to it, and then, we only use that entry in pool to 
    // resolve all the forward literal symbol references belonging to that symbol
    
    std::vector<Symbol> &symbol_table_ref = *symbol_table;
    int entry = findSymbol(symbol);
    if ( entry > 0 ) {                
        if ( symbol_table_ref[entry].defined ) {
            // If defined, we only have to create a relocation entry
            Reloc_Entry* reloc = new Reloc_Entry();
            reloc->addend = 0;
            if ( symbol_table_ref[entry].bind == STB_GLOBAL ) {
                reloc->symbol = entry;
            } else {
                reloc->symbol = symbol_table_ref[entry].section;
                reloc->addend += symbol_table_ref[entry].offset;
            }
            reloc->offset = LC;
            reloc->section = current_section;
            reloc->type = R_32;
            
            symbol_table_ref[current_section].reloc_table->push_back(reloc);
            //relocation_table->push_back(reloc);
        } else {
            // Not defined, we dont know the value, so we add it to forward ref list
            // If symbol is not defined even during backpatching, that's an error
            // Relocation entry added during backpatching
            ForwardRef_Entry* fr_entry = new ForwardRef_Entry();
            fr_entry->section = current_section;
            fr_entry->offset = LC;
            fr_entry->type = REGULAR;
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
        fr_entry->type = REGULAR;
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

void Assembler::storeSymbolLiteral(std::string symbol, ForwardRef_Type type, Instruction instr) {
    // We will only be adding forward reference to this symbol here and nothing else
    // Symbols value will be added to literal table and displacement will be patched during backpatching

    // During the process of backpatching if it is concluded that symbol and forward reference occure in the same section
    // the displacement has to be filled with offset, and the opcode has to be changed

    // The type of these forward references is used to singalize backpatcher
    // that this forward reference is result of symbol used as an operand in an instruction
    // and in case that the section is same it also tells backpatcher which opcode to use
    std::unordered_map<uint32_t, LiteralRef_Entry*>& symbol_literal_table_ref = *symbol_table->at(current_section).symbol_literal_table; 
    int entry = findSymbol(symbol);
    if ( entry > 0 ) {
        if ( type == OPERAND ) {
            // If type is OPERAND add it directly to symbol literal table
            LiteralRef_Entry* lr = new LiteralRef_Entry();
            lr->offset = LC;
            lr->next = nullptr;
            if ( symbol_literal_table_ref.find(entry) == symbol_literal_table_ref.end() ) {
                symbol_literal_table_ref[entry] = lr; 
            } else {
                lr->next = symbol_literal_table_ref[entry];
                symbol_literal_table_ref[entry] = lr;
            }
        } else {            
            std::vector<Symbol>& symbol_table_ref = *symbol_table;
            ForwardRef_Entry* fr_entry = new ForwardRef_Entry();
            fr_entry->section = current_section;
            fr_entry->offset = LC;
            fr_entry->next = symbol_table_ref[entry].flink;
            fr_entry->type = type; 
            fr_entry->instr = instr;
            symbol_table_ref[entry].flink = fr_entry;
        }
    } else {
        int new_entry = addSymbol(symbol, 0, false, false, 0, 0);

        if ( type == OPERAND ) {
            // If type is OPERAND add it directly to symbol literal table
            LiteralRef_Entry* lr = new LiteralRef_Entry();
            lr->next = nullptr;
            lr->offset = LC;
            symbol_literal_table_ref[new_entry] = lr;
        } else {  
            ForwardRef_Entry* fr_entry = new ForwardRef_Entry();
            fr_entry->section = current_section;
            fr_entry->offset = LC;
            fr_entry->next = nullptr;
            fr_entry->type = type;
            fr_entry->instr = instr;
            symbol_table->at(new_entry).flink = fr_entry;
        }

    }
}


void Assembler::addDirective(Directive directive) {
    if ( current_section == -1 && (directive.type == Types::WORD || directive.type == Types::SKIP || directive.type == Types::ASCII) ) {
        // Error, a directive can not stand on its own, outside of a section
        std::string type = ( directive.type == Types::WORD ? "word" : (directive.type == Types::SKIP ? "skip" : "ascii" ) );
        printError("the directive ." + type + " must be placed within a section");
    } else {
        switch (directive.type) {
       case Types::GLOBAL: {
            // Iterate through list of symbols
            // If symbol is not present in symbol table, we will add an entry with bind set to global and mark it as undefined?
            // In opposite case, we will just set the bind in corresponding entry to global
            std::vector<std::string> elems = Helper::splitString(directive.symbol, ',');
            std::vector<Symbol>& symbol_table_ref = *symbol_table;

            for ( std::string& symbol : elems ) {
                int entry = findSymbol(symbol);
                if ( entry > 0 ) {
                    symbol_table_ref[entry].bind = STB_GLOBAL;
                } else {
                    addSymbol(symbol, STB_GLOBAL, false, false, 0, 0);
                }
            }
            break;
        }
        case Types::EXTERN: {
            // Add a new entry in symbol table for each extern symbol in list
            // TODO - check if symbol was already defined
            std::vector<std::string> elems = Helper::splitString(directive.symbol, ',');

            for ( std::string& symbol : elems ) {
                addSymbol(symbol, STB_GLOBAL, true, false, 0, 0);
                // defined is set to true - This is the difference between extern symbol and forward referenced global symbol
            }
            break;
        }
        case Types::SECTION: {
            int entry = findSymbol(directive.symbol);
            if ( entry > 0 ) {
                printError("redefinition of section '" + directive.symbol + "'"); 
            } else {
                int new_entry = addSymbol(directive.symbol, STB_LOCAL, true, true);

                // Reset LC and set current_section
                LC = 0;
                current_section = new_entry; 
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
                    resolveSymbol(symbol);
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
            std::vector<char> string_vector = processString(directive.symbol);
            // Start from last character because of little endian
            // TODO - maybe stop at first \0 if present
            for ( int i = string_vector.size(); i >= 0 ; i-- ) {
                section_contents.push_back(string_vector[i]);
            }
            LC += string_vector.size();
            break;
        }
        case Types::EQU: {
            // TODO - equ
            break;
        }
        case Types::END: {
            ended = true;
            // Ends the process of assembling
            startBackpatching();
            resolveLiteralPools();
            makeTextFile();
            makeObjectFile();
            break;
        }
        default:
            break;
        }
    }
    lineno++;
}


std::vector<char> Assembler::processString(std::string string) {
    std::vector<char> res;
    for(int i = 0; i < string.length(); i++) {
        if ( string[i] == '\\' && i != string.length() - 1) {
            switch(string[i+1]) {
                case 'n': res.push_back('\n'); break;
                case 't': res.push_back('\t'); break;
                case '0': res.push_back('\0'); break;
                case 'v': res.push_back('\v'); break;
                case 'b': res.push_back('\b'); break;
                case 'r': res.push_back('\r'); break;
                case 'f': res.push_back('\f'); break;
                case 'a': res.push_back('\a'); break;
                case '\'': res.push_back('\''); break;
                case '\"': res.push_back('\"'); break;
                case '\\': res.push_back('\\'); break;
                default: res.push_back('\\'); i--;
            }
            i++;
        } else res.push_back(string[i]);
    }

    return res;
}

void Assembler::end() {
    if ( !ended ) {
        printError("missing .end directive"); 
    }
}


void Assembler::startBackpatching() {
    // Go through every entry in symbol table and resolve all the forward references
    // If the type of forward reference is OPERAND, we have to add the value of that symbol into literal table
    // together with relocation for that literal table entry
    // If the type is DISPLACEMENT, we have to chek if symbol is defined, if not we have to report an error, otherwise,
    // we make a relcoation entry for that location
    // If the type is word, we have to add a relocation entry
    std::vector<Symbol>& symbol_table_ref = *symbol_table;
    // We start from 1 because of default section which is in first entry is of no importance here
    for ( int i = 1; i < symbol_table_ref.size(); i++) {
        if ( !symbol_table_ref[i].defined ) {
            // Every symbol present in symbol table has to be defined after first pass
            // If it's in symbol table that means that it has been referenced at some point in program
            printError("symbol '" + symbol_table_ref[i].name + "' is not defined"); 
            
        } else {
            // Here, we go through reference list and process them based on forward reference type
            
            for ( ForwardRef_Entry* forward_ref = symbol_table_ref[i].flink; forward_ref; forward_ref = forward_ref->next ) {
                switch (forward_ref->type) {
                case REGULAR: {
                    // regular forward reference to write symbols value into one word
                    // TODO - constant symbols wont need relocation, but for now it will always produce relocation entry
                    Reloc_Entry* reloc = new Reloc_Entry();
                    reloc->addend = 0;
                    if ( symbol_table_ref[i].bind == STB_GLOBAL ) {
                        reloc->symbol = i;
                    } else {
                        reloc->symbol = symbol_table_ref[i].section;
                        reloc->addend += symbol_table_ref[i].offset;
                    }
                    reloc->offset = forward_ref->offset;
                    reloc->section = forward_ref->section;
                    reloc->type = R_32;
                    
                    symbol_table_ref[forward_ref->section].reloc_table->push_back(reloc);
                    //relocation_table->push_back(reloc);
                    break;
                }
                case CONSTANT: {
                    if ( symbol_table_ref[i].section != -1 ) {
                        // Again, -1 is placeholder for ABS section
                        // If the symbol is not constant we report an error
                        printError("non constant symbol '" + symbol_table_ref[i].name + "' can't be used as offset in base register addressing"); 
                    } else {
                        // Symbol is constant, so we just add it into 12 displacement bits of instruction this reference points to
                        // We also have to check if it can fit in 12b
                        if ( !(symbol_table_ref[i].offset <= 0x7ff && symbol_table_ref[i].offset >= ~0x7ff) ) {
                            printError("symbol '" + symbol_table_ref[i].name + "' used as offset in base register addressing must fit in 12 displacement bits"); 
                        } else {
                            insertDisplacement(forward_ref->section, forward_ref->offset, symbol_table_ref[i].offset);
                        }
                    }
                    break;
                }
                default: {
                    // All the OPERAND types go here
                    int32_t offset = (int32_t)symbol_table_ref[i].offset - ((int32_t)forward_ref->offset + 4 );
                    uint32_t opcode = 0;
                    switch(forward_ref->type) {
                        // opcode for CALL where offset to symbol can fit in D is 0x20F00DDD where represents D the offset to that symbol
                        case OPERAND_CALL: opcode = makeOpcode(0x20, 15, 0, 0, offset); break;
                        // opcode for JMP where offset to symbol can fit in D is 0x30F00DDD where represents D the offset to that symbol
                        case OPERAND_JMP: opcode = makeOpcode(0x30, 15, 0, 0, offset); break;
                        // opcode for ST with symbol direct addresing mode where offset to symbol can fit in D is 0x80F0XDDD where represents D the offset to that symbol
                        case OPERAND_ST: opcode = makeOpcode(0x80, 15, 0, forward_ref->instr.reg1, offset); break;
                        // opcode for LD with symbol immediate addressing mode where offset to symbol can fit in D is 0x91XF0DDD where represents D the offset to that symbol
                        case OPERAND_LD: opcode = makeOpcode(0x91, forward_ref->instr.reg1, 15, 0, offset); break;
                        // opcode for BTT where offset to symbol can fit in D is 0x3TFXYDDD where D represents D the offset to that symbol
                        case OPERAND_BEQ: opcode = makeOpcode(0x31, 15, forward_ref->instr.reg1, forward_ref->instr.reg2, offset); break;
                        case OPERAND_BNE: opcode = makeOpcode(0x32, 15, forward_ref->instr.reg1, forward_ref->instr.reg2, offset); break;
                        case OPERAND_BGT: opcode = makeOpcode(0x33, 15, forward_ref->instr.reg1, forward_ref->instr.reg2, offset); break;
                    }

                    // The literal symbol values that have to be resolved in all the OPERAND types will not be added to pool via litera table
                    // rather, they will be added to symbol literal table and dealt with later on
                    // This is done because the way real literals and literals that originate from symbols are handled is different

                    // TODO - USE SIGNED TYPES WHERE NEEDED
                    if ( forward_ref->section == symbol_table_ref[i].section && offset <= 0x7ff && offset >= ~0x7ff) {                     
                        // Patch the instruction with new opcode
                        patchWord(forward_ref->section, forward_ref->offset, opcode);
                    }
                    else {
                        // The reference and symbol are not in same section or offset can't fit in 12b
                        // We add this reference to symbol literal table with key being referenced symbol
                        std::unordered_map<uint32_t, LiteralRef_Entry*>& symbol_literal_table_ref = *symbol_table->at(forward_ref->section).symbol_literal_table;
			            LiteralRef_Entry* lr = new LiteralRef_Entry();
                        lr->offset = forward_ref->offset;
                        lr->next = nullptr;
                        if ( symbol_literal_table_ref.find(i) == symbol_literal_table_ref.end() ) {
                            symbol_literal_table_ref[i] = lr; 
                        } else {
                            lr->next = symbol_literal_table_ref[i];
                            symbol_literal_table_ref[i] = lr;
                        }
                    }
                    break;
                }
                }          
            }
		// TODO - free forward reference structures

        }
    }
}

void Assembler::resolveLiteralPools() {
    // We have to add all the literal and symbol literal pools at the end of their respective sections and insert offsets in instructions 
    // that reference literals from the pool
    // For entries in symbol literal pool we will also have to generate relocation entries

    std::vector<Symbol>& symbol_table_ref = *symbol_table;
    for ( int i = 1; i < symbol_table_ref.size(); i++ ) {
        // We will know that symbol represents a section if his index is equal to his section field
        if ( symbol_table_ref[i].section != i ) continue;

        // Since current_section and LC are not important anymore i will them to store the section(and its LC) whose pools are being added
        // We do this so we can use addWordToCurrentSection function which operates on these valeus
        // LC of the section is its size after first pass
        LC = symbol_table_ref[i].contents->size();
        current_section = i;

        // First we deal with regular literal pool

        for ( auto& entry : *symbol_table_ref[i].literal_table ) {
            // We go through every entry in literal table, add the literal to section and patch all the instructions that reference it
            addWordToCurentSection(entry.first);

            for ( LiteralRef_Entry* ref = entry.second; ref; ref = ref->next) {
                // Location of entry for this literal is LC - 4 (because addWordToCurentSection function updates LC)
                int32_t offset = (LC - 4) - ((int32_t)ref->offset + 4);
                insertDisplacement(current_section, ref->offset, offset);
            }

            // TODO - delete literal tables and references in them in destructor
        }

        // Symbol literal table

        for ( auto& entry : *symbol_table_ref[i].symbol_literal_table ) {
            // We go through every entry in symbol literal table, reserve space for it and add relocation entry
            // then we patch patch all the instructions that reference it
            
            addWordToCurentSection(0);

            for ( LiteralRef_Entry* ref = entry.second; ref; ref = ref->next) {
                // Location of entry for this literal is LC - 4 (because addWordToCurentSection function updates LC)
                int32_t offset = (LC - 4) - ((int32_t)ref->offset + 4);
                insertDisplacement(current_section, ref->offset, offset);
            }

            Reloc_Entry* reloc = new Reloc_Entry();
            reloc->addend = 0;
            if ( symbol_table_ref[entry.first].bind == STB_GLOBAL ) {
                reloc->symbol = entry.first;
            } else {
                reloc->symbol = symbol_table_ref[entry.first].section;
                reloc->addend += symbol_table_ref[entry.first].offset;
            }
            reloc->offset = LC - 4;
            reloc->section = current_section;
            reloc->type = R_32;

            symbol_table_ref[current_section].reloc_table->push_back(reloc);
            //relocation_table->push_back(reloc);

            // TODO - delete literal tables and references in them in destructor
        }

    }

    
}


void Assembler::insertDisplacement(uint32_t section, uint32_t offset, uint32_t value) {
    std::vector<unsigned char>& contents_ref = *symbol_table->at(section).contents;
    //                  -------------------------------------------------------------------------------------------------
    // Instriction:     |   OC      |   MOD     |   RegA    |   RegB    |   RegC    |   Disp2   |   Disp1   |   Disp0   |
    //                  -------------------------------------------------------------------------------------------------
    // In memory:
    //                  -------------------------------------------------------------------------------------------------
    // Offset:          |           +0          |           +1          |           +2          |           +3          |
    //                  -------------------------------------------------------------------------------------------------
    // Bytes:           |   Disp1   |   Disp0   |   RegC    |   Disp2   |   RegA    |   RegB    |   OC      |   MOD     |
    //                  -------------------------------------------------------------------------------------------------

    // Put lower byte of value in first byte at offset which represents lower byte of displacement
    contents_ref[offset] = (unsigned char)(value & 0xff);
    // Put remaining nibble of value in lower nibble of byte at offset + 1 which represents highest 4 bits of dispalcement
    contents_ref[offset + 1] &= 0xf0;
    contents_ref[offset + 1] |= (unsigned char)((value >> 8) & 0xf);
}

void Assembler::patchWord(uint32_t section, uint32_t offset, uint32_t word) {
    std::vector<unsigned char>& contents_ref = *symbol_table->at(section).contents;

    for ( int i = 0; i < 4; i++) {
        // Data is stored in little endian, so we start from lowest byte
        unsigned char byte = word;
        word >>= 8;
        contents_ref[offset + i] = byte; 
    }
}


void Assembler::makeTextFile() {

    std::ofstream fout("output.txt");
    
    std::vector<Symbol>& symbol_table_ref = *symbol_table;

    fout << std::endl << "SYMBOL TABLE" << std::endl << "------------------------------------------------" << std::endl;

    fout << std::hex  << std::left;

    // TODO - remove Def column(Its just for testing)
    fout << std::setw(4) << "Ind" << std::setw(15) << "Name" << std::setw(11) << "Value" << std::setw(5) << "Bind" 
    << std::setw(8) << "Section" << std::setw(3) << "Def" << std::endl;

    for ( int i = 0; i < symbol_table_ref.size(); i++) {
        fout << std::setw(4) << i << std::setw(15) << std::left << symbol_table_ref[i].name;
        fout << std::showbase << std::internal << std::setfill('0');
        if ( symbol_table_ref[i].offset == 0 ) fout << "0x00000000";
        else fout << std::setw(10) << symbol_table_ref[i].offset;
        fout << " ";
        fout << std::noshowbase << std::setfill(' ') << std::left;
        fout << std::setw(5) << (symbol_table_ref[i].bind == STB_GLOBAL ? "G" : (symbol_table_ref[i].bind != STB_LOCAL ? "N" : "L" ) );
        fout << std::setw(8) << symbol_table_ref[i].section << std::setw(3) << ( symbol_table_ref[i].defined == true ? "1" : "0" ) << std::endl;
    }

    fout << std::endl << "SECTIONS" << std::endl << "------------------------------------------------" << std::endl;
    fout << std::noshowbase << std::internal << std::setfill('0');
    
    for ( int i = 1; i < symbol_table_ref.size(); i++) {
        if ( symbol_table_ref[i].section != i ) continue;
        fout << "'" << symbol_table_ref[i].name << "':" << std::endl;

        std::vector<unsigned char>& contents_ref = *symbol_table_ref[i].contents;
        for ( int j = 0; j < contents_ref.size(); j += 4 ) {
            if ( !(j%16) ) fout << std::setw(4) << j << ": ";
            uint32_t word = (uint32_t)contents_ref[j] | ((uint32_t)contents_ref[j+1] << 8) | ((uint32_t)contents_ref[j+2] << 16) | ((uint32_t)contents_ref[j+3] << 24);
            fout << std::setw(8) << word << " ";
            if ( j != 0 && j != contents_ref.size() - 1 && !((j+4) % 16) ) fout << std::endl;
        }
        fout << std::endl << std::endl;
    }

    fout << std::endl << "RELOCATION TABLES" << std::endl << "------------------------------------------------" << std::endl;
    fout << std::showbase << std::setfill(' ') << std::left;

    for ( int i = 1; i < symbol_table_ref.size(); i++) {
        if ( symbol_table_ref[i].section != i || symbol_table_ref[i].reloc_table->size() == 0 ) continue;
        fout << "'" << symbol_table_ref[i].name << ".rela':" << std::endl;

        std::vector<Reloc_Entry*>& reloc_table_ref = *symbol_table_ref[i].reloc_table; 

        fout << std::noshowbase << std::setfill(' ') << std::left;

        fout << std::setw(11) << "Offset" << std::setw(5) << "Type" << std::setw(7) << "Symbol" << std::setw(10) << "Addend" << std::endl;
        for ( int j = 0; j < reloc_table_ref.size(); j++) {
            fout << std::showbase << std::internal << std::setfill('0');
            if ( reloc_table_ref[j]->offset == 0 ) fout << "0x00000000";
            else fout << std::setw(10) << reloc_table_ref[j]->offset;
            fout << " ";
            fout << std::noshowbase << std::setfill(' ') << std::left;
            fout << std::setw(5) << ( reloc_table_ref[j]->type == R_32 ? "R_32" : "R_32S" ); 
            fout << std::setw(7) << reloc_table_ref[j]->symbol;
            fout << std::showbase << std::internal << std::setfill('0');
            if ( reloc_table_ref[j]->addend == 0 ) fout << "0x00000000";
            else fout << std::setw(10) << reloc_table_ref[j]->addend;
            fout << std::endl;
        }
        fout << std::endl;
    }

    fout.close();
}


void Assembler::makeObjectFile() {
    Elf32File obj_file("output.o", ET_REL);
    obj_file.addSymbolTable(symbol_table);
    
    std::vector<Symbol>& symbol_table_ref = *symbol_table;
    for ( int i = 1; i < symbol_table_ref.size(); i++ ) {
        if ( i != symbol_table_ref[i].section ) continue;
        obj_file.addAssemblerSection(symbol_table_ref[i]);
    }

    obj_file.writeIntoFile();
}