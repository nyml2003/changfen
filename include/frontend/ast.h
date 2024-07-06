#ifndef SED_frontend_AST_H
#define SED_frontend_AST_H
#include "frontend/compileTime.h"
#include "frontend/symbolTable.h"
#include "frontend/type.h"
#include <memory>
#include <optional>

namespace sed::frontend
{
enum class BinaryOperator
{
    Add, // +
    Sub, // -
    Mul, // *
    Div, // /
    Mod, // %
    And, // &&
    Or,  // ||
    Eq,  // ==
    Ne,  // !=
    Lt,  // <
    Le,  // <=
    Gt,  // >
    Ge,  // >=
    At,
};

enum class UnaryOperator
{
    Plus,  // +
    Minus, // -
    Not,   // !
};

namespace ast
{
namespace expression
{
struct Identifier
{
    std::string name;

    SymbolEntryPtr symbolEntry;
};

struct Binary
{
    BinaryOperator op;
    ExpressionPtr lhs;
    ExpressionPtr rhs;
    SymbolEntryPtr symbol;
};

struct Unary
{
    UnaryOperator op;
    ExpressionPtr operand;
    SymbolEntryPtr symbol;
};

struct InitializerList
{
    std::vector<ExpressionPtr> elements;
    ValueTypePtr valueType = nullptr;
    // TODO
    bool is_zeroinitializer = false;

    void setValueType(ValueTypePtr valueType, Driver &driver);
};

struct FunctionCall
{
    std::vector<ExpressionPtr> arguments;
    SymbolEntryPtr function;
    SymbolEntryPtr call;
};

struct Cast
{
    ValueTypePtr targetType;
    ExpressionPtr value;
    SymbolEntryPtr symbol;
};

struct Constant
{
    CompileTimeConstantValuePtr value;
};
} // namespace expression

struct Expression
{
    ExpressionKind kind;

    Expression(ExpressionKind kind);

    bool isCompileTimeConstant() const;

    std::optional<CompileTimeConstantValuePtr> getCompileTimeConstantValue() const;

    ValueTypePtr getValueType() const;

    std::string to_string() const;
};
namespace statement
{

struct Blank
{
};

struct If
{
    ExpressionPtr condition;
    StatementPtr thenStatement;
    std::optional<StatementPtr> elseStatement;
};

struct While
{
    ExpressionPtr condition;
    StatementPtr body;
};

struct Break
{
};

struct Continue
{
};

struct Return
{
    std::optional<ExpressionPtr> value;
};

struct Block
{
    SymbolTablePtr symbolTable;
    std::vector<StatementPtr> statements;
};

struct Assignment
{
    ExpressionPtr lhs;
    ExpressionPtr rhs;
};

struct Expression
{
    ExpressionPtr expression;
};

struct Declaration
{
    Scope scope;

    bool isConst;

    std::vector<std::tuple<ValueTypePtr, std::string, std::optional<ExpressionPtr>>> definitions;
};

struct FunctionDefinition
{
    SymbolTablePtr symbolTable;
    SymbolEntryPtr function;
    std::vector<std::string> parameters;
    std::optional<StatementPtr> body = std::nullopt;
};
} // namespace statement

struct Statement
{
    StatementKind kind;

    Statement(StatementKind kind);

    std::string to_string() const;

    std::optional<statement::Block> asBlock() const;
};

struct CompilationUnit
{
    SymbolTablePtr symbolTable;
    std::vector<StatementPtr> statements;

    CompilationUnit();

    std::string to_string() const;
};

ExpressionPtr createIdentifier(SymbolEntryPtr symbolEntry);

ExpressionPtr createConstant(CompileTimeConstantValuePtr value);

ExpressionPtr createBinary(BinaryOperator op, ExpressionPtr lhs, ExpressionPtr rhs, Driver &driver);

ExpressionPtr createUnary(UnaryOperator op, ExpressionPtr operand, Driver &driver);

ExpressionPtr createFunctionCall(SymbolEntryPtr function, std::vector<ExpressionPtr> arguments, Driver &driver);

ExpressionPtr createCast(ValueTypePtr targetType, ExpressionPtr value, Driver &driver);

ExpressionPtr createInitializerList(std::vector<ExpressionPtr> elements);

StatementPtr createBlank();

StatementPtr createIf(ExpressionPtr condition, StatementPtr thenStatement, std::optional<StatementPtr> elseStatement,Driver& driver);

StatementPtr createWhile(ExpressionPtr condition, StatementPtr body,Driver& driver);

StatementPtr createBreak();

StatementPtr createContinue();

StatementPtr createReturn(std::optional<ExpressionPtr> value, Driver &driver);

StatementPtr createBlock(SymbolTablePtr parent);

StatementPtr createAssignment(ExpressionPtr lhs, ExpressionPtr rhs, Driver &driver);

StatementPtr createExpression(ExpressionPtr expression);

StatementPtr createDeclaration(
    Scope scope, bool isConst,
    std::vector<std::tuple<ValueTypePtr, std::string, std::optional<ExpressionPtr>>> definitions, Driver &driver);

StatementPtr createFunctionDefinition(SymbolTablePtr parent, ValueTypePtr returnType, std::string name,
                                      std::vector<std::tuple<ValueTypePtr, std::string>> parameters);

SymbolEntryPtr createSymbolEntry(Scope scope, bool isConstant,
                                 std::tuple<ValueTypePtr, std::string, std::optional<ExpressionPtr>> definition);

size_t countElements(std::vector<size_t> dims, size_t from, size_t to, std::vector<ExpressionPtr> elements);

void outputElements(std::vector<ExpressionPtr> elements, std::string message = "");

} // namespace ast

} // namespace sed::frontend

#endif // !SED_frontend_AST_H
