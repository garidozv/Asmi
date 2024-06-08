#include "../../inc/linker/Linker.hpp"

Linker* Linker::linker = nullptr;

Linker* Linker::getInstance() {
  if ( linker == nullptr ) {
    linker = new Linker();
  }

  return linker;
}

Linker::Linker() {
  memory_map_p = new std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>();
  memory_map = new std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>();
  files = new std::vector<Elf32File*>();
}

bool Linker::addMapping(std::string section_name, uint32_t addr) {
  if ( memory_map_p->find(section_name) != memory_map_p->end() ) return false;
  memory_map_p->emplace(std::make_pair(section_name, std::make_pair(addr, 0)));
  return true;
}

void Linker::addFile(std::string file_name) {
  Elf32File* file = new Elf32File(file_name, ET_REL, true);
  file->readFromFile();
  files->push_back(file);
}


void Linker::startLinking() {
  output_file = new Elf32File(( file_name == "" ? ( file_type == ET_REL ? "output.o" : "output.hex") : file_name ), file_type);
  
  mapSections();
  updateSymbols();
  // If type is EXEC resolve relocation entries, if REL updated relocation entries add to output file
  if ( file_type == ET_EXEC ) resolveRelEntries();
  else updateRelEntries();

  output_file->makeBinaryFile();
  output_file->makeTextFile();
  //output_file->makeHexDumpFile();
}

