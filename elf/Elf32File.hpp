#ifndef ELF32FILE_H
#define ELF32FILE_H

#include "./Elf32Mod.hpp"
#include "../AssemblerDefs.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>

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

public:

    // TODO - copy/move constructors?

    // TODO - change empty flag
    Elf32File(std::string file_name, Elf32_Half file_type, bool empty = false);

    // TODO - destructor
    ~Elf32File();

    bool writeIntoFile();
    bool readFromFile();


    void addSymbolTable(std::vector<Symbol>* symbol_table);
    void addAssemblerSection(Symbol section);
};



#endif