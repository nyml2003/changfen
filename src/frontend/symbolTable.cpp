#include "frontend/symbolTable.h"
#include <sstream>

namespace sed::frontend
{

SymbolEntry::SymbolEntry(std::string name, ValueTypePtr valueType, Scope scope, bool isConstant,
                         std::optional<CompileTimeConstantValuePtr> value)
    : name(name), valueType(valueType), scope(scope), isConstant(isConstant), value(value)
{
}

SymbolEntryPtr createSymbolEntry(std::string name, ValueTypePtr valueType, Scope scope, bool isConstant,
                                 std::optional<CompileTimeConstantValuePtr> value)
{
    return std::make_shared<SymbolEntry>(name, valueType, scope, isConstant, value);
}

bool SymbolEntry::isCompileTimeConstant() const
{
    return value.has_value();
}
bool SymbolEntry::has_ir_operand() const {
  return maybe_ir_operand_id.has_value();
}

void SymbolEntry::set_ir_operand_id(ir::OperandID ir_operand_id) {
  this->maybe_ir_operand_id = ir_operand_id;
}

std::string SymbolEntry::to_string() const
{
    std::stringstream ss;
    switch (scope)
    {
    case Scope::Global:
        ss << "Global ";
        break;
    case Scope::Local:
        ss << "Local ";
        break;
    case Scope::Temp:
        ss << "Temp ";
        break;
    case Scope::Param:
        ss << "Param ";
        break;
    }
    if (isConstant)
    {
        ss << "Const ";
    }
    ss << name << ": " << valueType->to_string();
    if (isCompileTimeConstant())
    {
        ss << " = " << value.value()->to_string();
    }
    return ss.str();
}

SymbolTable::SymbolTable(std::optional<SymbolTablePtr> parent) : parent(parent)
{
}

SymbolTablePtr createSymbolTable(std::optional<SymbolTablePtr> parent)
{
    return std::make_shared<SymbolTable>(parent);
}

std::optional<SymbolEntryPtr> SymbolTable::find(const std::string &name, bool recursive) const
{
    auto it = entries.find(name);
    if (it != entries.end())
    {
        return it->second;
    }
    if (recursive && parent.has_value())
    {
        return parent.value()->find(name);
    }
    return std::nullopt;
}

void SymbolTable::insert(SymbolEntryPtr entry)
{
    entries[entry->name] = entry;
}

std::string SymbolTable::to_string() const
{
    std::stringstream ss;
    for (const auto &[name, entry] : entries)
    {
        ss << entry->to_string() << std::endl;
    }
    return ss.str();
}

} // namespace sed::frontend