void Linker::mapSections() {
  // First we have to see how much space will pre mapped sections take up, so we can know whats the starting addres
  // for all the other sections that were not mapepd via place option


  // If otuput file has to be relocatable, given mappings wont be used
  // For loop will still be used to initialize memory map with unique section names and caluclate their sizes
  // map will be used later to keep track of section offsets(inside of output file section with same name)
  if ( file_type == ET_REL ) memory_map_p->clear();

  for ( Elf32File* file : *files ) {
    for ( int32_t i = 0; i < file->getSymbolTable()->size(); i++) {
      Elf32_Sym& symbol = *file->getSymbolTable()->get(i);
      if ( ELF32_ST_TYPE(symbol.st_info) != STT_SECTION ) continue;

      std::string section_name = file->getString(symbol.st_name);
      if ( memory_map_p->find(section_name) != memory_map_p->end() ) {
        // Section with this name has been pre mapped
        // and then we 'append' this file's section by increasing the section size in mapping(second elment in value pair)
        Elf32_Shdr* section_header = file->getSectionHeader(symbol.st_shndx);
        // Size will be set in section header for now, and updated in symbol table later
        std::pair<uint32_t, uint32_t>& addr_size_pair = memory_map_p->at(section_name);
        //section_header->sh_addr = addr_size_pair.first + addr_size_pair.second;
        addr_size_pair.second += section_header->sh_size;
      } else {
        // Section has not been pre mapped, but we will still keep track of the size of the section with this name
        Elf32_Shdr* section_header = file->getSectionHeader(symbol.st_shndx);
        if ( memory_map->find(section_name) != memory_map->end() ) {         
          memory_map->at(section_name).second += section_header->sh_size;
        } else {
          memory_map->emplace(std::make_pair(section_name, std::make_pair(0, section_header->sh_size)));
        }
        // Address is not known right now, so we wont be updating the header
      }
    }
  }

  if ( file_type == ET_EXEC ) {

    // Check if any of the pre mapped sections has size euqal to 0(which means that it doesn't exist) and remove it
    bool changed;
    do {
      changed = false;
      for ( auto& mapping : *memory_map_p ) {
        if ( mapping.second.second == 0 ) {
          memory_map_p->erase(mapping.first);
          changed = true;
          break;
        }
      }
    } while(changed);

    // Now, we have to go through mappings and see if there are any overlappings,
    // and also, find the highest address from which we will be storing remaining sections

    // We will check for overlappings by sorting the list of (section.start_addr, section.end_addr)
    // We only do this if there is more than one mapped section
    if ( memory_map_p->size() > 1 ) {
      std::vector<std::pair<std::string, std::pair<uint32_t, uint32_t>>> mappings;
    
      for ( auto& mapping : *memory_map_p ) {
        mappings.push_back({mapping.first, {mapping.second.first, mapping.second.first + mapping.second.second}});
      }

      std::sort(mappings.begin(), mappings.end(), [](std::pair<std::string, std::pair<uint32_t, uint32_t>> a, std::pair<std::string, std::pair<uint32_t, uint32_t>> b) {
        return (a.second.first < b.second.first ? true : (a.second.first == b.second.first ? a.second.second <= b.second.second : false ));
      });

      for ( int i = 0; i < mappings.size(); i++ ) {
        if ( i != mappings.size() - 1 && mappings[i+1].second.first < mappings[i].second.second ) {
          // Error, overlapping sections
          std::cout << "linker: error: given section mappings cause overlapping between sections '"
                    << mappings[i].first << "' and '" << mappings[i+1].first << "'" << std::endl;
          exit(-1);
        }
      } 

      // No overlappings, we determine the starting address as the end address of the section that's mapped highest in memory
      curr_address = mappings[mappings.size() - 1].second.second; 
    } else if ( memory_map_p->size() == 1 ) {
      curr_address = memory_map_p->begin()->second.first + memory_map_p->begin()->second.second;
    }


    // Assign starting addresses to sections that have not been pre mapped

    /*
      When it comes to alignment the .skip and .ascii directives complicate things a bit since their sizes dont have to be 4B aligned
      For now, we will assume that these directives will only appear in data sections
      Alignment is only important for 
    */

    for ( auto& mapping : *memory_map ) {
      //curr_address = (curr_address + 3) & ~(3); // align address 
      mapping.second.first = curr_address;
      curr_address += mapping.second.second;
    }

    // We will move these sections to memory_map map because there are no differences between mappings in these maps anymore
    // and it will be easier to work with a single map

    for ( auto& mapping : *memory_map_p ) {
      memory_map->emplace(std::make_pair(mapping.first, mapping.second));
    }
  }

  // Add defualt symbol to output file symbol table
  output_file->addSymbol("undefined", 0, 0, ELF32_ST_INFO(STB_LOCAL,STT_NOTYPE), "UND");


  // Next, before we reset sizes, we have to populate the output file with needed section/program headers(depends on its type) and their contents
  // Every section(mapped ones) will have its own section/program header 
  // In our case each section will be mapped to one program header table segment

  for ( auto& mapping : *memory_map ) {
    std::vector<uint8_t>* contents = new std::vector<uint8_t>();
    // Every section will still have section header, even if we are making an executable file
    Elf32_Shdr* section_header = output_file->createSectionHeader(mapping.first, SHT_PROGBITS, mapping.second.first, 0, mapping.second.second, 0, 0);
    output_file->addSection(section_header, contents);

    // Only EXEC file will have program headers
    if ( file_type == ET_EXEC ) {
      Elf32_Phdr* program_header = output_file->createProgramHeader(PT_LOAD, 0, mapping.second.first, mapping.second.second);
      output_file->addSegment(program_header, contents, section_header); 
    } else {
      // Only REL file will have relocation entries
      // Here, we make section headers for relocation entries, tables will be made later if there is a need
      // Also, headers for relocation entries have to have their section sizes updated at some point
      Elf32_Shdr* reloc_header = output_file->createSectionHeader(".rela." + mapping.first, SHT_RELA, 0, 0, 0, 1, output_file->getNumberOfSections() - 1 );
      output_file->addSection(reloc_header, nullptr);
    }

    // We also have to add a symbol for each section
    output_file->addSymbol(mapping.first, mapping.second.first, 0, ELF32_ST_INFO(STB_LOCAL, STT_SECTION), mapping.first);
  }
  
  // We reset the size field because we don't need it anymore, but that field will be used to keep track of the current
  // address inside of the section
  // That address will be used when assigning the starting addreses of all the sections that it consists of
  // Also, since we already calculated the size of each section and assigned starting addreses based on that, we dont have to worry about overlapping
  for ( auto& mapping : *memory_map ) {
    mapping.second.second = 0;
  }
}

