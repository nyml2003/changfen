#include "frontend/irgenerator.h"
#include "index.h"
namespace sed
{
void CompilationUnit2IR(const frontend::ast::CompilationUnit &compilationUnit, ir::Builder &builder)
{
    for (auto &statement : compilationUnit.statements)
    {
        Statement2IR(statement, compilationUnit.symbolTable, builder);
    }
}
void Statement2IR(frontend::ast::StatementPtr statement, frontend::SymbolTablePtr symbolTable, ir::Builder &builder)
{
    // std::visit(
    //     overloaded{
    //         [symbolTable, &builder](const frontend::ast::statement::Declaration &kind) {
    //             switch (kind.scope)
    //             {
    //             case frontend::Scope::Global: {
    //                 for (auto &definition : kind.definitions)
    //                 {
    //                     auto [astType, name, value] = definition;
    //                     auto irType = ValueType2IR(astType, builder);
    //                 }
    //                 break;
    //             }
    //             }
    //         },
    //     },
    //     statement->kind);
}
} // namespace sed