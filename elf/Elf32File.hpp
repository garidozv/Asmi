#ifndef ELF32FILE_H
#define ELF32FILE_H

#include "./Elf32Mod.hpp"
#include <vector>
#include <unordered_map>
#include <string>

/*
    These Elf32 files will have an usual format, with program header table right after the file header
    and section header table at the end of the file wtih sections(segments) in between 
*/

class Elf32File {
private:

    struct SectionInfo {
        Elf32_Shdr* header;
        std::vector<uint8_t>* contents;

        SectionInfo(Elf32_Shdr* section_header, bool empty = false) : header(section_header) {
            if ( !empty ) contents = new std::vector<uint8_t>();
            else contents = nullptr;
        };
        ~SectionInfo() {
            if ( contents ) delete contents;
            delete header;
        }
    };

    // Elf32 file header
    Elf32_Ehdr* header;
    // String table which holds symbol name strings
    std::vector<std::string>* string_table;
    // String table of section names
    std::vector<std::string>* shdr_string_table;

    // List of sections
    std::vector<SectionInfo*>* sections;
    // Map that uses section names for efficient access to sections using their name
    std::unordered_map<std::string, SectionInfo*>* sections_map;

    // Since segments that program headers point to consist of already existing sections which are already stored in SectionInfo data structure,
    // and program headers are not recognized by their name, theres no need for anything else except just a regular vector with program headers
    std::vector<Elf32_Phdr*>* program_header_table;  

    // Separate fields for symbol table section and relocation table sections

    // Sumbol table
    std::vector<Elf32_Sym*>* symbol_table;
    // Map of symbol names and corresponging table entries
    std::unordered_map<std::string, Elf32_Sym*>* symbol_table_map;
    
    // Section names with corresponding relocation tables
    std::unordered_map<std::string, std::vector<Elf32_Rela*>*>* relocation_tables;

    //static Elf32_Shdr* createSectionHeader(std::string )

public:

    // TODO - copy/move constructors?

    /// @brief Makes an 'empty' Elf32File class of specified type, with set header, default section header, string table sections and symbol table section if needed
    /// @param file_type type of Elf32 file 
    Elf32File(Elf32_Half file_type);
    
    /// @brief Loads an Elf32File class from a file with specified name using the Elf32Parser
    /// @param file_name name of the Elf32 file to be loaded
    Elf32File(std::string file_name);

    ~Elf32File();

    /// @brief Writes contents of the Elf32File class into object file of Elf32 format
    /// @param file_name name of the resulting file
    /// @return 1 if writing was successful, 0 otherwise
    bool writeIntoFile(std::string file_name);

    /// @brief Creates a regular section 
    /// @param section_name name of the section to be created 
    /// @param type section type
    /// @param flags section attribute flags 
    /// @return positive number on success, negative number if there was an error
    int createSection(std::string section_name, Elf32_Word type, Elf32_Word flags, Elf32_Addr addr);
    // TODO - maybe type and flags not needed, logic on that can be in this class

};



#endif