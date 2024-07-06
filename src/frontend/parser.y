%require "3.8"
%language "C++"

%skeleton "lalr1.cc"
%defines
%define api.token.constructor
%define api.value.type variant

%locations

%define parse.trace
%define parse.error verbose

%define api.location.file "../../../include/frontend/syntax/location.h"
%define api.location.include { "frontend/syntax/location.h" }
%define api.parser.class { Parser }
%define api.namespace { sed::frontend }

%code requires {
  typedef void* yyscan_t;

  #include "index.h"
  #include "frontend/ast.h"
  #include "frontend/symbolTable.h"
  #include "frontend/type.h" 
  #include "frontend/driver.h"
  #include "frontend/compileTime.h"

  using namespace sed;

}

%code {
  sed::frontend::Parser::symbol_type yylex(
    yyscan_t yyscanner, 
    sed::frontend::location& loc, 
    sed::frontend::Driver& driver
  );
}

%lex-param { yyscan_t yyscanner } { location& loc } { Driver& driver }
%parse-param { yyscan_t yyscanner } { location& loc } { Driver& driver }

%token END 0;

%token '+' '-' '*' '/' '%' '=' '[' ']' '(' ')' '{' '}' '!' ',' ';'
%token LE GE EQ NE LOR LAND

%token <std::string> IDENTIFIER 
%token <CompileTimeConstantValuePtr> INTEGER FLOATING

%token IF ELSE WHILE RETURN BREAK CONTINUE 
%token CONST INT FLOAT VOID

%start Program

%type <ast::StatementPtr> Stmt IfStmt WhileStmt ReturnStmt BreakStmt ExprStmt
%type <ast::StatementPtr> ContinueStmt BlankStmt DeclStmt AssignStmt BlockStmt

%type <std::tuple<ValueTypePtr, std::string, std::optional<ast::ExpressionPtr>>> Def
%type <std::vector<std::tuple<ValueTypePtr, std::string, std::optional<ast::ExpressionPtr>>>> DefList

%type <ast::ExpressionPtr> Expr AddExpr LOrExpr PrimaryExpr RelExpr MulExpr
%type <ast::ExpressionPtr> LAndExpr EqExpr UnaryExpr LVal InitVal Cond

%type <std::vector<ast::ExpressionPtr>> InitValList

%type <std::tuple<ValueTypePtr, std::string>> FuncParam
%type <std::vector<std::tuple<ValueTypePtr, std::string>>> FuncParamList
%type <std::vector<ast::ExpressionPtr>> FuncArgList

%type <ValueTypePtr> Type 
%type <std::vector<ast::ExpressionPtr>> ArrayIndices

%precedence THEN
%precedence ELSE


%%

Program 
  : Stmts
  ;

Stmts 
  : Stmt {
    if ($1 != nullptr) {
      driver.appendStatement($1);
    }
  }
  | Stmts Stmt {
    if ($2 != nullptr) {
      driver.appendStatement($2);
    }
  }
  ;

Stmt
  : AssignStmt {
    $$ = $1;
  }
  | IfStmt {
    $$ = $1;
  }
  | WhileStmt {
    $$ = $1;
  }
  | ReturnStmt {
    $$ = $1;
  }
  | ContinueStmt {
    $$ = $1;
  }
  | BreakStmt {
    $$ = $1;
  }
  | DeclStmt {
    $$ = $1;
  }
  | ExprStmt {
    $$ = $1;
  }
  | BlankStmt {
    $$ = $1;
  }
  | BlockStmt {
    $$ = $1;
  }
  | FuncDef {
    $$ = nullptr;
  }
  ;

FuncDef 
  : Type IDENTIFIER '(' ')' {
    driver.appendFunction($1, $2, {});
  } BlockStmt {
    driver.quitFunction();
  }
  | Type IDENTIFIER '(' FuncParamList ')' {
    driver.appendFunction($1, $2, $4);
  } BlockStmt {
    driver.quitFunction();
  }
  ;

ExprStmt
  : Expr ';' {
    $$ = ast::createExpression($1);
  }
  ;

DeclStmt
  : Type DefList ';' {
    $$ = ast::createDeclaration(driver.currentScope, false, $2,driver);
  }
  | CONST Type DefList ';' {
    $$ = ast::createDeclaration(driver.currentScope, true, $3,driver);
    driver.isCurrentDeclarationConst = false;
  }
  ;

