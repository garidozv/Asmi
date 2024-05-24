#ifndef ELF32FILE_H
#define ELF32FILE_H

#include "./Elf32Mod.hpp"
#include "../AssemblerDefs.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>
#include <iomanip>

/*
    These Elf32 files will have an usual format, with program header table right after the file header
    and section header table at the end of the file wtih sections(segments) in between 
*/

class Elf32File {
private:

    struct SectionInfo {
        Elf32_Shdr* header;
        std::vector<uint8_t>* contents;

        SectionInfo(Elf32_Shdr* section_header, std::vector<uint8_t>* contents) : header(section_header), contents(contents) {
        };
        ~SectionInfo() {
            if ( contents ) delete contents;
            delete header;
        }
    };
    
    std::string name;

    // Elf32 file header
    Elf32_Ehdr* header;
    // String table which holds symbol name strings
    std::vector<std::string>* string_table;
    uint32_t str_tab_size = 0;

    // List of sections
    std::vector<SectionInfo*>* sections;

    // Since segments that program headers point to consist of already existing sections which are already stored in SectionInfo data structure,
    // and program headers are not recognized by their name, theres no need for anything else except just a regular vector with program headers
    std::vector<Elf32_Phdr*>* program_header_table;  

    // Separate fields for symbol table section and relocation table sections

    // Sumbol table
    std::vector<Elf32_Sym*>* symbol_table;
    
    // Section names with corresponding relocation tables
    std::unordered_map<uint32_t, std::vector<Elf32_Rela*>*>* relocation_tables;


    inline Elf32_Shdr* createSectionHeader(std::string name, Elf32_Word type, Elf32_Word addr, Elf32_Off offset, Elf32_Word size, Elf32_Word info, Elf32_Word link);

    uint32_t addString(std::string string);

    void printHex(std::ostream& os, uint32_t number, int width, bool prefix = false) {
        std::ios old_state(nullptr);
        old_state.copyfmt(os);

        if ( prefix ) os << std::showbase;
        os << std::hex << std::internal << std::setfill('0');

        // We do this because showbase wont work if number is 0
        if ( prefix && number == 0 ) os << "0x" << std::setw(width - 2) << "";
        else os << std::setw(width) << number;

        os.copyfmt(old_state);
    }

    std::string elf_sym_types[5] {
        "NOTYP",
        "OBJCT",
        "FUNC",
        "SCTN",
        "FILE"
    };

    std::string elf_sym_binds[3] {
        "LOC",
        "GLOB",
        "WEAK"
    };

    std::string elf_rela_types[1] {
        "RELOC_32"
    };

public:

    // TODO - copy/move constructors?

    // TODO - change empty flag
    Elf32File(std::string file_name, Elf32_Half file_type, bool empty = false);

    // TODO - destructor
    ~Elf32File();

    bool makeBinaryFile();
    bool readFromFile();

    void makeTextFile();


    void addSymbolTable(std::vector<Symbol>* symbol_table);
    void addAssemblerSection(Symbol section);
};



#endif