#ifndef SED_IR_OPERAND_H
#define SED_IR_OPERAND_H

#include "index.h"

namespace sed::ir
{
namespace operand
{

struct ZeroInitializer
{
};

// 全局变量
struct Global
{
    std::string name;
    bool is_constant;
    OperandID init;
};

struct Parameter
{
    std::string name;
};

// 立即数
struct Constant
{
    ConstantKind kind;
    TypePtr type;
    std::string to_string(bool with_type) const;
    bool get_bool_value() const;
    bool is_zero() const;
};

// 局部变量
struct Local
{
};

} // namespace operand

struct Operand
{
    OperandID id;
    OperandKind kind;
    TypePtr type;
    std::optional<InstructionID> maybe_def_id;
    std::set<InstructionID> use_id_list;

    Operand(OperandID id, OperandKind kind, TypePtr type);
    void set_def(InstructionID def_id);
    void add_use(InstructionID use_id);
    void remove_def();
    void remove_use(InstructionID use_id);
    bool is_int() const;
    bool is_float() const;
    bool is_zero() const;

    template <typename T> inline bool is() const
    {
        return std::holds_alternative<T>(kind);
    }

    template <typename T> inline T as() const
    {
        return std::get<T>(kind);
    }

    std::string to_string(bool with_type) const;
};

} // namespace sed::ir

#endif // SED_IR_OPERAND_H