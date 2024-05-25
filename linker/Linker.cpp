#include "Linker.hpp"


Linker* Linker::getInstance() {
  if ( linker == nullptr ) {
    linker = new Linker();
  }

  return linker;
}

Linker::Linker() {
  memory_map = new std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>();
  files = new std::vector<Elf32File*>();
  // TODO - add support for RELOC
  output_file = new Elf32File("output.hex", ET_EXEC);
}

bool Linker::addMapping(std::string section_name, uint32_t addr) {
  if ( memory_map->find(section_name) != memory_map->end() ) return false;
  memory_map->emplace(std::make_pair(section_name, std::make_pair(addr, 0)));
  return true;
}

void Linker::addFile(std::string file_name) {
  Elf32File* file = new Elf32File(file_name, ET_REL, true);
  file->readFromFile();
  files->push_back(file);
}


void Linker::startLinking() {
  // First we have to see how much space will pre mapped sections take up, so we can know whats the starting addres
  // for all the other sections that were not mapepd via place option

  // For now I'll iterate through symbol table of each file to find these, TODO - map symbol names to the entries

  for ( Elf32File* file : *files ) {
    std::vector<Elf32_Sym*>& symbol_table_ref = *file->getSymbolTable();
    std::vector<std::string>& string_table_ref = *file->getStringTable();
    for ( Elf32_Sym* symbol : symbol_table_ref ) {
      if ( ELF32_ST_TYPE(symbol->st_info) != STT_SECTION ) continue;

      std::string section_name = string_table_ref[symbol->st_name];
      if ( memory_map->find(section_name) != memory_map->end() ) {
        // Section with this name has been pre mapped
        // We first assign current end address of the section(from mapping) to this section
        // and then we 'append' this file's section by increasing the section size in mapping(second elment in value pair)
        Elf32_Shdr* section_header = file->getSectionHeader(symbol->st_shndx);
        // Size will be set in section header for now, and updated in symbol table later
        std::pair<uint32_t, uint32_t>& addr_size_pair = memory_map->at(section_name);
        section_header->sh_addr = addr_size_pair.first + addr_size_pair.second;
        addr_size_pair.second += section_header->sh_size;
      }
    }
  }

  // Now, we have to go through mappings and see if there are any overlappings,
  // and also, find the highest address from which we will be storing remaining sections

  // We will check for overlappings by sorting the list of (section.start_addr, section.end_addr)
  // We only do this if there is more than one mapped section
  if ( memory_map->size() > 1 ) {
    std::vector<std::pair<uint32_t, uint32_t>> mappings;
  
    for ( auto mapping : *memory_map ) {
      mappings.push_back({mapping.second.first, mapping.second.first + mapping.second.second});
    }

    std::sort(mappings.begin(), mappings.end(), [](std::pair<uint32_t, uint32_t> a, std::pair<uint32_t, uint32_t> b) {
      return (a.first > b.first ? true : (a.first == b.first ? a.second >= b.second : false ));
    });

    for ( int i = 0; i < mappings.size(); i++ ) {
      if ( i != mappings.size() - 1 && mappings[i+1].first < mappings[i].first + mappings[i].second ) {
        // Error, overlapping sections
        std::cout << "linker: error: give section mappings cause section overlapping in memory" << std::endl;
        exit(-1);
      }
    } 

    // No overlappings, we determine the starting address as the end address of the section thats mapped highest in memory
    curr_address = mappings[mappings.size() - 1].first + mappings[mappings.size() - 1].second; 
  }

  
  // Now we have to map all the remaining sections to memory
  
}