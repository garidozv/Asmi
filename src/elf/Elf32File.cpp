#include "../../inc/elf/Elf32File.hpp"

const std::string Elf32File::elf_sym_types[5] =  {
    "NOTYP",
    "OBJCT",
    "FUNC",
    "SCTN",
    "FILE"
};

const std::string Elf32File::elf_sym_binds[3] = {
    "LOC",
    "GLOB",
    "WEAK"
};

const std::string Elf32File::elf_rela_types[1] {
    "RELOC_32"
};

Elf32File::Elf32File(std::string file_name, Elf32_Half type, bool empty) {

    name = file_name;

    string_table = new std::vector<std::string>();
    sections = new Table<SectionInfo*>();
    symbol_table = new Table<Elf32_Sym*>();
    relocation_tables = new std::unordered_map<uint32_t, std::vector<Elf32_Rela*>*>();
    segments = new Table<SegmentInfo*>();

    if ( !empty ) {
        // Add section headers for symbol table and string table
        // Size 0 for now, will be set when symbol table is added
        Elf32_Shdr* symbol_table_header = createSectionHeader(".symtab", SHT_SYMTAB, 0, 0, 0, 0, 0);
        sections->put(".symtab", new SectionInfo(symbol_table_header, nullptr));
        // Size will be set at the end when writing to file
        Elf32_Shdr* string_table_header = createSectionHeader(".strtab", SHT_STRTAB, 0, 0, 0, 0, 0);
        sections->put(".strtab", new SectionInfo(string_table_header, nullptr));

        header = new Elf32_Ehdr();
        header->e_entry = 0;
        header->e_type = type;
        header->e_strndx = 1;
        //if ( type == ET_EXEC ) header->e_entry = 0x40000000
        // All the other fields will be known at the end when writing to file
    }
}

Elf32_Shdr* Elf32File::createSectionHeader(std::string name, Elf32_Word type, Elf32_Word addr, Elf32_Off offset, Elf32_Word size, Elf32_Word info, Elf32_Word link) {
    Elf32_Shdr* header = new Elf32_Shdr();
    header->sh_name = addString(name);
    header->sh_type = type;
    header->sh_addr = addr;
    header->sh_offset = offset;
    header->sh_size = size;
    header->sh_info = info;
    header->sh_link = link;
    return header;
}

Elf32_Phdr* Elf32File::createProgramHeader(Elf32_Word type, Elf32_Off offset, Elf32_Addr vaddr, Elf32_Word size) {
    Elf32_Phdr* header = new Elf32_Phdr();
    header->p_type = type;
    header->p_offset = offset;
    header->p_vaddr = vaddr;
    header->p_size = size;
    return header;
}

uint32_t Elf32File::addString(std::string string) {
    string_table->push_back(string);
    str_tab_size += string.length() + 1;
    return string_table->size() - 1;
}

void Elf32File::appendToSection(uint32_t index, std::vector<uint8_t>* cont) {
    std::vector<uint8_t>& contents = *getSectionContents(index);
    std::vector<uint8_t>& contents_to_append = *cont;

    for ( int i = 0; i < contents_to_append.size(); i++ ) {
        contents.push_back(contents_to_append[i]);
    }
    // This function does not update the size
}

void Elf32File::patchSectionContents(std::string section_name, uint32_t offset, uint32_t word) {
    std::vector<uint8_t>& contents_ref = *getSectionContents(getSectionIndex(section_name));

    for ( int i = 0; i < 4; i++) {
        // Data is stored in little endian, so we start from lowest byte
        unsigned char byte = word;
        word >>= 8;
        contents_ref[offset + i] = byte; 
    }
}

std::vector<Elf32_Rela*>* Elf32File::getRelocationTable(std::string section_name) {
    int rel_sec_ind = sections->getIndex(".rela." + section_name);
    if ( relocation_tables->find(rel_sec_ind) != relocation_tables->end() ) {
        return relocation_tables->at(rel_sec_ind);
    } else {
        std::vector<Elf32_Rela*>* reloc_table = new std::vector<Elf32_Rela*>();
        relocation_tables->emplace(std::make_pair(rel_sec_ind, reloc_table));
        return reloc_table;
    }
}


