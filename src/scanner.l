%{


#include "ConSymbolTable.hpp"
#include "lexical_analyzer.h"
#include <stdio.h>
#include "VarSymbolTable.hpp"

static int curr_lineno = 1;

void PrintCon(char * token,int radix){
    int index = con(token,radix);
    int value = getLastConstant();
   printf("#%d ( %d , %d ,Con_%d)\n",curr_lineno, ID_CONSTANT, value,index); // constant
}

void PrintDouble(char * token){
    int index = conDouble(token);
    double value = getLastDoubleConstant();
    printf("#%d ( %d , %f ,Con_%d)\n",curr_lineno, ID_CONSTANT, value,index); // constant
}

void PrintVar(char * token){
   printf("#%d, ( %d , %s ,Var_%d)\n",curr_lineno, ID_CONSTANT, token,var(token)); // constant
}

void PrintID(int id){
   printf("#%d ( %d , - )\n",curr_lineno, id);
}

void PrintError(const char *s){
    printf("#%d, ERROR: %s\n", curr_lineno, s);
}

%}

%option noyywrap


/*似乎并没有下划线。。？*/

IDENTIFIER [a-zA-Z][a-zA-Z0-9]*
INT_HEX 0[xX][0-9a-fA-F]+
INT_OCT 0[1-7][0-7]*
INT_BIN 0[bB][0-1]*
INT_DEC [1-9][0-9]*
FLOAT_DEC [1-9][0-9]*\.[0-9]+
ZERO 0



%x COMMENT
%x INLINE_COMMENT

%%

    /* deal with the comment. */

<INITIAL>"/*" {
    BEGIN(COMMENT);
}

<COMMENT>{
  "*/"      {
    BEGIN(INITIAL);
  }
  \n        {
        curr_lineno++;
    }
  [^*\n]+   {
   }
  [/*]     {  }
    
  <<EOF>>   {
    PrintError("comment not closed");
    BEGIN(INITIAL);
  }
}

<INITIAL>"//" {
  BEGIN(INLINE_COMMENT);
}

<INLINE_COMMENT>{
  \n {
    curr_lineno++;
    BEGIN(INITIAL);
  }
  [^\n]* { 
    
   }

}

    /* let's begin! */

"int" {
    PrintID(ID_INT);
}

"if" {
    PrintID(ID_IF);
}

"else" {
    PrintID(ID_ELSE);

}

"while" {
    PrintID(ID_WHILE);
}

"for" {
    PrintID(ID_FOR);
}

"write" {
    PrintID(ID_WRITE);
}

"read" {
    PrintID(ID_READ);
}

"return" {
    PrintID(ID_RETURN);
}

"continue" {
    PrintID(ID_CONTINUE);
}

"break" {
    PrintID(ID_BREAK);
}



{INT_HEX} {
    PrintCon(yytext,16);
}

{INT_OCT} {
    PrintCon(yytext,8);
}

{INT_DEC} {
    PrintCon(yytext,10);
}

{INT_BIN} {
    PrintCon(yytext+2,2);
}

{FLOAT_DEC} {
    PrintDouble(yytext);
}

{ZERO} {
    PrintCon(yytext,10);
}

{IDENTIFIER} {
    PrintVar(yytext);
}

"<=" { 
    PrintID(ID_LE);
}

">=" {
    PrintID(ID_GE);
}

"!=" {
    PrintID(ID_NE);
}

"==" {
    PrintID(ID_EQ);
}

"&&" {
    PrintID(ID_LAND);
}

"||" {
    PrintID(ID_LOR);
}

"<<" {
    PrintID(ID_LS);
}

">>" {
    PrintID(ID_RS);
}

"<" {
    PrintID(ID_LT);
}

">" {
    PrintID(ID_GT);
}

"+" {
    PrintID(ID_PLUS);
}

"-" {
    PrintID(ID_SUB);
}

"*" {
    PrintID(ID_MUL);
}

"/" {
    PrintID(ID_DIV);
}

"|" {
    PrintID(ID_BOR);
}

"&" {
    PrintID(ID_BAND);
}

"^" {
    PrintID(ID_XOR);
}


"!" {
    PrintID(ID_NOT);
}



"=" {
    PrintID(ID_ASSIGN);
}

"(" {
    PrintID(ID_LBRACKET);
}

")" {
    PrintID(ID_RBRACKET);
}

"{" {
    PrintID(ID_BIG_LBRACKET);
}

"}" {
    PrintID(ID_BIG_RBRACKET);
}

";" {
    PrintID(ID_SEMICOLON);
}

"[" {
    PrintID(ID_MID_LBRACKET);
}

"]" {
    PrintID(ID_MID_RBRACKET);
}

"," {
    PrintID(ID_COMMA);
}

[ \t\f\v\r]+ {
    // do nothing
}

\n {
    curr_lineno++;
}


%%

int main(int argc, char **argv) {
    char* input = argv[1];
    char* output = argv[2];
    yyin = fopen(input, "r");
    freopen(output, "w", stdout);
    yylex();
}