#include <stdio.h>
#include <string>
#include <iostream>
#include "../../misc/lexer.hpp"
#include "../../misc/parser.hpp"
#include "../../inc/assembler/Assembler.hpp"

int main(int argc, char *argv[]) {

  if ( argc < 2 || argc != 4 || (std::string)argv[1] != "-o" ) {
    std::cout << "usage: assembler [-o <output-file-name>] <input-file>" << std::endl;
    exit(-1);
  } 

  if ( argc == 4 ) {
    Assembler::getInstance()->setOutputFileName(argv[2]);
  }

  Assembler::getInstance()->setInputFileName(argv[3]);
   
  //yyin = fopen("./samples/nivo-a/main.s", "r");
  yyin = fopen(argv[3], "r");

  yyparse();

  fclose(yyin);

  return 0;
};