uint32_t Elf32File::addSymbol(std::string name, Elf32_Addr value, Elf32_Word size, unsigned char info, std::string section) {
    Elf32_Sym* symbol = new Elf32_Sym();
    symbol->st_name = addString(name);
    symbol->st_value = value;
    symbol->st_size = size;
    symbol->st_info = info;
    if ( section == "UND" ) symbol->st_shndx = 0;
    else if ( section == "ABS" ) symbol->st_shndx = -1;
    else symbol->st_shndx = getSectionIndex(section);
    symbol_table->put(name, symbol);

    // This function will also update the size of symbol table section(which is at index 0)
    sections->get(0)->header->sh_size += sizeof(Elf32_Sym);
    return symbol_table->size() - 1;
}


void Elf32File::addSymbolTable(Table<Symbol*>& sym_table) {
    for ( int i = 0; i < sym_table.size(); i++) {
        Symbol& as_sym = *sym_table.get(i);
        Elf32_Sym* symbol = new Elf32_Sym;
        symbol->st_name = addString(as_sym.name);
        symbol->st_size = 0; // For now symbols have unknown size
        symbol->st_value = as_sym.offset;
        symbol->st_info = ELF32_ST_INFO(as_sym.bind, ( (as_sym.section == i) && i != 0 ? STT_SECTION : STT_NOTYPE ));
        // This filed has to hold section header index of section that this symbol belongs to
        // Here, we will put the value given by assembler because sections have not been added yet
        // Will have to fix when they are added
        symbol->st_shndx = as_sym.section;    
        symbol_table->put(as_sym.name, symbol);
    }
    sections->get(0)->header->sh_size = sym_table.size() * sizeof(Elf32_Sym);
}


void Elf32File::addAssemblerSection(Symbol* section) {
    std::vector<uint8_t>* contents = section->contents;
    std::vector<Reloc_Entry*>* reloc_table = section->reloc_table;
    // Offset not know right now
    Elf32_Shdr* section_header = createSectionHeader(section->name, SHT_PROGBITS, 0, 0, contents->size(), 0, 0);
    SectionInfo* section_info = new SectionInfo(section_header, contents);
    //contents_size += section_header->sh_size;
    sections->put(section->name, section_info);

    int shndx = sections->size() - 1;   // Section header table of this section

    // We iterate through symbol table and fix the section header indexes of every symbol that belongs to this section
    // including the symbol that represents the section itself
    // TODO - assembler should do this the right way, so we dont have to fix it here 
    for ( int32_t i = 0; i < symbol_table->size(); i++ ) {
        Elf32_Sym& sym = *symbol_table->get(i);
        if ( sym.st_shndx == section->section )  sym.st_shndx = shndx;
    }

    Elf32_Shdr* rel_table_shdr = createSectionHeader(".rela." + section->name, SHT_RELA, 0, 0,
                                        reloc_table->size()*sizeof(Elf32_Rela), 1, sections->size() - 1);
    // symbol table is at entry 0 - so symbol table has to be first thing thats added into object of elf file
    sections->put(".rela." + section->name, new SectionInfo(rel_table_shdr, nullptr));
    // contents will be stored separately
    if ( reloc_table->size() > 0 ) {
        std::vector<Elf32_Rela*>* relocation_table = new std::vector<Elf32_Rela*>(); 
        for ( Reloc_Entry* reloc : *reloc_table ) {
            Elf32_Rela* entry = new Elf32_Rela();
            entry->r_addend = reloc->addend;
            entry->r_offset = reloc->offset;
            entry->r_info = ELF32_R_INFO(reloc->symbol, reloc->type);
            relocation_table->push_back(entry);
        }

        //rel_tabs_size += rel_table_shdr->sh_size;
        relocation_tables->emplace(std::make_pair(sections->size() - 1, relocation_table));
    }
    
}



