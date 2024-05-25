#ifndef LINKER_H
#define LINKER_H

#include <unordered_map>
#include <algorithm>
#include "../elf/Elf32File.hpp"

class Linker {

  // key = section_name
  // value = (start_address, section_size = 0)
  std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>* memory_map;
  std::vector<Elf32File*>* files;
  Elf32File* output_file;

  // If there are no mappings, starting address is 0
  uint32_t curr_address = 0;

protected:

  Linker();

  static Linker* linker;

public:

  Linker(Linker&) = delete;
  void operator=(const Linker&) = delete;
  static Linker* getInstance();

  void startLinking();
  bool addMapping(std::string section_name, uint32_t addr);
  void addFile(std::string file_name);

};



#endif