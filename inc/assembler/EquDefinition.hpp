#ifndef EQUDEFINITION_H
#define EQUDEFINITION_H

#include <set>
#include <string>
#include "AssemblerDefs.hpp"


class EquDefinition {

  std::string symbol_name;
  Expression* expr;
  // Set of symbols used in this expression, will be used to check if expression can be caluclated(if all of the symbols used are defined)
  std::set<std::string> symbols;
  // If Equ symbol is relocatable, this field will hold the index of symbol relative to which it's relocatable, otherwise it will be -1  
  // The reason it is symbol and not section is because of extern symbols
  uint32_t rel_sym = -1;
  bool extern_flag = false;

public:

  EquDefinition(std::string name, Expression* expr);
  ~EquDefinition();

  // Returns true if symbols expression is computable
  bool computable();

  // Checks if the symbol, defined by the given expression is valid(constant or relocatable to one section)
  // If it is relocatable, the rel_section filed will store index of section relative to which it's relocatable
  // Should be called after it is determined that expression is computable
  bool valid();

  uint32_t getRelSymbol() const { return rel_sym; };
  bool getExternFlag() const { return extern_flag; };
  std::string getName() const { return symbol_name; };

  // Caluclates the expression 
  // If relocatable the value returned represents addend that should be used in relocation entries for this symbol
  // If constant the value returned is this symbols constant value
  // Should be called after it is determined that expression is valid, and used in combination with getRelSymbol to determine if it constant or relocatable
  int32_t caluclate();
};


#endif