void Elf32File::makeBinaryFile() {

    // First we need to set all the fields that have remained empty and are known now
    header->e_shnum = sections->size();
    header->e_phnum = segments->size();

    // Set size of string table section
    sections->get(1)->header->sh_size = str_tab_size;

    // Now we have to go through every section header and set section offsets
    Elf32_Off offset = sizeof(Elf32_Ehdr) + segments->size() * sizeof(Elf32_Phdr);
    for ( int32_t i = 0; i < sections->size(); i++ ) {
        SectionInfo* sec = sections->get(i);
        sec->header->sh_offset = offset;
        if ( sec->header->sh_type == SHT_SYMTAB ) offset += symbol_table->size() * sizeof(Elf32_Sym);
        else if ( sec->header->sh_type == SHT_STRTAB ) offset += str_tab_size;
        else if ( sec->header->sh_type == SHT_RELA ) {
            if ( sec->header->sh_size > 0 ) offset += relocation_tables->find(i)->second->size() * sizeof(Elf32_Rela);
        } else {
            offset += sec->contents->size();
        }
    }

    // Since section offsets have been set, we go through program headers and set their offsets based on their starting section
    for ( int32_t i = 0; i < segments->size(); i++ ) {
        segments->get(i)->header->p_offset = segments->get(i)->starting_section->sh_offset;
    }

    // Section header table is at offset: sizeof(header) + sizeof(PHT) + sizeof(sections)
    header->e_shoff = offset;

    // Write to file

    std::fstream file;
    file.open(name, std::ios::out | std::ios::binary );

    // Write header
    file.write(reinterpret_cast<const char*>(header), sizeof(Elf32_Ehdr));

    // Write program header table
    for ( int32_t i = 0; i < segments->size(); i++ ) {
        file.write(reinterpret_cast<const char*>(segments->get(i)->header), sizeof(Elf32_Phdr));
    }

    // Write sections
    for( int i = 0; i < sections->size(); i++ ) {
        SectionInfo* sec = sections->get(i);
        switch (sec->header->sh_type) {
        case SHT_SYMTAB: {
            for( int32_t i = 0; i < symbol_table->size(); i++ ) {
                file.write(reinterpret_cast<const char*>(symbol_table->get(i)), sizeof(Elf32_Sym));
            }
        break;
        }
        case SHT_STRTAB: {
            for ( std::string& str : *string_table ) {
                file << str << '\0';
            }
            break;
        }
        case SHT_RELA: {
            if ( sec->header->sh_size > 0 ) {
                for ( Elf32_Rela* rel : *relocation_tables->find(i)->second ) {
                    file.write(reinterpret_cast<const char*>(rel), sizeof(Elf32_Rela));
                }
            }
            break;
        }
        default: {
            std::vector<uint8_t>& contents = *sec->contents;
            for ( uint8_t byte : contents ) {
                file << byte;
            }
            break;
        }
        }
    }

    // Write section header table
    for( int32_t i = 0; i < sections->size(); i++ ) {
        file.write(reinterpret_cast<const char*>(sections->get(i)->header), sizeof(Elf32_Shdr));
    }

    file.close();
    
}

