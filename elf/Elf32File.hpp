#ifndef ELF32FILE_H
#define ELF32FILE_H

#include "./Elf32.hpp"
#include <vector>
#include <unordered_map>
#include <string>

enum Elf32_Type {
    RELOCATABLE,
    EXECUTABLE,
    SHARED_OBJECT
};

class Elf32File {
private:

    struct SectionInfo {
        Elf32_Shdr section_header;
        std::vector<uint8_t> section_contets;

        // TODO - add constructors and destructor 
    };

    Elf32_Ehdr header;
    // Map is used for easier access to sections, so we dont have to access string tables every time
    std::unordered_map<std::string, SectionInfo>* sections;
    // Since segments that program headers point to consist of already existing sections which are already stored in SectionInfo data structure,
    // and program headers are not recognized by their name, theres no need for anything else except just a regular vector with program headers
    std::vector<Elf32_Phdr>* program_header_table;  


public:

    // TODO - copy/move constructors?

    /// @brief Makes an 'empty' Elf32File class of specified type, with set header and default section(s)
    /// @param file_type type of Elf32 file 
    Elf32File(Elf32_Type file_type);
    
    /// @brief Loads an Elf32File class from a file with specified name using the Elf32Parser
    /// @param file_name name of the Elf32 file to be loaded
    Elf32File(std::string file_name);

    ~Elf32File();

    /// @brief Writes contents of the Elf32File class into object file of Elf32 format
    /// @param file_name name of the resulting file
    /// @return 1 if writing was successful, 0 otherwise
    bool writeIntoFile(std::string file_name);

    /// @brief 
    /// @param section_name 
    /// @param data 
    /// @return 
    bool insertIntoSection(std::string section_name, std::vector<uint8_t>& data );

    /// @brief 
    /// @param section_header 
    /// @return 
    bool createSection(Elf32_Shdr section_header);

    /// @brief 
    /// @param program_header 
    /// @return 
    bool createProgramHeader(Elf32_Phdr program_header);

    // TODO - separately track specific sections like symbol table and relocation tables, maybe even create separate classes for Relocatable and Executable files
};



#endif