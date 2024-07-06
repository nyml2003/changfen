#ifndef SED_frontend_IRGEN_H
#define SED_frontend_IRGEN_H

#include "frontend/ast.h"
#include "frontend/symbolTable.h"
#include "index.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace sed
{

using AstBinaryOp = frontend::BinaryOperator;
using AstUnaryOp = frontend::UnaryOperator;

using AstCompunit = frontend::ast::CompilationUnit;
using AstStmtPtr = frontend::ast::StatementPtr;
using AstExprPtr = frontend::ast::ExpressionPtr;

using AstTypePtr = frontend::ValueTypePtr;
using AstScope = frontend::Scope;
using AstSymbolEntryPtr = frontend::SymbolEntryPtr;
using AstSymbolTablePtr = frontend::SymbolTablePtr;
using AstComptimeValuePtr = frontend::CompileTimeConstantValuePtr;

using IrBinaryOp = ir::instruction::BinaryOp;
using IrICmpCond = ir::instruction::ICmpOp;
using IrFCmpCond = ir::instruction::FCmpOp;
using IrCastOp = ir::instruction::CastOp;

using IrBuilder = ir::Builder;
using IrTypePtr = ir::TypePtr;
using IrConstantPtr = ir::operand::ConstantPtr;
using IrOperandID = ir::OperandID;
using IrInstructionPtr = ir::InstructionPtr;
using IrBasicBlockPtr = ir::BasicBlockPtr;

void irgen(const AstCompunit &compunit, IrBuilder &builder);

void irgen_stmt(AstStmtPtr stmt, AstSymbolTablePtr symtable, IrBuilder &builder);

std::optional<IrOperandID> irgen_expr(AstExprPtr expr, AstSymbolTablePtr symtable, IrBuilder &builder,
                                      bool use_address = false);

std::optional<IrTypePtr> irgen_type(AstTypePtr type, IrBuilder &builder);

IrConstantPtr irgen_comptime_value(AstComptimeValuePtr value, IrBuilder &builder);

} // namespace sed

#endif