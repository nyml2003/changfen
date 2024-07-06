#ifndef SED_frontend_DRIVER_H
#define SED_frontend_DRIVER_H
#include "frontend/ast.h"
#include "frontend/compileTime.h"
#include "frontend/syntax/parser.h"
#include <optional>
#include <stack>
namespace sed::frontend
{
class Parser;
class location;
struct Driver
{
    ast::CompilationUnit compilationUnit;
    ast::StatementPtr currentBlock;
    ast::StatementPtr currentFunction;
    SymbolTablePtr currentSymbolTable;
    std::stack<ast::StatementPtr> blockStack = std::stack<ast::StatementPtr>();
    std::string tokens;
    ValueTypePtr currentValueType;
    Scope currentScope;
    bool isCurrentDeclarationConst;
    yyscan_t scanner;
    location *loc;
    Parser *parser;
    size_t tempSymbolEntryCounter = 0;
    std::string getTempSymbolEntryName();
    Driver(std::string filename = "");
    ~Driver();

    void appendToken(const std::string &token);
    void appendStatement(ast::StatementPtr statement);
    void appendBlock();
    void quitBlock();
    void appendFunction(ValueTypePtr returnValueType, std::string name,
                        std::vector<std::tuple<ValueTypePtr, std::string>> parameters =
                            std::vector<std::tuple<ValueTypePtr, std::string>>());
    void quitFunction();
    void appendFunctionDeclaration(ValueTypePtr returnValueType, std::string name,
                                   std::vector<std::tuple<ValueTypePtr, std::string>> parameters);
};
} // namespace sed::frontend
#endif