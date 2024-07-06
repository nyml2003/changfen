#include "frontend/driver.h"
#include "frontend/syntax/parser.h"
#include "frontend/syntax/scanner.h"
namespace sed::frontend
{
Driver::Driver(std::string filename)
{
    compilationUnit = ast::CompilationUnit();
    currentBlock = nullptr;
    currentFunction = nullptr;
    currentValueType = nullptr;
    currentScope = Scope::Global;
    isCurrentDeclarationConst = false;
    currentSymbolTable = compilationUnit.symbolTable;
    blockStack = std::stack<ast::StatementPtr>();
    tokens = "";
    yylex_init(&scanner);
    yyset_in(fopen(filename.c_str(), "r"), scanner);
    loc = new location();
    parser = new Parser(scanner, *loc, *this);
    this->appendFunctionDeclaration(createInteger32(), "getint", {});
    this->appendFunctionDeclaration(createInteger32(), "getch", {});
    this->appendFunctionDeclaration(createFloat32(), "getfloat", {});
    this->appendFunctionDeclaration(createInteger32(), "getarray",
                                    {{createArray(createInteger32(), std::nullopt), "a"}});
    this->appendFunctionDeclaration(createInteger32(), "getfarray",
                                    {{createArray(createFloat32(), std::nullopt), "a"}});
    this->appendFunctionDeclaration(createVoid(), "putint", {{createInteger32(), "a"}});
    this->appendFunctionDeclaration(createVoid(), "putch", {{createInteger32(), "a"}});
    this->appendFunctionDeclaration(createVoid(), "putfloat", {{createFloat32(), "a"}});
    this->appendFunctionDeclaration(createVoid(), "putarray",
                                    {
                                        {createInteger32(), "n"},
                                        {createArray(createInteger32(), std::nullopt), "a"},
                                    });
    this->appendFunctionDeclaration(createVoid(), "putfarray",
                                    {
                                        {createInteger32(), "n"},
                                        {createArray(createFloat32(), std::nullopt), "a"},
                                    });
    this->appendFunctionDeclaration(createVoid(), "_sysy_starttime", {{createInteger32(), "lineno"}});
    this->appendFunctionDeclaration(createVoid(), "_sysy_stoptime", {{createInteger32(), "lineno"}});
    this->appendFunctionDeclaration(
        createVoid(), "__builtin_fill_zero",
        {{createArray(createInteger32(), std::nullopt), "dest"}, {createInteger32(), "count"}});
}
Driver::~Driver()
{
    yylex_destroy(scanner);
    delete parser;
    delete loc;
}

void Driver::appendToken(const std::string &token)
{
    tokens += token + "\n";
}

void Driver::appendStatement(ast::StatementPtr statement)
{
    if (currentFunction)
    {
        std::get<ast::statement::Block>(currentBlock->kind).statements.push_back(statement);
    }
    else
    {
        compilationUnit.statements.push_back(statement);
    }
}

void Driver::appendBlock()
{
    if (!currentFunction)
    {
        throw std::runtime_error("Cannot append block in function");
    }
    auto block = ast::createBlock(currentSymbolTable);
    if (!currentBlock)
    {
        std::get<ast::statement::FunctionDefinition>(currentFunction->kind).body = block;
    }
    currentBlock = block;
    currentSymbolTable = std::get<ast::statement::Block>(block->kind).symbolTable;
    blockStack.push(currentBlock);
}

void Driver::quitBlock()
{
    if (!currentFunction)
    {
        throw std::runtime_error("Cannot quit block outside of function");
    }
    blockStack.pop();
    if (blockStack.empty())
    {
        currentBlock = nullptr;
        currentSymbolTable = std::get<ast::statement::FunctionDefinition>(currentFunction->kind).symbolTable;
    }
    else
    {
        currentBlock = blockStack.top();
        currentSymbolTable = std::get<ast::statement::Block>(currentBlock->kind).symbolTable;
    }
}

void Driver::appendFunction(ValueTypePtr returnValueType, std::string name,
                            std::vector<std::tuple<ValueTypePtr, std::string>> parameters)
{
    if (currentFunction)
    {
        throw std::runtime_error("Cannot append function in function");
    }
    auto function = ast::createFunctionDefinition(currentSymbolTable, returnValueType, name, parameters);
    appendStatement(function);
    currentFunction = function;
    currentSymbolTable = std::get<ast::statement::FunctionDefinition>(function->kind).symbolTable;
}

void Driver::quitFunction()
{
    currentBlock = nullptr;
    blockStack = std::stack<ast::StatementPtr>();
    currentFunction = nullptr;
    currentSymbolTable = compilationUnit.symbolTable;
}

void Driver::appendFunctionDeclaration(ValueTypePtr returnValueType, std::string name,
                                       std::vector<std::tuple<ValueTypePtr, std::string>> parameters)
{
    appendFunction(returnValueType, name, parameters);
    quitFunction();
}

std::string Driver::getTempSymbolEntryName()
{
    return "temp" + std::to_string(tempSymbolEntryCounter++);
}

} // namespace sed::frontend