void Elf32File::readFromFile() {

    std::fstream file;
    file.open(name, std::ios::in | std::ios::binary);

    // Read file header
    header = new Elf32_Ehdr();
    file.read(reinterpret_cast<char*>(header), sizeof(Elf32_Ehdr));

    // Read string table, beacuse we will need strings when adding section headers
    // We know that string table's header is at index 1
    file.seekg(header->e_shoff + sizeof(Elf32_Shdr));
    Elf32_Shdr* str_tab_hdr = new Elf32_Shdr();
    file.read(reinterpret_cast<char*>(str_tab_hdr), sizeof(Elf32_Shdr));
    file.seekg(str_tab_hdr->sh_offset);
    uint32_t read_bytes = 0;
    while ( read_bytes < str_tab_hdr->sh_size ) {
        // Read null terminated string
        std::string str;
        char c;
        while (file.read(&c, sizeof(char)) && c != '\0') {
            str += c;
        }
        string_table->push_back(str);
        read_bytes += str.size() + 1;
    }

    file.seekg(header->e_shoff);
    // Read section header table
    for ( int i = 0; i < header->e_shnum; i++) {
        Elf32_Shdr* shdr = new Elf32_Shdr();
        file.read(reinterpret_cast<char*>(shdr), sizeof(Elf32_Shdr));
        sections->put(getString(shdr->sh_name), new SectionInfo(shdr, nullptr));
    }

    // Read sections

    for ( int i = 0; i < sections->size(); i++) {
        SectionInfo* sec = sections->get(i); 
        if ( sec->header->sh_size == 0 ) continue;
        file.seekg(sec->header->sh_offset);
        switch(sec->header->sh_type) {
            case SHT_SYMTAB: {
                for ( int j = 0; j < sec->header->sh_size / sizeof(Elf32_Sym); j++) {
                    Elf32_Sym* sym = new Elf32_Sym();
                    file.read(reinterpret_cast<char*>(sym), sizeof(Elf32_Sym));
                    symbol_table->put(getString(sym->st_name), sym);
                }
                break;
            }
            case SHT_STRTAB: break; // Already read
            case SHT_RELA: {
                std::vector<Elf32_Rela*>* reloc_table = new std::vector<Elf32_Rela*>();
                for ( int j = 0; j < sec->header->sh_size / sizeof(Elf32_Rela); j++) {
                    Elf32_Rela* rel = new Elf32_Rela();
                    file.read(reinterpret_cast<char*>(rel), sizeof(Elf32_Rela));
                    reloc_table->push_back(rel);
                }

                relocation_tables->emplace(std::make_pair(i, reloc_table));
                break;
            }
            default: {
                std::vector<uint8_t>* contents = new std::vector<uint8_t>();
                for ( int j = 0; j < sec->header->sh_size; j++) {
                    uint8_t byte;
                    file.read(reinterpret_cast<char*>(&byte), sizeof(uint8_t));
                    contents->push_back(byte);
                }

                sec->contents = contents;
                break;
            }
        }
    } 


    file.seekg(sizeof(Elf32_Ehdr));
    // Read program header table
    for ( int i = 0; i < header->e_phnum; i++) {
        Elf32_Phdr* phdr = new Elf32_Phdr();
        file.read(reinterpret_cast<char*>(phdr), sizeof(Elf32_Phdr));
        // We have to connect program headers with their sections
        // we will do this by finding the starting section using the offset
        std::vector<uint8_t>* contents = nullptr;
        Elf32_Shdr* starting_section = nullptr;

        for ( int32_t i = 0; i < sections->size(); i++ ) {
            SectionInfo* sec = sections->get(i);
            if ( sec->header->sh_offset == phdr->p_offset ) {
                contents = sec->contents;
                starting_section = sec->header;
                break;
            }
        }
        segments->put(getString(starting_section->sh_name), new SegmentInfo(phdr, contents, starting_section));
    }

    



    file.close();
}