void Linker::updateSymbols() {
  // All sections have been mapped in memory, next we have to update all of the symbols belonging to those sections
  // and append section contents to the output file seciton
  
  for ( Elf32File* file : *files ) {
    for ( int32_t i = 0; i < file->getSymbolTable()->size(); i++) {
      Elf32_Sym& symbol = *file->getSymbolTable()->get(i);
      if ( ELF32_ST_TYPE(symbol.st_info) != STT_SECTION ) continue;

      std::string section_name = file->getString(symbol.st_name);
      // The mapping for section with this name has to exist, so we dont check for that
      std::pair<uint32_t, uint32_t>& addr_size_pair = memory_map->at(section_name);
      Elf32_Shdr* section_header = file->getSectionHeader(symbol.st_shndx);     
      
      // Set symbol value and section address in the header
      // Since we will be using address field of section header to update values of the symbols that belong to that section
      // we will have to update it as well
      section_header->sh_addr = addr_size_pair.first + addr_size_pair.second;
      symbol.st_value = section_header->sh_addr;
      
      // Update size, so the next seciton with same name has the correct start address
      addr_size_pair.second += section_header->sh_size;

      // Even in EXEC files, sections still exist(program headers represent segments which consist of section groups)
      output_file->appendToSection(output_file->getSectionIndex(section_name),
                                    file->getSectionContents(symbol.st_shndx));

    }
  }


  // All of the sections have been mapped and their corresponding symbols updated
  // Next we have to update symbol values of all the other symbols that belong to these sections
  // Local symbols wont be used, but will update their values as well just in case(if it is REL file they should probably be updated)
  // GNU linker keeps local symbols in output file's symbol table, we won't be doing this for now

  for ( Elf32File* file : *files ) {
    for ( int32_t i = 0; i < file->getSymbolTable()->size(); i++) {
      Elf32_Sym& symbol = *file->getSymbolTable()->get(i);
      if ( ELF32_ST_TYPE(symbol.st_info) == STT_SECTION || symbol.st_shndx == SHN_UNDEF ) continue;
      if ( file_type == ET_EXEC && ELF32_ST_BIND(symbol.st_info) == STB_LOCAL ) continue;
      // EXEC files won't keep local symbols in their symbol table(REL will)
      Elf32_Shdr* section_header;
      if ( symbol.st_shndx != (Elf32_Half)SHN_ABS ) {
        section_header = file->getSectionHeader(symbol.st_shndx);  
        symbol.st_value += section_header->sh_addr;
      }
      // As we do this, we will also be adding these symbols to output file.s symbol table
      // First we check if symbol was already defined(multiple symbol definitions)
      if ( output_file->getSymbol(file->getString(symbol.st_name)) != nullptr ) {
        printError("multiple definitions for symbol '" + file->getString(symbol.st_name) + "'");
      }
      output_file->addSymbol(file->getString(symbol.st_name), symbol.st_value,
        symbol.st_size, symbol.st_info, (symbol.st_shndx != (Elf32_Half)SHN_ABS ? file->getString(section_header->sh_name) : "ABS" ) );
    }
  }
}

void Linker::resolveRelEntries() {
  for ( Elf32File* file : *files ) {
      for ( int i = 2; i < file->getNumberOfSections(); i++) {
        Elf32_Shdr* header = file->getSectionHeader(i);
        if ( header->sh_type != SHT_RELA || header->sh_size == 0 ) continue;
        std::vector<Elf32_Rela*>& reloc_table_ref = *file->getRelocationTable(i);
        // Besides relocation table, we need the starting address 
        // as well as starting address of section with that name in output file, so we can calculate the index in section contents
        Elf32_Shdr* input_section_header = file->getSectionHeader(header->sh_link);
        std::string section_name = file->getString(input_section_header->sh_name); // Name of the section tha this relocation table belongs to
        Elf32_Shdr* output_section_header = output_file->getSectionHeader(section_name);
        uint32_t addr = input_section_header->sh_addr;
        uint32_t base_addr = output_section_header->sh_addr;
        // Offset that has to be added to every offset in relocation table
        uint32_t offset = addr - base_addr;

        for ( Elf32_Rela* reloc: reloc_table_ref) {
          Elf32_Sym& in_sym = *file->getSymbolTable()->get(ELF32_R_SYM(reloc->r_info));
          // First we get the symbol name from this file's symbol table
          std::string symbol_name = file->getString(in_sym.st_name);

          // We have to check if the symbol represents section, if it does, that means that we might have to update addend
          // because the sections offset might have changed if it was merged with another section of the same name

          if ( ELF32_ST_TYPE(in_sym.st_info) == STT_SECTION ) {
            // Calculate the offset from section with same name in output file
            Elf32_Shdr* input_rel_section_header = file->getSectionHeader(symbol_name);
            Elf32_Shdr* output_rel_section_header = output_file->getSectionHeader(symbol_name);
            uint32_t addend_offset = input_rel_section_header->sh_addr - output_rel_section_header->sh_addr;
            reloc->r_addend += addend_offset;
          }

          // Next, using the symbol name, we find symbol value in output file's symbol table
          Elf32_Sym* symbol = output_file->getSymbol(symbol_name);
          if ( symbol == nullptr ) {
            // Error, symbol has not been exported by any other file
            printError("undefined symbol '" + symbol_name + "'");
          }
          // Now, we have to patch resulting sections entry at offset + reloc.offset with symbols value(if RELOC_32)
          if ( ELF32_R_TYPE(reloc->r_info) == RELOC_32 ) {
            output_file->patchSectionContents(section_name, offset + reloc->r_offset, symbol->st_value + reloc->r_addend);
          }
        }
      }
    }
}

