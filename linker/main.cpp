
#include <string>
#include <iostream>
#include <unordered_map>
#include "../Helper.hpp"
#include "./Linker.hpp"

int main(int argc, char *argv[]) {

  std::string usage = "usage: linker [options] <input-file>... \
      \n\noptions:\n -o <output-file-name>\n -place=<section-name>@<address>\n -hex\n -relocatable";

  Linker* linker = Linker::getInstance();

  bool relocatable = false;
  bool hex = false;

  for ( int i = 1; i < argc; i++) {
    std::string temp = argv[i];
    if ( temp == "-relocatable" ) relocatable = true;
    else if ( temp == "-hex" ) hex = true;
    else if ( temp.substr(0, 7) == "-place=" ) {
      temp = temp.substr(7);
      std::vector<std::string> pair = Helper::splitString(temp, '@');
      if ( pair.size() == 2 ) { 
        linker->addMapping(pair[0], std::strtol(pair[1].c_str(), NULL, 16));
      } else {
        std::cout << usage << std::endl;
        exit(-1);
      }
    } else if ( temp == "-o" ) {
      if ( i == argc - 1 ) {
        std::cout << usage << std::endl;
        exit(-1);
      } else {
        linker->setFileName((std::string)argv[i+1]);
        i++;
      }
    } else {
      linker->addFile(temp);
    }
  }

  if ( hex && relocatable ) {
    std::cout << "linker: error : options '-hex' and '-relocatable' are mutually exclusive" << std::endl;
    exit(-1);
  }

  if ( !hex && !relocatable ) {
    exit(0);
  }

  if ( relocatable ) {
    linker->setFileType(ET_REL);
  }

  linker->startLinking();

  return 0;
}