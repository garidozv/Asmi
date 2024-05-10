#include "Elf32File.hpp"

Elf32File::Elf32File(Elf32_Half file_type) {
    header = new Elf32_Ehdr();
    string_table = new std::vector<std::string>();
    shdr_string_table = new std::vector<std::string>();
    // Entry is initialized to zero, and will have to be changed at some point
    header->e_entry = 0;
    // Offset to section header table is initialized with 0, and will have to be set after everything is done, since it is at the end of the file
    header->e_shoff = 0;
    // Intialized with 0, will have to be set at the end if the file has program header table
    header->e_phnum = 0;
    // Intialized with 0, will have to be set at the end if the file has section header table
    header->e_shnum = 0;

    // Since all of the files used in this project will have section header table, we will initialize it by adding 
    // the default entry at index 0, entry associated with section name string table whose index will be 1, and entry for string table at index 2
    header->e_shstrndx = 1;

    // Add names of string table sections to section header string table
    // TODO - With or without dots??
    shdr_string_table->push_back(".shstrtab");
    shdr_string_table->push_back(".strtab");
    sections_map = new std::unordered_map<std::string, SectionInfo*>();
    sections = new std::vector<SectionInfo*>();

    Elf32_Shdr* index0_header = new Elf32_Shdr(SectionHeaderTable_Entry0);
    // Add section header for undefined section
    SectionInfo* index0_section_info = new SectionInfo(index0_header, true);
    sections_map->emplace(std::make_pair("UND",index0_section_info));
    sections->push_back(index0_section_info);

    // Section header string table section header

    Elf32_Shdr* shstrtab_header = new Elf32_Shdr();
    // sh_name will hold index of string in string tatble, and not index of starting byte like in original Elf32 format
    shstrtab_header->sh_name = 0;     
    shstrtab_header->sh_type = SHT_STRTAB;
    // No flags are set
    shstrtab_header->sh_flags = 0;
    shstrtab_header->sh_addr = 0;
    // Not known right now, will be known when writing to the file
    shstrtab_header->sh_offset = 0;
    shstrtab_header->sh_size = 0;
    shstrtab_header->sh_link = SHN_UNDEF;
    shstrtab_header->sh_info = 0;

    SectionInfo* shstrtab_info = new SectionInfo(shstrtab_header, true);
    // Contents of SectionInfo structure for string tables will be empty, since we have separate vectors with their contents
    sections_map->emplace(std::make_pair("shstrtab", shstrtab_info));
    sections->push_back(shstrtab_info);

    // String table section header

    Elf32_Shdr* strtab_header = new Elf32_Shdr();
    strtab_header->sh_name = 1;
    strtab_header->sh_type = SHT_STRTAB;
    // No flags are set
    strtab_header->sh_flags = 0;
    strtab_header->sh_addr = 0;
    // Not know right now, will be known when writing to the file
    strtab_header->sh_offset = 0;
    strtab_header->sh_size = 0;
    strtab_header->sh_link = SHN_UNDEF;
    strtab_header->sh_info = 0;

    SectionInfo* strtab_info = new SectionInfo(strtab_header, true);
    // Contents of SectionInfo structure for string tables will be empty, since we have separate vectors with their contents
    sections_map->emplace(std::make_pair("strtab", strtab_info));
    sections->push_back(strtab_info);

    // Relocatable file has no program header table
    if ( file_type != ET_REL ) program_header_table = new std::vector<Elf32_Phdr*>();
    else program_header_table = nullptr;

    // TODO - Are relocation tables needed in executble files
    if ( file_type == ET_REL ) {
        // Empty for now
        relocation_tables = new std::unordered_map<std::string, std::vector<Elf32_Rela*>*>();
        // Relocatable file will definitely have symbol table, so we add it now
        shdr_string_table->push_back(".symtab");

        // Symbol table section header

        Elf32_Shdr* symtab_header = new Elf32_Shdr();
        symtab_header->sh_name = 2;
        symtab_header->sh_type = SHT_SYMTAB;
        // No flags are set
        symtab_header->sh_flags = 0;
        symtab_header->sh_addr = 0;
        // Not know right now, will be known when writing to the file
        symtab_header->sh_offset = 0;
        symtab_header->sh_size = 0;

        // TODO - For now i'll hardcode section header indexes in section header table
        symtab_header->sh_link = 2;
        // Not known right now
        symtab_header->sh_info = 0;

        SectionInfo* symtab_info = new SectionInfo(symtab_header, true);
        // Contents of SectionInfo structure for symbol table will be empty, since we have separate map with their contents
        sections_map->emplace(std::make_pair("symtab", symtab_info));
        sections->push_back(symtab_info);

        symbol_table_map = new std::unordered_map<std::string, Elf32_Sym*>();
        symbol_table = new std::vector<Elf32_Sym*>();

        // Add default entry into symbol table, it has no name, so we dont have to add it into string table
        Elf32_Sym* index0_sym = new Elf32_Sym(SymbolTable_Entry0);
        symbol_table_map->emplace(std::make_pair("UND", index0_sym));
        symbol_table->push_back(index0_sym);
    }

    
}



Elf32File::~Elf32File() {
    delete header;
    delete string_table;
    delete shdr_string_table;

    for ( auto section : *sections ) delete section;
    delete sections;
    delete sections_map;    // SectionInfo structures have already been freed

    if ( program_header_table ) {
        for( auto phdr : *program_header_table ) delete phdr;
        delete program_header_table;
    }

    if ( symbol_table ) {
        for ( auto entry : *symbol_table ) delete entry;
        delete symbol_table;
    }

    if ( symbol_table_map ) {
        delete symbol_table_map; 
    }

    if ( relocation_tables ) {
        for ( auto map_entry : *relocation_tables ) {
            for ( auto rela : *map_entry.second ) delete rela;
            delete map_entry.second; 
        }
        delete relocation_tables;
    }

}

int Elf32File::createSection(std::string section_name, Elf32_Word type, Elf32_Word flags, Elf32_Addr addr) {
    if ( sections_map->find(section_name) != sections_map->end() ) return -1;   // Section with given name already exists

    shdr_string_table->push_back("section_name");
    int st_index = shdr_string_table->size() - 1;

    Elf32_Shdr* section_header = new Elf32_Shdr();
    section_header->sh_name = st_index;
    section_header->sh_type = type;
    // No flags are set
    section_header->sh_flags = flags;
    section_header->sh_addr = addr;
    // Not know right now, will be known when writing to the file
    section_header->sh_offset = 0;
    section_header->sh_size = 0;

    // This function is used to create regular sections
    section_header->sh_link = SHN_UNDEF;
    section_header->sh_info = 0;

    SectionInfo* section_info = new SectionInfo(section_header);    // Not empty
    sections_map->emplace(std::make_pair(section_name, section_info));
    sections->push_back(section_info);

}