DefList
  : DefList ',' Def {
    $$ = $1;
    $$.push_back($3);

    auto symbol_entry = ast::createSymbolEntry(
      driver.currentScope,
      driver.isCurrentDeclarationConst,
      $3
    );
    driver.currentSymbolTable->insert(symbol_entry);
  }
  | Def {
    $$.push_back($1);

    auto symbol_entry = ast::createSymbolEntry(
      driver.currentScope,
      driver.isCurrentDeclarationConst,
      $1
    );
    driver.currentSymbolTable->insert(symbol_entry);
  }
  ;

Def 
  : IDENTIFIER {
    $$ = std::make_tuple(driver.currentValueType, $1, std::nullopt);
  } 
  | IDENTIFIER ArrayIndices {
    ValueTypePtr decl_type = driver.currentValueType;

    auto curr_index_expr = $2.rbegin();
    while (curr_index_expr != $2.rend()) {
      auto maybe_decl_type = createArrayFromExpression(
        decl_type, *curr_index_expr);
      if (!maybe_decl_type.has_value()) {
        std::cerr << @2 << ":"
                  << "Array length must be compile-time available value" 
                  << std::endl;
        YYABORT;
      }
      decl_type = maybe_decl_type.value();
      ++curr_index_expr;
    }

    $$ = std::make_tuple(decl_type, $1, std::nullopt);
  }
  | IDENTIFIER '=' InitVal {
    auto init_val = $3;
    if (std::holds_alternative<ast::expression::InitializerList>(init_val->kind)) {
      std::get<ast::expression::InitializerList>(init_val->kind)
        .setValueType(driver.currentValueType, driver);
    } else if (init_val->getValueType() != driver.currentValueType) {
      init_val = createCast(driver.currentValueType, init_val, driver);
    }
    $$ = std::make_tuple(
      driver.currentValueType, 
      $1, 
      std::make_optional(init_val)
    );
  }
  | IDENTIFIER ArrayIndices '=' InitVal {
    ValueTypePtr decl_type = driver.currentValueType;

    auto curr_index_expr = $2.rbegin();
    while (curr_index_expr != $2.rend()) {
      auto maybe_decl_type = createArrayFromExpression(
        decl_type, *curr_index_expr);
      if (!maybe_decl_type.has_value()) {
        std::cerr << @2 << ":"
                  << "Array length must be compile-time available value" 
                  << std::endl;
        YYABORT;
      }
      decl_type = maybe_decl_type.value();
      ++curr_index_expr;
    }

    auto init_val = $4;
    if (std::holds_alternative<ast::expression::InitializerList>(init_val->kind)) {
      std::get<ast::expression::InitializerList>(init_val->kind)
        .setValueType(decl_type, driver);
    }
    $$ = std::make_tuple(decl_type, $1, std::make_optional(init_val));
  }
  ;

ArrayIndices
  : '[' Expr ']' {
    $$ = { $2 };
  }
  | ArrayIndices '[' Expr ']' {
    $$ = $1;
    $$.push_back($3);
  }
  ;

InitVal 
  : Expr {
    $$ = $1;
  }
  | '{' '}' {
    $$ = ast::createInitializerList({});
  }
  | '{' InitValList '}' {
    $$ = ast::createInitializerList($2);
  }
  ;

InitValList 
  : InitVal {
    $$.push_back($1);
  }
  | InitValList ',' InitVal {
    $$ = $1;
    $$.push_back($3);
  } 
  ;

IfStmt 
  : IF '(' Cond ')' Stmt %prec THEN {
    $$ = ast::createIf($3, $5, std::nullopt,driver);
  }
  | IF '(' Cond ')' Stmt ELSE Stmt {
    $$ = ast::createIf($3, $5, std::make_optional($7),driver);
  }
  ;

WhileStmt 
  : WHILE '(' Cond ')' Stmt {
    $$ = ast::createWhile($3, $5,driver);
  }
  ;

ReturnStmt 
  : RETURN Expr ';' {
    $$ = ast::createReturn(std::make_optional($2), driver);
  }
  | RETURN ';' {
    $$ = ast::createReturn(std::nullopt, driver);
  }
  ;

