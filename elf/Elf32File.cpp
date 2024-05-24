#include "Elf32File.hpp"

Elf32File::Elf32File(std::string file_name, Elf32_Half type, bool empty) {

    name = file_name;

    string_table = new std::vector<std::string>();
    sections = new std::vector<SectionInfo*>();
    symbol_table = new std::vector<Elf32_Sym*>();
    relocation_tables = new std::unordered_map<uint32_t, std::vector<Elf32_Rela*>*>();
    program_header_table = new std::vector<Elf32_Phdr*>();

    if ( !empty ) {
        // Add section headers for symbol table and string table
        // Size 0 for now, will be set when symbol table is added
        Elf32_Shdr* symbol_table_header = createSectionHeader(".symtab", SHT_SYMTAB, 0, 0, 0, 0, 0);
        sections->push_back(new SectionInfo(symbol_table_header, nullptr));
        // Size will be set at the end when writing to file
        Elf32_Shdr* string_table_header = createSectionHeader(".strtab", SHT_STRTAB, 0, 0, 0, 0, 0);
        sections->push_back(new SectionInfo(string_table_header, nullptr));

        header = new Elf32_Ehdr();
        header->e_entry = 0;
        header->e_type = type;
        header->e_strndx = 1;
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

uint32_t Elf32File::addString(std::string string) {
    string_table->push_back(string);
    str_tab_size += string.length() + 1;
    return string_table->size() - 1;
}


void Elf32File::addSymbolTable(std::vector<Symbol>* sym_table) {
    std::vector<Symbol>& sym_table_ref = *sym_table;
    for ( int i = 0; i < sym_table_ref.size(); i++) {
        Elf32_Sym* symbol = new Elf32_Sym;
        symbol->st_name = addString(sym_table_ref[i].name);
        symbol->st_size = 0; // For now symbols have unknown size
        symbol->st_value = sym_table_ref[i].offset;
        symbol->st_info = ELF32_ST_INFO(sym_table_ref[i].bind, ( (sym_table_ref[i].section == i) && i != 0 ? STT_SECTION : STT_NOTYPE ));
        symbol->st_shndx = sym_table_ref[i].section;
        symbol_table->push_back(symbol);
    }
    sections->at(0)->header->sh_size = sym_table->size() * sizeof(Elf32_Sym);
}

// TODO - this is only for code sections because of type
void Elf32File::addAssemblerSection(Symbol section) {
    std::vector<uint8_t>* contents = section.contents;
    std::vector<Reloc_Entry*>* reloc_table = section.reloc_table;
    // Offset not know right now
    Elf32_Shdr* section_header = createSectionHeader(section.name, SHT_PROGBITS, 0, 0, contents->size(), 0, 0);
    SectionInfo* section_info = new SectionInfo(section_header, contents);
    //contents_size += section_header->sh_size;
    sections->push_back(section_info);

    Elf32_Shdr* rel_table_shdr = createSectionHeader(".rela." + section.name, SHT_RELA, 0, 0,
                                        reloc_table->size()*sizeof(Elf32_Rela), 1, sections->size() - 1);
    // symbol table is at entry 0 - so symbol table has to be first thing thats added into object of elf file
    sections->push_back(new SectionInfo(rel_table_shdr, nullptr));
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



bool Elf32File::makeBinaryFile() {
    if ( header->e_type == ET_REL ) {
        // First we need to set all the fields that have remained empty and are known now
        header->e_shnum = sections->size();
        header->e_phnum = program_header_table->size();

        // Set size of string table section
        sections->at(1)->header->sh_size = str_tab_size;
    
        // Now we have to go through every section header and set section offsets
        Elf32_Off offset = sizeof(Elf32_Ehdr);
        for ( int i = 0; i < sections->size(); i++ ) {
            SectionInfo* sec = sections->at(i);
            sec->header->sh_offset = offset;
            if ( sec->header->sh_type == SHT_SYMTAB ) offset += symbol_table->size() * sizeof(Elf32_Sym);
            else if ( sec->header->sh_type == SHT_STRTAB ) offset += str_tab_size;
            else if ( sec->header->sh_type == SHT_RELA ) {
                if ( sec->header->sh_size > 0 ) offset += relocation_tables->find(i)->second->size() * sizeof(Elf32_Rela);
            } else {
                offset += sec->contents->size();
            }
        }
        
        // Section header table is at offset: sizeof(header) + sizeof(PHT)(= 0 in relocatable file) + sizeof(sections)
        header->e_shoff = offset;

        // Write to file

        std::fstream file;
        file.open(name, std::ios::out | std::ios::binary );

        // Write header
        file.write(reinterpret_cast<const char*>(header), sizeof(Elf32_Ehdr));

        // Write sections
        for( int i = 0; i < sections->size(); i++ ) {
            SectionInfo* sec = sections->at(i);
            switch (sec->header->sh_type) {
            case SHT_SYMTAB: {
                for ( Elf32_Sym* sym : *symbol_table ) {
                    file.write(reinterpret_cast<const char*>(sym), sizeof(Elf32_Sym));
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
        for ( SectionInfo* sec : *sections ) {
            file.write(reinterpret_cast<const char*>(sec->header), sizeof(Elf32_Shdr));
        }

        file.close();
    } else {

    }
    

    // TODO - check for errors and fix return
    return true;
}

bool Elf32File::readFromFile() {

    std::fstream file;
    file.open(name, std::ios::in | std::ios::binary);

    // Read file header
    header = new Elf32_Ehdr();
    file.read(reinterpret_cast<char*>(header), sizeof(Elf32_Ehdr));

    file.seekg(header->e_shoff);
    // Read section header table
    for ( int i = 0; i < header->e_shnum; i++) {
        Elf32_Shdr* shdr = new Elf32_Shdr();
        file.read(reinterpret_cast<char*>(shdr), sizeof(Elf32_Shdr));
        sections->push_back(new SectionInfo(shdr, nullptr));
    }

    // Read sections

    for ( int i = 0; i < sections->size(); i++) {
        SectionInfo* sec = sections->at(i); 
        if ( sec->header->sh_size == 0 ) continue;
        file.seekg(sec->header->sh_offset);
        switch(sec->header->sh_type) {
            case SHT_SYMTAB: {
                for ( int j = 0; j < sec->header->sh_size / sizeof(Elf32_Sym); j++) {
                    Elf32_Sym* sym = new Elf32_Sym();
                    file.read(reinterpret_cast<char*>(sym), sizeof(Elf32_Sym));
                    symbol_table->push_back(sym);
                }
                break;
            }
            case SHT_STRTAB: {            
                uint32_t read_bytes = 0;
                while ( read_bytes < sec->header->sh_size ) {
                    // Read null terminated string
                    std::string str;
                    char c;
                    while (file.read(&c, sizeof(char)) && c != '\0') {
                        str += c;
                    }
                    string_table->push_back(str);
                    read_bytes += str.size() + 1;
                }
               
                break;
            }
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

    file.close();

    // TODO -
    return true;
}

void Elf32File::makeTextFile() {

    std::ofstream fout(name + ".readelf");
    
    std::vector<Elf32_Sym*>& symbol_table_ref = *symbol_table;

    fout << "Symbol table '.symtab' containts " << symbol_table_ref.size() << " entries:\n";
    
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
        fout << std::setw(3) << ""
             << std::setw(2) << std::right << i << ": ";
        printHex(fout, symbol_table_ref[i]->st_value, 8);
        fout << "  "
             << std::setw(4) << symbol_table_ref[i]->st_size << "  " << std::left
             << std::setw(5) << elf_sym_types[ELF32_ST_TYPE(symbol_table_ref[i]->st_info)] << "  "
             << std::setw(4) << elf_sym_binds[ELF32_ST_BIND(symbol_table_ref[i]->st_info)] << "  " << std::right
             << std::setw(3) << (symbol_table_ref[i]->st_shndx == 0 ? "UND" : ( symbol_table_ref[i]->st_shndx == -1 ? "ABS" : std::to_string(symbol_table_ref[i]->st_shndx) ) ) << "  "
             << string_table->at(symbol_table_ref[i]->st_name) << '\n';
    }

    fout << '\n';

    for ( int i = 0; i < sections->size(); i++ ) {
        SectionInfo* sec = sections->at(i);
        if ( sec->contents == nullptr ) continue;

        fout << "Hex dump of section '" << string_table->at(sec->header->sh_name) << "':\n";

        std::vector<uint8_t>& contents_ref = *sec->contents;
        for ( int j = 0; j < contents_ref.size(); j += 4 ) {
            if ( !(j % 16) ) {
                fout << std::setw(3) << "";
                printHex(fout, j, 10, true);
                fout << ": ";
            }

            uint32_t word = (uint32_t)contents_ref[j] | ((uint32_t)contents_ref[j+1] << 8) | ((uint32_t)contents_ref[j+2] << 16) | ((uint32_t)contents_ref[j+3] << 24);
            printHex(fout, word, 8);

            if ( j != 0 && j != contents_ref.size() - 1 && !((j + 4) % 16) ) fout << '\n';
            else fout << " ";
        }

        fout << "\n\n";
    }

    for ( int i = 0; i < sections->size(); i++ ) {
        SectionInfo* sec = sections->at(i);
        if ( sec->header->sh_type != SHT_RELA || sec->header->sh_size == 0 ) continue;

        std::vector<Elf32_Rela*>& reloc_table_ref = *relocation_tables->find(i)->second; 

        fout << "Relocation section '" << string_table->at(sec->header->sh_name) << "' contains " <<  reloc_table_ref.size() << (reloc_table_ref.size() == 1 ? " entry" : " entries" ) << ":\n";

        fout << std::left
             << std::setw(3)    << ""
             << std::setw(10)   << "Offset"
             << std::setw(10)   << "Type"
                                << "Value\n";

        for ( int j = 0; j < reloc_table_ref.size(); j++ ) {
            fout << std::setw(3) << "";
            printHex(fout, reloc_table_ref[j]->r_offset, 8);
            fout << "  "
                 << std::setw(8) << elf_rela_types[ELF32_R_TYPE(reloc_table_ref[j]->r_info)] << "  "
                 << string_table->at(symbol_table_ref[ELF32_R_SYM(reloc_table_ref[j]->r_info)]->st_name) << " + ";
            printHex(fout, reloc_table_ref[j]->r_addend, 10, true);
            fout << '\n';
        }

        fout << "\n";
    }

    fout.close();
}


Elf32File::~Elf32File() {
    delete header;
    for ( SectionInfo* sec : *sections ) {
        delete sec;
    }
    delete string_table;
    for ( Elf32_Sym* sym : *symbol_table ) {
        delete sym;
    }
    delete symbol_table;
    for ( Elf32_Phdr* phdr : *program_header_table ) {
        delete phdr;
    }
    delete program_header_table;
    for ( auto entry : *relocation_tables) {
        for ( Elf32_Rela* rel : *entry.second ) {
            delete rel;
        }
        delete entry.second;
    }
    delete relocation_tables;

}