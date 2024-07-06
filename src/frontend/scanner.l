%option reentrant noyywrap nounput yylineno
%option header-file="/app/include/frontend/syntax/scanner.h"

%{

#include "index.h"
#include "frontend/ast.h"
#include "frontend/type.h"
#include "frontend/compileTime.h"

using namespace sed;

#define YY_DECL frontend::Parser::symbol_type yylex( \
  yyscan_t yyscanner, frontend::location&loc, frontend::Driver& driver)

#define YY_USER_ACTION                      \
  loc.begin.line = loc.end.line = yylineno; \
  loc.begin.column = yycolumn;              \
  loc.end.column = yycolumn + yyleng - 1;   \
  yycolumn += yyleng;

#define yyterminate() return frontend::Parser::make_END(loc)

#include "frontend/syntax/parser.h"

using frontend::Parser;
using token = frontend::Parser::token;
using token_type = frontend::Parser::token_type;

%}

DEC ([1-9][0-9]*|0)
OCT (0[0-7]+)
HEX (0[xX][0-9A-Fa-f]+)

IDENTIFIER [A-Za-z_][A-Za-z0-9_]*

EOL (\r\n|\n|\r)

WS [\t ]

BLOCK_COMMENT_BEGIN \/\*
BLOCK_COMMENT_ELEMENT .
BLOCK_COMMENT_END \*\/

%x BLOCK_COMMENT

LINE_COMMENT \/\/[^\n]*

DEC_FP ((([0-9]*[.][0-9]*([eE][+-]?[0-9]+)?)|([0-9]+[eE][+-]?[0-9]+))[fLlL]?)
HEX_FP (0[xX](([0-9A-Fa-f]*[.][0-9A-Fa-f]*([pP][+-]?[0-9]+)?)|([0-9A-Fa-f]+[pP][+-]?[0-9]+))[fLlL]?)

%%

%{
  loc.step();
%}

"+" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type('+'), loc);
}

"-" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type('-'), loc);
}

"*" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type('*'), loc);
}

"/" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type('/'), loc);
}

"%" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type('%'), loc);
}

"=" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type('='), loc);
}

"[" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type('['), loc);
}

"]" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type(']'), loc);
}

"{" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type('{'), loc);
}

"}" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type('}'), loc);
}

"(" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type('('), loc);
}

")" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type(')'), loc);
}

"!" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type('!'), loc);
}

"," {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type(','), loc);
}

";" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type(';'), loc);
}

">" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type('>'), loc);
}

"<" {
  driver.appendToken(yytext);
  return Parser::symbol_type(token_type('<'), loc);
}

"<=" {
  driver.appendToken("LE");
  return Parser::make_LE(loc);
}

">=" {
  driver.appendToken("GE");
  return Parser::make_GE(loc);
}

"==" {
  driver.appendToken("EQ");
  return Parser::make_EQ(loc);
}

"!=" {
  driver.appendToken("NE");
  return Parser::make_NE(loc);
}

"||" {
  driver.appendToken("LOR");
  return Parser::make_LOR(loc);
}

"&&" {
  driver.appendToken("LAND");
  return Parser::make_LAND(loc);
}

"int" {
  driver.appendToken("INT");
  return Parser::make_INT(loc);
}

"float" {
  driver.appendToken("FLOAT");
  return Parser::make_FLOAT(loc);
}

"void" {
  driver.appendToken("VOID");
  return Parser::make_VOID(loc);
}

"const" {
  driver.appendToken("CONST");
  driver.isCurrentDeclarationConst= true;
  return Parser::make_CONST(loc);
}

"if" {
  driver.appendToken("IF");
  return Parser::make_IF(loc);
}

"else" {
  driver.appendToken("ELSE");
  return Parser::make_ELSE(loc);
}

"return" {
  driver.appendToken("RETURN");
  return Parser::make_RETURN(loc);
}

"break" {
  driver.appendToken("BREAK");
  return Parser::make_BREAK(loc);
}

"continue" {
  driver.appendToken("CONTINUE");
  return Parser::make_CONTINUE(loc);
}

"while" {
  driver.appendToken("WHILE");
  return Parser::make_WHILE(loc);
}

{IDENTIFIER} {
  driver.appendToken(std::string("IDENTIFIER ") + yytext);
  return Parser::make_IDENTIFIER(yytext, loc);
}

{DEC} {
  auto value = frontend::createCompileTimeConstantValue(
    std::stoi(yytext, nullptr, 10) , frontend::createInteger32());

  driver.appendToken(value->to_string());

  return Parser::make_INTEGER(value, loc);
}

{OCT} {
  auto value = frontend::createCompileTimeConstantValue(
    std::stoi(yytext, nullptr, 8) , frontend::createInteger32());

  driver.appendToken(value->to_string());
  
  return Parser::make_INTEGER(value, loc);
}

{HEX} {
  auto value = frontend::createCompileTimeConstantValue(
    std::stoi(yytext, nullptr, 16) , frontend::createInteger32());

  driver.appendToken(value->to_string());

  return Parser::make_INTEGER(value, loc);
}

{DEC_FP} {
  auto value = frontend::createCompileTimeConstantValue(
    std::stof(yytext, nullptr), frontend::createFloat32());

  driver.appendToken(value->to_string());

  return Parser::make_FLOATING(value, loc);
}

{HEX_FP} {
  auto value = frontend::createCompileTimeConstantValue(
    std::stof(yytext, nullptr), frontend::createFloat32());

  driver.appendToken(value->to_string());

  return Parser::make_FLOATING(value, loc);
}

{EOL} { yycolumn = 1; }
{WS} {}
{LINE_COMMENT} {}
{BLOCK_COMMENT_BEGIN} {BEGIN BLOCK_COMMENT;}
<BLOCK_COMMENT>{BLOCK_COMMENT_ELEMENT} {}
<BLOCK_COMMENT>{BLOCK_COMMENT_END} {BEGIN INITIAL;}

%%