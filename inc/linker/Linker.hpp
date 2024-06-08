#ifndef LINKER_H
#define LINKER_H

#include <unordered_map>
#include <algorithm>
#include "../elf/Elf32File.hpp"

class Linker {

  // key = section_name
  // value = (start_address, section_size = 0)
  std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>* memory_map_p; // premapped
  std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>* memory_map; // rest of the sections
  std::vector<Elf32File*>* files;
  Elf32File* output_file;

  // If there are no mappings, starting address is 0
  uint32_t curr_address = 0;
  unsigned char file_type = ET_EXEC;  // REL or EXEC
  std::string file_name = "";

  void printError(std::string message) {
    std::cout << "linker: error : " << message << std::endl;
    exit(-1);
  }

  void mapSections();
  void updateSymbols();
  void resolveRelEntries();
  void updateRelEntries();

protected:

  Linker();

  static Linker* linker;

public:

  ~Linker();
  Linker(Linker&) = delete;
  void operator=(const Linker&) = delete;
  static Linker* getInstance();

  void startLinking();
  bool addMapping(std::string section_name, uint32_t addr);
  void addFile(std::string file_name);
  void setFileType(unsigned char type) { file_type = type; };
  void setFileName(std::string name) { file_name = name; };
};



#endif