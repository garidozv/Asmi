#ifndef ELF32FILE_H
#define ELF32FILE_H

#include "Elf32Mod.hpp"
#include "../assembler/AssemblerDefs.hpp"
#include "../Helper.hpp"
#include "../Table.hpp"
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

        SectionInfo(Elf32_Shdr* section_header, std::vector<uint8_t>* contents) : header(section_header), contents(contents) {};
        ~SectionInfo() {
            if ( contents ) delete contents;
            delete header;
        }
    };

    struct SegmentInfo {
        Elf32_Phdr* header;
        // TODO - SectionInfo instead of these two
        std::vector<uint8_t>* contents;
        Elf32_Shdr* starting_section;   // Points to starting section of this segment

        SegmentInfo(Elf32_Phdr* program_header, std::vector<uint8_t>* contents, Elf32_Shdr* sec_header)
         : header(program_header), contents(contents), starting_section(sec_header) {};
        ~SegmentInfo() {
            // Section header and contents will be deleted in SectionInfo
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
    Table<SectionInfo*>* sections;
    // Segments which consist of program header and segment contents
    Table<SegmentInfo*>* segments; 
    // Sumbol table
    Table<Elf32_Sym*>* symbol_table;   
    // Section names with corresponding relocation tables
    std::unordered_map<uint32_t, std::vector<Elf32_Rela*>*>* relocation_tables;

    uint32_t addString(std::string string);

    static const std::string elf_sym_types[5];
    static const std::string elf_sym_binds[3];
    static const std::string elf_rela_types[1];

public:

    Elf32File(std::string file_name, Elf32_Half file_type, bool empty = false);
    ~Elf32File();

    void makeBinaryFile();
    void readFromFile();
    void makeTextFile();
    void makeHexDumpFile();


    void addSymbolTable(Table<Symbol*>& symbol_table);
    void addAssemblerSection(Symbol* section);

    Elf32_Phdr* createProgramHeader(Elf32_Word type, Elf32_Off offset, Elf32_Addr vaddr, Elf32_Word size);
    Elf32_Shdr* createSectionHeader(std::string name, Elf32_Word type, Elf32_Word addr, Elf32_Off offset, Elf32_Word size, Elf32_Word info, Elf32_Word link);
    void addSection(Elf32_Shdr* header, std::vector<uint8_t>* contents) { sections->put(getString(header->sh_name) ,new SectionInfo(header, contents)); };
    void addSegment(Elf32_Phdr* header, std::vector<uint8_t>* contents, Elf32_Shdr* starting_section) { segments->put(getString(starting_section->sh_name), new SegmentInfo(header, contents, starting_section)); };
    uint32_t addSymbol(std::string name, Elf32_Addr value, Elf32_Word size, unsigned char info, std::string section);

    void appendToSection(uint32_t index, std::vector<uint8_t>* contents);
    void patchSectionContents(std::string section_name, uint32_t offset, uint32_t word);

    Elf32_Half getType() const { return header->e_type; };

    std::string getString(uint32_t index) const { return string_table->at(index); };
    
    uint32_t getNumberOfSections() const { return sections->size(); };
    Elf32_Shdr* getSectionHeader(uint32_t index) const { return sections->get(index)->header; };
    Elf32_Shdr* getSectionHeader(std::string name) const { return sections->get(name)->header; };
    uint32_t getSectionIndex(std::string section_name) const { return sections->getIndex(section_name); };
    std::vector<uint8_t>* getSectionContents(uint32_t index) const { return sections->get(index)->contents; };

    uint32_t getNumberOfSegments() const { return segments->size(); };
    Elf32_Phdr* getSegmentHeader(uint32_t index) const { return segments->get(index)->header; };
    std::vector<uint8_t>* getSegmentContents(uint32_t index) const { return segments->get(index)->contents; };

    std::vector<Elf32_Rela*>* getRelocationTable(uint32_t index) const { return relocation_tables->at(index); };
    std::vector<Elf32_Rela*>* getRelocationTable(std::string section_name);
    Elf32_Shdr* getRelocSectionHeader(std::string section_name) const { return sections->get(".rela." + section_name)->header; };

    Table<Elf32_Sym*>* getSymbolTable() const { return symbol_table; };
    Elf32_Sym* getSymbol(std::string name) const { if ( symbol_table->exists(name) ) return symbol_table->get(name); else return nullptr; };
    uint32_t getSymbolIndex(std::string name) const { if ( symbol_table->exists(name) ) return symbol_table->getIndex(name); else return -1; };
    
};



#endif