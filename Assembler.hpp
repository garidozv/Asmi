#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <vector>
#include <cctype>
#include <stdint.h>
#include <iomanip>
#include <fstream>

#include "Helper.hpp"
#include "AssemblerDefs.hpp"
#include "./elf/Elf32Mod.hpp"
#include "./elf/Elf32File.hpp"


class Assembler {

    int LC;

    // TODO - use Symbol pointer
    std::vector<Symbol>* symbol_table;
    //std::vector<Reloc_Entry>* relocation_table;
    int32_t current_section;
    bool ended = false;
    int lineno = 0;

    std::string output_file_name = "";

    // looks for a symbol in symbol table, returns -1 if not present
    // made so i can implement it more efficiently later on
    int findSymbol(std::string symbol_name);
    void addWordToCurentSection(uint32_t word);
    void patchWord(uint32_t section, uint32_t offset, uint32_t word);
    void insertDisplacement(uint32_t section, uint32_t offset, uint32_t value);
    
    // Adds symbol to symbol table
    // If not section and section and offset are not provided, sets offset to current LC, and section to current section
    // If section, sets offset to 0, and section to itself
    // Does not change LC or current_section
    int addSymbol(std::string name, uint8_t bind, bool defined, bool is_section = false, uint32_t section = -1, uint32_t offset = -1);
    static uint32_t makeOpcode(uint32_t ocmod, uint32_t reg_a, uint32_t reg_b, uint32_t reg_c, uint32_t disp) {
        return ( ocmod << 24 ) | ( (reg_a & 0xf) << 20 ) | ( (reg_b & 0xf) << 16 ) | ( (reg_c & 0xf) << 12 ) | ( disp & 0xfff );
    }

    void storeLiteral(uint32_t literal);
    void storeSymbolLiteral(std::string symbol, ForwardRef_Type type, Instruction instr);
    void resolveSymbol(std::string symbol);
    void checkSymbol(std::string);
    std::vector<char> processString(std::string string);
       
    void startBackpatching();
    void resolveLiteralPools();
    void printError(std::string message);

    //void makeTextFile();
    void makeOutputFiles();

protected:

    Assembler();
    static Assembler* assembler;

public:

    Assembler(Assembler&) = delete;
    void operator=(const Assembler&) = delete;
    static Assembler* getInstance();

    ~Assembler();

    void setOutputFileName(std::string file_name) { output_file_name = file_name; };
    void end();
    void newLine() { lineno++; };

    void addLabel(std::string label_name);
    void addInstruction(Instruction instruction);
    void addDirective(Directive directive);

};



#endif 
