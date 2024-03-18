%{
#include "LexTable.hpp"
#include "lexical_analyzer.h"
#include <stdio.h>

static int curr_lineno = 0;
static LexTable * table = newLexTable();

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
    appendErrorWithLine(table, "Comment not closed", curr_lineno);
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
    appendSignWithLine(table, ID_INT, curr_lineno);
}

"if" {
    appendSignWithLine(table, ID_IF, curr_lineno);
}

"else" {
    appendSignWithLine(table, ID_ELSE,  curr_lineno);
}

"while" {
    appendSignWithLine(table, ID_WHILE, curr_lineno);
}

"for" {
    appendSignWithLine(table, ID_FOR, curr_lineno);
}

"write" {
    appendSignWithLine(table, ID_WRITE, curr_lineno);
}

"read" {
    appendSignWithLine(table, ID_READ, curr_lineno);
}

"return" {
    appendSignWithLine(table, ID_RETURN, curr_lineno);
}

"continue" {
    appendSignWithLine(table, ID_CONTINUE,  curr_lineno);
}

"break" {
    appendSignWithLine(table, ID_BREAK, curr_lineno);
}

{IDENTIFIER} {
    appendIdentWithLine(table, yytext, curr_lineno);
}

{CONSTANT} {
    appendConstantWithLine(table, yytext, curr_lineno);
}

"<=" { 
    appendSignWithLine(table, ID_LE, curr_lineno);
}

">=" {
    appendSignWithLine(table, ID_GE, curr_lineno);
}

"!=" {
    appendSignWithLine(table, ID_NE, curr_lineno);
}

"==" {
    appendSignWithLine(table, ID_EQ, curr_lineno);
}

"&&" {
    appendSignWithLine(table, ID_LAND, curr_lineno);
}

"||" {
    appendSignWithLine(table, ID_LOR, curr_lineno);
}

"<<" {
    appendSignWithLine(table, ID_LS, curr_lineno);
}

">>" {
    appendSignWithLine(table, ID_RS, curr_lineno);
}

"<" {
    appendSignWithLine(table, ID_LT, curr_lineno);
}

">" {
    appendSignWithLine(table, ID_GT, curr_lineno);
}

"+" {
    appendSignWithLine(table, ID_PLUS, curr_lineno);
}

"-" {
    appendSignWithLine(table, ID_SUB, curr_lineno);
}

"*" {
    appendSignWithLine(table, ID_MUL, curr_lineno);
}

"/" {
    appendSignWithLine(table, ID_DIV, curr_lineno);
}

"|" {
    appendSignWithLine(table, ID_BOR, curr_lineno);
}

"&" {
    appendSignWithLine(table, ID_BAND, curr_lineno);
}

"^" {
    appendSignWithLine(table, ID_XOR, curr_lineno);
}


"!" {
    appendSignWithLine(table, ID_NOT, curr_lineno);
}



"=" {
    appendSignWithLine(table, ID_ASSIGN, curr_lineno);
}

"(" {
    appendSignWithLine(table, ID_LBRACKET, curr_lineno);
}

")" {
    appendSignWithLine(table, ID_RBRACKET, curr_lineno);
}

"{" {
    appendSignWithLine(table, ID_BIG_LBRACKET, curr_lineno);
}

"}" {
    appendSignWithLine(table, ID_BIG_RBRACKET, curr_lineno);
}

";" {
    appendSignWithLine(table, ID_SEMICOLON, curr_lineno);
}

"[" {
    appendSignWithLine(table, ID_MID_LBRACKET, curr_lineno);
}

"]" {
    appendSignWithLine(table, ID_MID_RBRACKET, curr_lineno);
}

"," {
    appendSignWithLine(table, ID_COMMA, curr_lineno);
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
    putln(table);
}