#include "Emulator.hpp"

#include <iostream>


int main(int argc, char* argv[]) {
  if ( argc != 2 ) {
    std::cout << "usage: emulator <input-file>" << std::endl;
    exit(-1);
  }

  Emulator* emulator = Emulator::getInstance();

  emulator->setFileName((std::string)argv[1]);
  
  emulator->startEmulating();

  return 0;
}