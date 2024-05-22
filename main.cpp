#include <stdio.h>
#include <string>
#include "lexer.hpp"
#include "parser.hpp"

int main() {
  
  yyin = fopen("./samples/nivo-a/math.s", "r");

  yyparse();

  fclose(yyin);

  return 0;
};