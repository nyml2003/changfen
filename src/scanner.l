%{
#include "LexTable.hpp"
#include "lexical_analyzer.h"
#include <stdio.h>

static int curr_lineno = 0;

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
  [^*\n]+   {  }
  [/*]     {  }
  <<EOF>>   {
    appendErrorWithLine("Comment not closed", curr_lineno);
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
  [^\n]* {  }

}

    /* let's begin! */

"int" {
    appendSignWithLine(ID_INT, curr_lineno);
}

"if" {
    appendSignWithLine(ID_IF, curr_lineno);
}

"else" {
    appendSignWithLine(ID_ELSE,  curr_lineno);
}

"while" {
    appendSignWithLine(ID_WHILE, curr_lineno);
}

"for" {
    appendSignWithLine(ID_FOR, curr_lineno);
}

"write" {
    appendSignWithLine(ID_WRITE, curr_lineno);
}

"read" {
    appendSignWithLine(ID_READ, curr_lineno);
}

"return" {
    appendSignWithLine(ID_RETURN, curr_lineno);
}

"continue" {
    appendSignWithLine(ID_CONTINUE,  curr_lineno);
}

"break" {
    appendSignWithLine(ID_BREAK, curr_lineno);
}

{IDENTIFIER} {
    appendIdentWithLine(yytext, curr_lineno);
}

{CONSTANT} {
    appendConstantWithLine(yytext, curr_lineno);
}

"<=" { 
    appendSignWithLine(ID_LE, curr_lineno);
}

">=" {
    appendSignWithLine(ID_GE, curr_lineno);
}

"!=" {
    appendSignWithLine(ID_NE, curr_lineno);
}

"==" {
    appendSignWithLine(ID_EQ, curr_lineno);
}

"&&" {
    appendSignWithLine(ID_LAND, curr_lineno);
}

"||" {
    appendSignWithLine(ID_LOR, curr_lineno);
}

"<<" {
    appendSignWithLine(ID_LS, curr_lineno);
}

">>" {
    appendSignWithLine(ID_RS, curr_lineno);
}

"<" {
    appendSignWithLine(ID_LT, curr_lineno);
}

">" {
    appendSignWithLine(ID_GT, curr_lineno);
}

"+" {
    appendSignWithLine(ID_PLUS, curr_lineno);
}

"-" {
    appendSignWithLine(ID_SUB, curr_lineno);
}

"*" {
    appendSignWithLine(ID_MUL, curr_lineno);
}

"/" {
    appendSignWithLine(ID_DIV, curr_lineno);
}

"|" {
    appendSignWithLine(ID_BOR, curr_lineno);
}

"&" {
    appendSignWithLine(ID_BAND, curr_lineno);
}

"^" {
    appendSignWithLine(ID_XOR, curr_lineno);
}


"!" {
    appendSignWithLine(ID_NOT, curr_lineno);
}



"=" {
    appendSignWithLine(ID_ASSIGN, curr_lineno);
}

"(" {
    appendSignWithLine(ID_LBRACKET, curr_lineno);
}

")" {
    appendSignWithLine(ID_RBRACKET, curr_lineno);
}

"{" {
    appendSignWithLine(ID_BIG_LBRACKET, curr_lineno);
}

"}" {
    appendSignWithLine(ID_BIG_RBRACKET, curr_lineno);
}

";" {
    appendSignWithLine(ID_SEMICOLON, curr_lineno);
}

"[" {
    appendSignWithLine(ID_MID_LBRACKET, curr_lineno);
}

"]" {
    appendSignWithLine(ID_MID_RBRACKET, curr_lineno);
}

"," {
    appendSignWithLine(ID_COMMA, curr_lineno);
}

[ \t] {  
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
    putln();
}