ContinueStmt 
  : CONTINUE ';' {
    $$ = ast::createContinue();
  }
  ;

BreakStmt 
  : BREAK ';' {
    $$ = ast::createBreak();
  }
  ;

BlockStmt
  : '{' {
    driver.appendBlock();
  } Stmts '}' {
    $$ = driver.currentBlock;
    driver.quitBlock();
  }
  | '{' '}' {
    // Just ignore.
    $$ = ast::createBlank();
  }
  ;

BlankStmt 
  : ';' {
    $$ = ast::createBlank();
  }
  ;


Expr 
  : AddExpr {
    $$ = $1;
  }
  ;

Cond
  : LOrExpr {
    $$ = $1;
  }
  ;

PrimaryExpr
  : '(' Expr ')' {
    $$ = $2;
  }
  | LVal {
    $$ = $1;
  }
  | INTEGER {
    $$ = ast::createConstant($1);
  }
  | FLOATING {
    $$ = ast::createConstant($1);
  }
  ;

LVal
  : IDENTIFIER {
    auto maybe_symbol_entry = driver.currentSymbolTable->find($1);
    if (!maybe_symbol_entry.has_value()) {
      std::cerr << @1 << ":" << "Undefined identifier: " + $1;
      YYABORT;
    }
    $$ = ast::createIdentifier(maybe_symbol_entry.value());
  }
  | LVal '[' Expr ']' {
    $$ = ast::createBinary(
      BinaryOperator::At, $1, $3, driver
    );
  }
  ;

AssignStmt
  : LVal '=' Expr ';' {
    $$ = ast::createAssignment($1, $3, driver);
  }
  ;

UnaryExpr
  : PrimaryExpr {
    $$ = $1;
  }
  | '+' UnaryExpr {
    $$ = ast::createUnary(UnaryOperator::Plus, $2, driver);
  }
  | '-' UnaryExpr {
    $$ = ast::createUnary(UnaryOperator::Minus, $2, driver);
  }
  | '!' UnaryExpr {
    $$ = ast::createUnary(UnaryOperator::Not, $2, driver);
  }
  | IDENTIFIER '(' FuncArgList ')' {
    auto maybe_symbol_entry = driver.compilationUnit.symbolTable->find($1);
    if (!maybe_symbol_entry.has_value()) {
      std::cerr << @1 << ":" << "Undefined identifier: " + $1;
      YYABORT;
    }
    auto maybe_func_type = maybe_symbol_entry.value()->valueType;
    if (
      !std::holds_alternative<valueType::Function>(maybe_func_type->kind)
    ) {
      std::cerr << @1 << ":" << "Not a function: " + $1;
      YYABORT;
    }
    $$ = ast::createFunctionCall(
      maybe_symbol_entry.value(), 
      $3, 
      driver
    );
  }
  | IDENTIFIER '(' ')' {
    
      auto maybe_symbol_entry = driver.compilationUnit.symbolTable->find($1);
      if (!maybe_symbol_entry.has_value()) {
        std::cerr << @1 << ":" << "Undefined identifier: " + $1;
        YYABORT;
      }
      auto maybe_func_type = maybe_symbol_entry.value()->valueType;
      if (
        !std::holds_alternative<valueType::Function>(maybe_func_type->kind)
      ) {
        std::cerr << @1 << ":" << "Not a function: " + $1;
        YYABORT;
      }
      $$ = ast::createFunctionCall(
        maybe_symbol_entry.value(), 
        {}, 
        driver
      );
    
  }
  ;

MulExpr
  : UnaryExpr {
    $$ = $1;
  }
  | MulExpr '*' UnaryExpr {
    $$ = ast::createBinary(
      BinaryOperator::Mul, $1, $3, driver);
  }
  | MulExpr '/' UnaryExpr {
    $$ = ast::createBinary(
      BinaryOperator::Div, $1, $3, driver);
  }
  | MulExpr '%' UnaryExpr {
    $$ = ast::createBinary(
      BinaryOperator::Mod, $1, $3, driver);
  }
  ;

AddExpr
  : MulExpr {
    $$ = $1;
  }
  | AddExpr '+' MulExpr {
    $$ = ast::createBinary(
      BinaryOperator::Add, $1, $3, driver);
  }
  | AddExpr '-' MulExpr {
    $$ = ast::createBinary(
      BinaryOperator::Sub, $1, $3, driver);
  }
  ;

