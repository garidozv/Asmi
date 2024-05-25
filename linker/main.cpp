
#include <string>
#include <iostream>
#include <unordered_map>
#include "../Helper.hpp"
#include "./Linker.hpp"

int main(int argc, char *argv[]) {

  std::string usage = "usage: linker [-place=<section-name>@<starting-address>...] <input-file>...";

  Linker* linker = Linker::getInstance();

  for ( int i = 1; i < argc; i++) {
    std::string temp = argv[i];
    if ( temp.substr(0, 7) == "-place=" ) {
      temp = temp.substr(7);
      std::vector<std::string> pair = Helper::splitString(temp, '@');
      if ( pair.size() == 2 ) { 
        linker->addMapping(pair[0], std::stoi(pair[1]));
      } else {
        std::cout << usage << std::endl;
        exit(-1);
      }
    } else {
      linker->addFile(temp);
    }
  }

  linker->startLinking();

  return 0;
}