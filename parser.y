%{

#include <string>
#include <vector>
#include "Helper.hpp"
#include "Assembler.hpp"
#include <iostream>

extern int yylineno;

using namespace std;

void yyerror (const char* s);
int  yylex ();

Assembler* assembler = Assembler::getInstance();

%}

%output "parser.cpp"
%defines "parser.hpp"

%union {
  int num;
  std::string* sym;
  struct Operand* op;
  struct Instruction* instr;
  struct Directive* dir;
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
%token<sym> GPR
%token<sym> CSR;
%type<sym> symbolic_list
%type<sym> literal_list
%type<sym> list
%type<op> jmpcallop
%type<op> operand
%type<instr> instruction
%type<dir> directive


%%

input: { assembler->end(); }
| line input;

line:
  LAB EOL {
	assembler->addLabel(*($1));
	delete $1;
}
| LAB directive EOL {
	assembler->addLabel(*($1));
	assembler->addDirective(*($2));
	delete $1;
	delete $2;
}
| LAB instruction EOL {
	assembler->addLabel(*($1));
	assembler->addInstruction(*($2));
	delete $1;
	delete $2;
}
| directive EOL {
	assembler->addDirective(*($1));
	delete $1;
}
| instruction EOL {
	assembler->addInstruction(*($1));
	delete $1;
}
| EOL { assembler->newLine(); };

directive:
  GLOBAL symbolic_list {
	struct Directive* dir = new struct Directive();
	dir->type = Types::GLOBAL;
	dir->symbol = *($2);
	delete $2;
	$$ = dir;
}
| EXTERN symbolic_list {
	struct Directive* dir = new struct Directive();
	dir->type = Types::EXTERN;
	dir->symbol = *($2);
	delete $2;
	$$ = dir;
}
| SECTION SYMBOLIC {
	struct Directive* dir = new struct Directive();
	dir->type = Types::SECTION;
	dir->symbol = *($2);
	delete $2;
	$$ = dir;
}
| WORD list {
	struct Directive* dir = new struct Directive();
	dir->type = Types::WORD;
	dir->symbol = *($2);
	delete $2;
	$$ = dir;
}
| SKIP LITERAL {
	struct Directive* dir = new struct Directive();
	dir->type = Types::SKIP;
	dir->literal = $2;
	$$ = dir;
}
| ASCII STR {
	struct Directive* dir = new struct Directive();
	dir->type = Types::ASCII;
	dir->symbol = *($2);
	delete $2;
	$$ = dir;
}
| EQU SYMBOLIC COMMA LITERAL {  /* expression here, not literal */
	struct Directive* dir = new struct Directive();
	dir->type = Types::EQU;
	dir->symbol = *($2);
	dir->literal = $4;
	delete $2;
	$$ = dir;
}
| END {
	struct Directive* dir = new struct Directive();
	dir->type = Types::END;
	$$ = dir;
}


instruction:
  HALT {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::HALT;
	$$ = instr;
}
| INT {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::INT;
	$$ = instr;
}
| IRET {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::IRET;
	$$ = instr;
}
| RET {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::RET;
	$$ = instr;
}
| CALL jmpcallop {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::CALL;
	instr->op = *($2);
	delete $2;
	$$ = instr;
}
| JMP jmpcallop {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::JMP;
	instr->op = *($2);
	delete $2;
	$$ = instr;
}
| PUSH GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::PUSH;
	instr->reg1 = Helper::parseReg(*($2));
	delete $2;
	$$ = instr;
}
| POP GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::POP;
	instr->reg1 = Helper::parseReg(*($2));
	delete $2;
	$$ = instr;
}
| NOT GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::NOT;
	instr->reg1 = Helper::parseReg(*($2));
	delete $2;
	$$ = instr;
}
| XCHG GPR COMMA GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::XCHG;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	delete $2;
	delete $4;
	$$ = instr;
}
| ADD GPR COMMA GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::ADD;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	delete $2;
	delete $4;
	$$ = instr;
}
| SUB GPR COMMA GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::SUB;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	delete $2;
	delete $4;
	$$ = instr;
}
| MUL GPR COMMA GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::MUL;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	delete $2;
	delete $4;
	$$ = instr;
}
| DIV GPR COMMA GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::DIV;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	delete $2;
	delete $4;
	$$ = instr;
}
| AND GPR COMMA GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::AND;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	delete $2;
	delete $4;
	$$ = instr;
}
| OR GPR COMMA GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::OR;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	delete $2;
	delete $4;
	$$ = instr;
}
| XOR GPR COMMA GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::XOR;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	delete $2;
	delete $4;
	$$ = instr;
}
| SHL GPR COMMA GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::SHL;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	delete $2;
	delete $4;
	$$ = instr;
}
| SHR GPR COMMA GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::SHR;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	delete $2;
	delete $4;
	$$ = instr;
}
| CSRRD CSR COMMA GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::CSRRD;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	delete $2;
	delete $4;
	$$ = instr;
}
| CSRWR GPR COMMA CSR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::CSRWR;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	delete $2;
	delete $4;
	$$ = instr;
}
| LD operand COMMA GPR {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::LD;
	instr->reg1 = Helper::parseReg(*($4));
	instr->op = *($2);
	delete $4;
	delete $2;
	$$ = instr;
}
| ST GPR COMMA operand {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::ST;
	instr->reg1 = Helper::parseReg(*($2));
	instr->op = *($4);
	delete $2;
	delete $4;
	$$ = instr;
}
| BEQ GPR COMMA GPR COMMA jmpcallop {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::BEQ;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	instr->op = *($6);
	delete $2;
	delete $4;
	delete $6;
	$$ = instr;
}
| BNE GPR COMMA GPR COMMA jmpcallop {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::BNE;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	instr->op = *($6);
	delete $2;
	delete $4;
	delete $6;
	$$ = instr;
}
| BGT GPR COMMA GPR COMMA jmpcallop {
	struct Instruction* instr = new struct Instruction();
	instr->type = Types::BGT;
	instr->reg1 = Helper::parseReg(*($2));
	instr->reg2 = Helper::parseReg(*($4));
	instr->op = *($6);
	delete $2;
	delete $4;
	delete $6;
	$$ = instr;
}


