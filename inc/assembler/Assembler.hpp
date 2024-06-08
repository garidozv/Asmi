#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <vector>
#include <cctype>
#include <stdint.h>
#include <iomanip>
#include <fstream>
#include <unordered_map>

#include "../Helper.hpp"
#include "../assembler/AssemblerDefs.hpp"
#include "../assembler/EquDefinition.hpp"
#include "../elf/Elf32Mod.hpp"
#include "../elf/Elf32File.hpp"
#include "../Table.hpp"


class Assembler {

    friend class EquDefinition;

    int32_t LC;

    Table<Symbol*>* symbol_table;
    //std::vector<Symbol>* symbol_table;
    //std::vector<Reloc_Entry>* relocation_table;
    int32_t current_section;
    bool ended = false;
    int32_t lineno = 0;

    std::string output_file_name = "";
    std::string input_file_name = "";

    // Table of unresolved symbols
    TNSEntry* tns = nullptr;

    void addWordToCurrentSection(uint32_t word);
    void patchWord(uint32_t section, uint32_t offset, uint32_t word);
    void insertDisplacement(uint32_t section, uint32_t offset, uint32_t value);
    
    // Adds symbol to symbol table
    // If not section and section and offset are not provided, sets offset to current LC, and section to current section
    // If section, sets offset to 0, and section to itself
    // Does not change LC or current_section
    uint32_t addSymbol(std::string name, uint8_t bind, bool defined, uint32_t section, uint32_t offset, uint32_t rel = 0, bool is_section = false);
    uint32_t addForwardRefSymbol(std::string name) {
        return addSymbol(name, 0, 0, 0, 0, 0);
    }
    static uint32_t makeOpcode(uint32_t ocmod, uint32_t reg_a, uint32_t reg_b, uint32_t reg_c, uint32_t disp) {
        return ( ocmod << 24 ) | ( (reg_a & 0xf) << 20 ) | ( (reg_b & 0xf) << 16 ) | ( (reg_c & 0xf) << 12 ) | ( disp & 0xfff );
    }
    static bool checkDisplacementFit(int32_t value) {
        return value <= 0x7ff && value >= ~0x7ff;
    }

    // Removes a symbol from symbol table, and updates everything(symbol table and reloc entries)
    void removeSymbol(uint32_t index);

    void storeLiteral(uint32_t literal);
    void storeSymbolLiteral(std::string symbol, ForwardRef_Type type, Instruction instr);
    void resolveSymbol(std::string symbol);
    void checkSymbol(std::string);
    std::vector<char> processString(std::string string);

    void processEqu(std::string name, Expression* expr);
    void fixExtern();
    void fixEqu();
    void startBackpatching();
    void resolveLiteralPools();
    void resolveTNS();
    void printError(std::string message);
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
    void setInputFileName(std::string file_name) { input_file_name = file_name; };
    std::string getInputFileName() { return input_file_name; };

    void addLabel(std::string label_name);
    void addInstruction(Instruction instruction);
    void addDirective(Directive directive);

};



#endif 
