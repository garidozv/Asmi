%{
 #include <string>
 #include "parser.hpp"
 #include "Helper.hpp"
 #include <cstdlib>
 #include <iostream>

 extern void yyerror(const char*);

%}

%option outfile="lexer.cpp" header-file="lexer.hpp"
%option noyywrap
%option yylineno

/* comments and labels */

COMM "#"[^\n\r]*
SYM [a-zA-Z_][a-zA-Z_0-9]*

/* directives */

GLOBAL \.global
EXTERN \.extern
SECTION \.section
WORD \.word
SKIP \.skip
ASCII \.ascii
EQU \.equ
END \.end

/* instructions */

HALT halt
INT int
IRET iret
CALL call
RET ret
JMP jmp
BEQ beq
BNE bne
BGT bgt
PUSH push
POP pop
XCHG xchg
ADD add
SUB sub
MUL mul
DIV div
NOT not
AND and
OR or
XOR xor
SHL shl
SHR shr
LD ld
ST st
CSRRD csrrd
CSRWR csrwr

/* ... */
HEX 0[xX][0-9A-Fa-f]+
DEC [0-9]+

GPR r[0-9]|r1[0-5]|sp|pc
CSR status|handler|cause

/* TODO - remove PER */

LSQB "["
RSQB "]"
PLUS "+"
MINUS "-"
COMMA ","
PER "%"
DOL "$"
COL ":"
WS [ \t]*
STR \".*\"|\'.*\'

%%
{WS} {}
{GLOBAL} { return GLOBAL; }
{EXTERN}  { return EXTERN; }
{SECTION}  { return SECTION; }
{WORD}  { return WORD; }
{SKIP}  { return SKIP; }
{ASCII}  { return ASCII; }
{EQU}  { return EQU; }
{END}  { return END; }
{HALT} { return HALT; }
{INT} { return INT; }
{IRET}  { return IRET; }
{CALL}  { return CALL; }
{RET}  { return RET; }
{JMP}  { return JMP; }
{BEQ}  { return BEQ; }
{BNE}  { return BNE; }
{BGT}  { return BGT; }
{PUSH}  { return PUSH; }
{POP}  { return POP; }
{XCHG}  { return XCHG; }
{ADD}  { return ADD; }
{SUB}  { return SUB; }
{MUL}  { return MUL; }
{DIV}  { return DIV; }
{NOT}  { return NOT; }
{AND}  { return AND; }
{OR}  { return OR; }
{XOR}  { return XOR; }
{SHL}  { return SHL; }
{SHR}  { return SHR; }
{LD}  { return LD; }
{ST}  { return ST; }
{CSRRD}  { return CSRRD; }
{CSRWR}  { return CSRWR; }
{PER}{GPR} { yylval.sym = Helper::make_string(yytext + 1); return GPR; }
{PER}{CSR} { yylval.sym = Helper::make_string(yytext + 1); return CSR; }
{LSQB} { return LSQB; }
{RSQB} { return RSQB; }
{SYM}{COL} { yylval.sym = Helper::make_string(yytext); yylval.sym->pop_back(); return LAB; }
{COMMA} { return COMMA; }
{PLUS} { return PLUS; }
{MINUS} { return MINUS; }
{DOL}{HEX} { yylval.num = strtol(yytext + 1, NULL, 16); return IMMLIT; }
{DOL}{DEC} { yylval.num = strtol(yytext + 1, NULL, 10); return IMMLIT; }
{DOL}{SYM} { yylval.sym = Helper::make_string(yytext+1); return IMMSYM; }
{HEX} { yylval.num = strtol(yytext, NULL, 16); return LITERAL; }
{DEC} { yylval.num = strtol(yytext, NULL, 10); return LITERAL; }
{SYM} { yylval.sym = Helper::make_string(yytext); return SYMBOLIC; }
{STR} { yylval.sym = Helper::make_string(yytext + 1); yylval.sym->pop_back(); return STR; }
{COMM} {}
\n { return EOL; }
. {
	std::string errorMsg = "Unexpected token: " + (std::string)yytext;
	yyerror(errorMsg.c_str()); exit(-1);
}

%%

