#include <stdio.h>
#include <string>
#include "lexer.hpp"
#include "parser.hpp"

int main() {
  
  yyin = fopen("./sample2.txt", "r");

  yyparse();

  fclose(yyin);

  return 0;
};