%{

#include <string>
#include <vector>
#include "helper.hpp"
#include <iostream>

extern int yylineno;

using namespace std;

int line_cnt = 0;
void yyerror (const char* s);
int  yylex ();

vector<string*>* labels = new vector<string*>();

%}

%output "parser.cpp"
%defines "parser.hpp"

%union {
  int num;
  std::string* sym;
}

%token GLOBAL
%token EXTERN
%token SECTION
%token WORD
%token SKIP
%token ASCII
%token EQU
%token END

%token HALT
%token INT
%token IRET
%token CALL
%token RET
%token JMP
%token BEQ
%token BNE
%token BGT
%token PUSH
%token POP
%token XCHG
%token ADD
%token SUB
%token MUL
%token DIV
%token NOT
%token AND
%token OR
%token XOR
%token SHL
%token SHR
%token LD
%token ST
%token CSRRD
%token CSRWR

%token GPR
%token CSR

%token EOL
%token PLUS
%token COL
%token PER
%token DOL
%token LSQB
%token RSQB
%token COMMA
%token<sym> STR
%token<sym> SYMBOLIC
%token<num> LITERAL
%token<num> IMMLIT
%token<sym> IMMSYM
%token<sym> LAB
%type<sym> symbolic_list
%type<sym> literal_list
%type<sym> list

/* TODO - change the way operands are recognized */

%%

input:
| line input;

line:
  LAB EOL { cout << "just a label" << endl << *$1 << endl; }
| LAB directive EOL { cout << "label and directive" << endl; }
| LAB instruction EOL { cout << "label and instruciton" << endl; }
| directive EOL { cout << "just a directive" << endl; }
| instruction EOL { cout << "just a isntruction" << endl; }
| EOL { line_cnt++; cout << "end" << endl; };

directive:
  GLOBAL symbolic_list { cout << *($2) << endl; }
| EXTERN symbolic_list { cout << *($2) << endl; }
| SECTION SYMBOLIC
| WORD list { cout << *($2) << endl; }
| SKIP LITERAL
| ASCII STR { cout << *($2) << endl; }
| EQU SYMBOLIC COMMA LITERAL /* expression here, not literal */ 
| END


instruction:
  HALT
| INT
| IRET
| RET
| CALL jmpcallop
| JMP jmpcallop
| PUSH GPR
| POP GPR
| NOT GPR
| XCHG GPR COMMA GPR
| ADD GPR COMMA GPR
| SUB GPR COMMA GPR
| MUL GPR COMMA GPR
| DIV GPR COMMA GPR
| AND GPR COMMA GPR
| OR GPR COMMA GPR
| XOR GPR COMMA GPR
| SHL GPR COMMA GPR
| SHR GPR COMMA GPR
| CSRRD CSR COMMA GPR
| CSRWR GPR COMMA CSR
| LD operand COMMA GPR
| ST GPR COMMA operand
| BEQ GPR COMMA GPR COMMA operand
| BNE GPR COMMA GPR COMMA operand
| BGT GPR COMMA GPR COMMA operand


/* CSR here as well? */
operand:
  IMMLIT
| IMMSYM
| GPR
| LSQB GPR RSQB
| LSQB GPR PLUS SYMBOLIC RSQB
| LSQB GPR PLUS LITERAL RSQB
| SYMBOLIC
| LITERAL

jmpcallop:
  SYMBOLIC
| LITERAL


list:
  symbolic_list { $$ = $1; }
| literal_list { $$ = $1; }

symbolic_list:
  SYMBOLIC { $$ = $1; }
| SYMBOLIC COMMA symbolic_list {
	$$ = Helper::concat_strings_with_comma($1, $3);
	delete $1;
	delete $3;
 }

literal_list:
  LITERAL { $$ = new string(to_string($1)); }
| LITERAL COMMA literal_list {
	string* temp = new string(to_string($1));
	$$ = Helper::concat_strings_with_comma(temp, $3);
	delete temp;
	delete $3;
}



%%


int main() {

  yyparse();

  printf("Number of lines is: %d\n", line_cnt);

  for ( int i = 0; i < labels->size(); i++ ) {
    cout << *((*labels)[i]) << endl;
    delete (*labels)[i];
  }

  delete labels;

  return 0;
}


void yyerror(const char* s) {
 cout << "ERROR: " << s << endl << "line: " << yylineno << endl;
}
