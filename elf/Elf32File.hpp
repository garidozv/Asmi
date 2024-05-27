#ifndef ELF32FILE_H
#define ELF32FILE_H

#include "./Elf32Mod.hpp"
#include "../assembler/AssemblerDefs.hpp"
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
    std::vector<SectionInfo*>* sections;

    // Segments which consist of program header and segment contents
    std::vector<SegmentInfo*>* segments; 

    // Separate fields for symbol table section and relocation table sections

    // Sumbol table
    std::vector<Elf32_Sym*>* symbol_table;
    
    // Section names with corresponding relocation tables
    std::unordered_map<uint32_t, std::vector<Elf32_Rela*>*>* relocation_tables;

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

    Elf32_Phdr* createProgramHeader(Elf32_Word type, Elf32_Off offset, Elf32_Addr vaddr, Elf32_Word size);
    Elf32_Shdr* createSectionHeader(std::string name, Elf32_Word type, Elf32_Word addr, Elf32_Off offset, Elf32_Word size, Elf32_Word info, Elf32_Word link);
    void addSection(Elf32_Shdr* header, std::vector<uint8_t>* contents) { sections->push_back(new SectionInfo(header, contents)); };
    void addSegment(Elf32_Phdr* header, std::vector<uint8_t>* contents, Elf32_Shdr* starting_section) { segments->push_back(new SegmentInfo(header, contents, starting_section)); };
    uint32_t addSymbol(std::string name, Elf32_Addr value, Elf32_Word size, unsigned char info, std::string section);

    void appendToSection(uint32_t index, std::vector<uint8_t>* contents);
    void patchSectionContents(std::string section_name, uint32_t offset, uint32_t word);

    uint32_t getSectionIndex(std::string section_name);

    std::vector<Elf32_Sym*>* getSymbolTable() const { return symbol_table; };
    std::vector<std::string>* getStringTable() const { return string_table; };
    Elf32_Shdr* getSectionHeader(uint32_t index) const { return sections->at(index)->header; };
    Elf32_Shdr* getSectionHeader(std::string name) { return sections->at(getSectionIndex(name))->header; };
    std::vector<uint8_t>* getSectionContents(uint32_t index) const { return sections->at(index)->contents; };
    std::vector<Elf32_Rela*>* getRelocationTable(uint32_t index) const { return relocation_tables->at(index); };
    uint32_t getNumberOfSections() { return sections->size(); };
    uint32_t getNumberOfSegments() { return segments->size(); };
    Elf32_Phdr* getSegmentHeader(uint32_t index) const { return segments->at(index)->header; };
    std::vector<uint8_t>* getSegmentContents(uint32_t index) const { return segments->at(index)->contents; };
    Elf32_Sym* getSymbol(std::string name);
    uint32_t getSymbolIndex(std::string name);
    std::vector<Elf32_Rela*>* getRelocationTable(std::string section_name);
    Elf32_Shdr* getRelocSectionHeader(std::string section_name);
    Elf32_Half getType() { return header->e_type; };
};



#endif