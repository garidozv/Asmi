#include "EquDefinition.hpp"
#include "Assembler.hpp"


EquDefinition::EquDefinition(std::string name, Expression* expr) {
    this->expr = expr;
    this->symbol_name = name;
    
    Expression* temp = expr;
    while (temp) {
      if ( !std::isdigit(temp->symbol[0]) ) {
        symbols.insert(temp->symbol);
      }
      temp = temp->next;
    }
  } 

  EquDefinition::~EquDefinition() {
    Expression* curr = expr, *next = nullptr;
    while ( curr ) {
        next = curr->next;
        delete curr;
        curr = next;
    }
  }

  bool EquDefinition::computable() {
    Assembler* assembler = Assembler::getInstance();
    std::vector<Symbol>& symbol_table_ref = *assembler->symbol_table;

    for ( auto sym : symbols ) {
      if ( !symbol_table_ref[assembler->findSymbol(sym)].defined ) return false;
    }

    return true;
  }

  bool EquDefinition::valid() {
    Assembler* assembler = Assembler::getInstance();
    std::unordered_map<uint32_t, int32_t> map;
    std::vector<Symbol>& symbol_table_ref = *assembler->symbol_table;

    uint32_t extern_symbol = -1;

    Expression* temp = expr;
    while(temp) {
      if ( !std::isdigit(temp->symbol[0]) ) {
        Symbol& sym = symbol_table_ref[assembler->findSymbol(temp->symbol)];

        if ( sym.section != -1 || sym.rel != -1 ) {    // Symbols with ABS(-1) section and rel set to -1, are constant equ defined symbols, so we skip them
          uint32_t section = sym.section;

          // Relocatable Equ defined symbols have the symbol in relation to which they are relocatable in rel field
          if ( sym.section == -1 ) {
            section = symbol_table_ref[sym.rel].section;
          }

          // Only one extern symbol can be part of expression(TODO - this wont allow ext - ext which doesn't make sense but is valid)
          if ( section == 0 && map[section] != 0 ) return false;

          // If first(and only) extern symbol, save it, so it can be saved in rel_sym
          if ( section == 0 ) {
            if ( sym.section == -1 ) extern_symbol = sym.rel;
            else extern_symbol = assembler->findSymbol(sym.name);
          }

          if ( temp->sign == Types::PLUS ) map[section]++;
          else map[section]--;
        }
      }

      temp = temp->next;
    }

    // Expression is valid only if every map key has value of 0, or if there is only one with value of 1
    int cnt = 0, val, sec = -1;

    for ( auto m : map ) {
      if ( m.second != 0 ) {
        cnt++;
        val = m.second;
        sec = m.first;
      }
    }

    if ( cnt == 0 || cnt == 1 && val == 1 ) {
      // Relocatable in relation to extern symbol, so we have to put it's index in rel_sym
      if ( sec == 0 ) {
        rel_sym = extern_symbol;
        // Equ symbol relocatable relative to extern symbol can not be global, so we have to check
        int entry = assembler->findSymbol(symbol_name);
        if ( entry > 0 && symbol_table_ref[entry].bind == STB_GLOBAL ) {
          assembler->printError("Equ symbol relocatable relative to extern symbol can not be global");
        }
      }
      else rel_sym = sec;
      return true;
    }
    return false;
  }

  int32_t EquDefinition::caluclate() {
    Assembler* assembler = Assembler::getInstance();
    std::vector<Symbol>& symbol_table_ref = *assembler->symbol_table;

    int32_t value = 0;
    Expression* temp = expr;
    
    while(temp) {
      if ( std::isdigit(temp->symbol[0]) ) {
        value += std::stoi(temp->symbol);
      } else {
        Symbol& sym = symbol_table_ref[assembler->findSymbol(temp->symbol)];

        if ( temp->sign == Types::PLUS ) value += sym.offset;
        else value -= sym.offset;
      }

      temp = temp->next;
    }
    
    return value;
  }