void Linker::updateRelEntries() {
  for ( Elf32File* file : *files ) {
      for ( int i = 2; i < file->getNumberOfSections(); i++) {
        Elf32_Shdr* header = file->getSectionHeader(i);
        if ( header->sh_type != SHT_RELA || header->sh_size == 0 ) continue;
        std::vector<Elf32_Rela*>& reloc_table_ref = *file->getRelocationTable(i);
        // Besides relocation table, we need to starting address 
        // as well as starting address of section with that name in output file, so we can determine the index in section contents
        Elf32_Shdr* input_section_header = file->getSectionHeader(header->sh_link);
        std::string section_name = file->getString(input_section_header->sh_name); // Name of the section tha this relocation table belongs to
        Elf32_Shdr* output_section_header = output_file->getSectionHeader(section_name);
        // In this case, offset will be the address itself(since base address is always 0)
        uint32_t offset = input_section_header->sh_addr;

        // We also need a reference to corresponding relocation table in output file
        std::vector<Elf32_Rela*>& output_reloc_table_ref = *output_file->getRelocationTable(section_name);
        Elf32_Shdr* reloc_section = output_file->getRelocSectionHeader(section_name);

        for ( Elf32_Rela* reloc: reloc_table_ref) {
          Elf32_Rela* new_reloc = new Elf32_Rela();
          new_reloc->r_addend = reloc->r_addend;

          Elf32_Sym* symbol = file->getSymbolTable()->get(ELF32_R_SYM(reloc->r_info));

          // We have to check if the symbol represents section, if it does, that means that we might have to update addend
          // because the sections offset might have changed if it was merged with another section of the same name

          if ( ELF32_ST_TYPE(symbol->st_info) == STT_SECTION ) {
            std::string symbol_name = file->getString(symbol->st_name);
            // Calculate the offset from section with same name in output file
            Elf32_Shdr* input_rel_section_header = file->getSectionHeader(symbol_name);
            Elf32_Shdr* output_rel_section_header = output_file->getSectionHeader(symbol_name);
            uint32_t addend_offset = input_rel_section_header->sh_addr - output_rel_section_header->sh_addr;
            reloc->r_addend += addend_offset;
          }

          uint32_t symbol_index = output_file->getSymbolIndex(file->getString(symbol->st_name));
          if ( symbol_index == -1 ) {
            // Undefined symbol, so we add it in symbol table(this is not error since this is REL file)
            symbol_index = output_file->addSymbol(file->getString(symbol->st_name), 0, 0, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), section_name);
          }
          new_reloc->r_info = ELF32_R_INFO(symbol_index, ELF32_R_TYPE(reloc->r_info));
          new_reloc->r_offset = reloc->r_offset + offset;

          output_reloc_table_ref.push_back(new_reloc);
          // Update the size of relocation section
          reloc_section->sh_size += sizeof(Elf32_Rela);
        }
      }
    }
}


Linker::~Linker() {
  delete memory_map_p;
  delete memory_map;
  for ( Elf32File* file : *files ) {
    delete file;
  }
  delete files;
}