void Elf32File::makeTextFile() {

    std::ofstream fout(name + ".readelf");

    fout << "Symbol table '.symtab' containts " << symbol_table->size() << " entries:\n";
    
    fout << std::left
         << std::setw(3)    << ""
         << std::setw(4)    << "Num"
         << std::setw(10)   << "Value"
         << std::setw(6)    << "Size"
         << std::setw(7)    << "Type"
         << std::setw(6)    << "Bind"
         << std::setw(5)    << "Ndx"
                            << "Name\n";

    for ( int i = 0; i < symbol_table->size(); i++) {
        Elf32_Sym& sym = *symbol_table->get(i);
        fout << std::setw(3) << ""
             << std::setw(2) << std::right << i << ": ";
        Helper::printHex(fout, sym.st_value, 8);
        fout << "  "
             << std::setw(4) << sym.st_size << "  " << std::left
             << std::setw(5) << elf_sym_types[ELF32_ST_TYPE(sym.st_info)] << "  "
             << std::setw(4) << elf_sym_binds[ELF32_ST_BIND(sym.st_info)] << "  " << std::right
             << std::setw(3) << (sym.st_shndx == 0 ? "UND" : ( sym.st_shndx == (Elf32_Half)-1 ? "ABS" : std::to_string(sym.st_shndx) ) ) << "  "
             << getString(sym.st_name) << '\n';
    }

    fout << '\n';

    for ( int i = 0; i < sections->size(); i++ ) {
        SectionInfo* sec = sections->get(i);
        if ( sec->contents == nullptr ) continue;

        fout << "Hex dump of section '" << getString(sec->header->sh_name) << "':\n";

        std::vector<uint8_t>& contents_ref = *sec->contents;
        for ( int j = 0; j < contents_ref.size(); j += 4 ) {
            if ( !(j % 16) ) {
                fout << std::setw(3) << "";
                Helper::printHex(fout, sec->header->sh_addr + j, 10, true);
                fout << ": ";
            }

            uint32_t word = (uint32_t)contents_ref[j+3] | ((uint32_t)contents_ref[j+2] << 8) | ((uint32_t)contents_ref[j+1] << 16) | ((uint32_t)contents_ref[j] << 24);
            Helper::printHex(fout, word, 8);

            if ( j != 0 && j != contents_ref.size() - 1 && !((j + 4) % 16) ) fout << '\n';
            else fout << " ";
        }

        fout << "\n\n";
    }

    for ( int i = 0; i < sections->size(); i++ ) {
        SectionInfo* sec = sections->get(i);
        if ( sec->header->sh_type != SHT_RELA || sec->header->sh_size == 0 ) continue;

        std::vector<Elf32_Rela*>& reloc_table_ref = *relocation_tables->find(i)->second; 

        fout << "Relocation section '" << getString(sec->header->sh_name) << "' contains " <<  reloc_table_ref.size() << (reloc_table_ref.size() == 1 ? " entry" : " entries" ) << ":\n";

        fout << std::left
             << std::setw(3)    << ""
             << std::setw(10)   << "Offset"
             << std::setw(10)   << "Type"
                                << "Value\n";

        for ( int j = 0; j < reloc_table_ref.size(); j++ ) {
            fout << std::setw(3) << "";
            Helper::printHex(fout, reloc_table_ref[j]->r_offset, 8);
            fout << "  "
                 << std::setw(8) << elf_rela_types[ELF32_R_TYPE(reloc_table_ref[j]->r_info)] << "  "
                 << getString(symbol_table->get(ELF32_R_SYM(reloc_table_ref[j]->r_info))->st_name) << " + ";
            Helper::printHex(fout, reloc_table_ref[j]->r_addend, 10, true);
            fout << '\n';
        }

        fout << "\n";
    }

    fout.close();
}

void Elf32File::makeHexDumpFile() {
    std::ofstream fout(name + ".hexdump");

    for ( int32_t i = 0; i < sections->size(); i++ ) {
        SectionInfo* sec = sections->get(i);
        if ( sec->contents == nullptr ) continue;

        std::vector<uint8_t>& contents_ref = *sec->contents;
        for ( int32_t j = 0; j < contents_ref.size(); j++ ) {
            if ( !(j % 8) ) {
                fout << std::setw(3) << "";
                Helper::printHex(fout, sec->header->sh_addr + j, 10);
                fout << ": ";
            }

            uint8_t byte = contents_ref[j];
            Helper::printHex(fout, byte, 2);

            if ( j != 0 && j != contents_ref.size() - 1 && !((j + 1) % 8) ) fout << '\n';
            else if ( ((j + 1) % 8) ) fout << " ";
        }

        if ( contents_ref.size() % 8 != 0 ) {
            for ( int32_t j = 0; j < 8 - (contents_ref.size() % 8); j++) {
                fout << "00";
                if ( j != 8 - (contents_ref.size() % 8) - 1 ) fout << " ";
            }
        }

        fout << "\n";
    }

    fout.close();

}


Elf32File::~Elf32File() {
    delete header;
    for ( int32_t i = 0; i < sections->size(); i++ ) {
        delete sections->get(i);
    }   
    for ( int32_t i = 0; i < symbol_table->size(); i++ ) {
        delete symbol_table->get(i);
    }
    for ( int32_t i = 0; i < segments->size(); i++ ) {
        delete segments->get(i);
    }  
    delete string_table;
    delete sections;
    delete symbol_table;
    delete segments;
    for ( auto entry : *relocation_tables) {
        for ( Elf32_Rela* rel : *entry.second ) {
            delete rel;
        }
        delete entry.second;
    }
    delete relocation_tables;

}