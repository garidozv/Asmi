#include <stdio.h>
#include <string>
#include "lexer.hpp"
#include "parser.hpp"

int main() {
  
  yyin = fopen("./samples/nivo-a/main.s", "r");

  yyparse();

  fclose(yyin);

  return 0;
};