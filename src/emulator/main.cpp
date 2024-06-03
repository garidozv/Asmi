#include "../../inc/emulator/Emulator.hpp"

#include <iostream>


int main(int argc, char* argv[]) {
  if ( argc != 2 ) {
    std::cout << "usage: emulator <input-file>" << std::endl;
    exit(-1);
  }

  Emulator* emulator = Emulator::getInstance();

  emulator->setFileName((std::string)argv[1]);
  
  emulator->startEmulating();

  std::remove(argv[1]);

  return 0;
}