RelExpr
  : AddExpr {
    $$ = $1;
  }
  | RelExpr '<' AddExpr {
    $$ = ast::createBinary(
      BinaryOperator::Lt, $1, $3, driver);
  }
  | RelExpr '>' AddExpr {
    $$ = ast::createBinary(
      BinaryOperator::Gt, $1, $3, driver);
  }
  | RelExpr LE AddExpr {
    $$ = ast::createBinary(
      BinaryOperator::Le, $1, $3, driver);
  }
  | RelExpr GE AddExpr {
    $$ = ast::createBinary(
      BinaryOperator::Ge, $1, $3, driver);
  }
  ;

EqExpr
  : RelExpr {
    $$ = $1;
  }
  | EqExpr EQ RelExpr {
    $$ = ast::createBinary(
      BinaryOperator::Eq, $1, $3, driver);
  }
  | EqExpr NE RelExpr {
    $$ = ast::createBinary(
      BinaryOperator::Ne, $1, $3, driver);
  }
  ;

LAndExpr
  : EqExpr {
    $$ = $1;
  }
  | LAndExpr LAND EqExpr {
    $$ = ast::createBinary(
      BinaryOperator::And, $1, $3, driver);
  }
  ;

LOrExpr
  : LAndExpr {
    $$ = $1;
  }
  | LOrExpr LOR LAndExpr {
    $$ = ast::createBinary(
      BinaryOperator::Or, $1, $3, driver);
  }
  ;

FuncArgList
  : Expr {
    $$.push_back($1);
  }
  | FuncArgList ',' Expr {
    $$ = $1;
    $$.push_back($3);
  }
  ;

FuncParamList 
  : FuncParam {
    $$.push_back($1);
  }
  | FuncParamList ',' FuncParam {
    $$ = $1;
    $$.push_back($3);
  }
  ;

FuncParam
  : Type IDENTIFIER {
    $$ = std::make_tuple($1, $2);
  }
  | Type IDENTIFIER ArrayIndices {
    ValueTypePtr type = $1;

    auto curr_index_expr = $3.rbegin();
    while (curr_index_expr != $3.rend()) {
      auto maybe_type = createArrayFromExpression(
        type, *curr_index_expr);
      if (!maybe_type.has_value()) {
        std::cerr << @2 << ":"
                  << "Array length must be compile-time available value" 
                  << std::endl;
        YYABORT;
      }
      type = maybe_type.value();
      ++curr_index_expr;
    }

    $$ = std::make_tuple(type, $2);
  }
  | Type IDENTIFIER '[' ']' {
    $$ = std::make_tuple(createArray($1, std::nullopt), $2);
  }
  | Type IDENTIFIER '[' ']' ArrayIndices {
    ValueTypePtr type = $1;

    auto curr_index_expr = $5.rbegin();
    while (curr_index_expr != $5.rend()) {
      auto maybe_type = createArrayFromExpression(
        type, *curr_index_expr);
      if (!maybe_type.has_value()) {
        std::cerr << @2 << ":"
                  << "Array length must be compile-time available value" 
                  << std::endl;
        YYABORT;
      }
      type = maybe_type.value();
      ++curr_index_expr;
    }

    type = createArray(type, std::nullopt);

    $$ = std::make_tuple(type, $2);
  }
  ;

Type
  : INT {
    $$ = createInteger32();
    driver.currentValueType = $$;
    driver.currentScope = driver.currentFunction ? Scope::Local 
                                                 : Scope::Global;

  }
  | FLOAT {
    $$ = createFloat32();
    driver.currentValueType = $$;
    driver.currentScope = driver.currentFunction ? Scope::Local 
                                                 : Scope::Global;
  }
  | VOID {
    $$ = createVoid();
    driver.currentValueType = $$;
    driver.currentScope = driver.currentFunction ? Scope::Local 
                                                 : Scope::Global;
  }
  ;

%%

namespace sed {
namespace frontend {

void Parser::error (const location_type& loc, const std::string& msg) {
  std::cerr << loc << ": " << msg << std::endl;
}

} // namespace frontend
} // namespace sed
