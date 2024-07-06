#ifndef SED_SYMBOL_TABLE_H
#define SED_SYMBOL_TABLE_H
#include "frontend/ast.h"
#include "index.h"
#include <map>
#include <optional>
#include <string>
namespace sed::frontend
{
enum class Scope
{
    Global,
    Local,
    Temp,
    Param
};

struct SymbolEntry
{
    std::string name;
    ValueTypePtr valueType;
    Scope scope;
    bool isConstant;
    std::optional<CompileTimeConstantValuePtr> value;
    SymbolEntry(std::string name, ValueTypePtr valueType, Scope scope, bool isConstant,
                std::optional<CompileTimeConstantValuePtr> value);

    bool isCompileTimeConstant() const;
    std::string to_string() const;
    std::optional<ir::OperandID> maybe_ir_operand_id = std::nullopt;
    void set_ir_operand_id(ir::OperandID ir_operand_id);
    bool has_ir_operand() const;

};

SymbolEntryPtr createSymbolEntry(std::string name, ValueTypePtr valueType, Scope scope, bool isConstant,
                                 std::optional<CompileTimeConstantValuePtr> value);

struct SymbolTable
{
    std::map<std::string, SymbolEntryPtr> entries;

    std::optional<SymbolTablePtr> parent;

    SymbolTable(std::optional<SymbolTablePtr> parent = std::nullopt);

    std::optional<SymbolEntryPtr> find(const std::string &name, bool recursive = true) const;

    void insert(SymbolEntryPtr entry);

    std::string to_string() const;
};

SymbolTablePtr createSymbolTable(std::optional<SymbolTablePtr> parent = std::nullopt);

} // namespace sed::frontend

#endif // !SED_SYMBOL_TABLE_H