%{


#include "ConSymbolTable.hpp"
#include "lexical_analyzer.h"
#include <stdio.h>
#include "VarSymbolTable.hpp"

static int curr_lineno = 0;

void PrintCon(char * token){
   printf("#%d ( %d , %s ,Con_%d)\n",curr_lineno, ID_CONSTANT, token,con(token)); // constant
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
CONSTANT   [1-9][0-9]*

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
  [^*\n]+   { 
   }
  [/*]     {  }
    \n        {
        curr_lineno++;
    }
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

{IDENTIFIER} {
    PrintVar(yytext);
}

{CONSTANT} {
    PrintCon(yytext);
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