/* CSR here as well? */
operand:
  IMMLIT {
	struct Operand* op = new struct Operand();
	op->type = Types::LIT;
	op->literal = $1;
	$$ = op;
}
| IMMSYM {
	struct Operand* op = new struct Operand();
	op->type = Types::SYM;
	op->symbol = *($1);
	delete $1;
	$$ = op;
}
| GPR {
	struct Operand* op = new struct Operand();
	op->type = Types::REG;
	op->reg = Helper::parseReg(*($1));
	delete $1;
	$$ = op;
}
| LSQB GPR RSQB {
	struct Operand* op = new struct Operand();
	op->type = Types::REG_DIR;
	op->reg = Helper::parseReg(*($2));
	delete $2;
	$$ = op;
}
| LSQB GPR PLUS SYMBOLIC RSQB {
	struct Operand* op = new struct Operand();
	op->type = Types::REG_SYM;
	op->reg = Helper::parseReg(*($2));
	op->symbol = *($4);
	delete $2;
	delete $4;
	$$ = op;
}
| LSQB GPR PLUS LITERAL RSQB {
	struct Operand* op = new struct Operand();
	op->type = Types::REG_LIT;
	op->reg = Helper::parseReg(*($2));
	op->literal = $4;
	delete $2;
	$$ = op;
}
| SYMBOLIC {
	struct Operand* op = new struct Operand();
        op->type = Types::SYM_DIR;
        op->symbol = *($1);
        delete $1;
        $$ = op;
}
| LITERAL {
	struct Operand* op = new struct Operand();
        op->type = Types::LIT_DIR;
        op->literal = $1;
        $$ = op;
}

jmpcallop:
  SYMBOLIC {
	struct Operand* op = new struct Operand();
	op->type = Types::SYM_DIR;
	op->symbol = *($1);
	delete $1;
	$$ = op;
}
| LITERAL {
	struct Operand* op = new struct Operand();
	op->type = Types::LIT_DIR;
	op->literal = $1;
	$$ = op;
}


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

  
  return 0;
}


void yyerror(const char* s) {
 cout << "ERROR: " << s << endl << "line: " << yylineno << endl;
}
