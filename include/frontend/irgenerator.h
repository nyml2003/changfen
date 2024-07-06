#ifndef SED_IR_GENERATOR_H
#define SED_IR_GENERATOR_H
#include "frontend/ast.h"
#include "index.h"
#include "ir/builder.h"
#include <optional>
namespace sed
{

void CompilationUnit2IR(const frontend::ast::CompilationUnit &cu, ir::Builder &builder);

void Statement2IR(frontend::ast::StatementPtr stmt, frontend::SymbolTablePtr symbolTable, ir::Builder &builder);

std::optional<ir::OperandID> Expression2IR(frontend::ast::ExpressionPtr expr, frontend::SymbolTablePtr symbolTable,
                                           ir::Builder &builder);

std::optional<ir::TypePtr> ValueType2IR(frontend::ValueTypePtr type, ir::Builder &builder);

void CompileTime2IR(frontend::CompileTimeConstantValuePtr compileTime, ir::Builder &builder);
} // namespace sed
#endif