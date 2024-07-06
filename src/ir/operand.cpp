#include "ir/operand.h"

#include "ir/type.h"
namespace sed::ir
{
namespace operand
{
std::string Constant::to_string(bool with_type) const
{
    std::stringstream s;
    if (with_type)
    {
        s << type->to_string() + " ";
    }
    std::visit(overloaded{[&s](int v) { s << v; },
                          [&s](float v) {
                              double dv = v;
                              uint64_t iv = *((uint64_t *)&dv);
                              s << "0x" << std::hex << std::uppercase << iv;
                          },
                          [&s, with_type](const std::vector<ConstantPtr> &v) {
                              s << "[";
                              for (size_t i = 0; i < v.size(); ++i)
                              {
                                  s << v[i]->to_string(with_type);
                                  if (i != v.size() - 1)
                                  {
                                      s << ", ";
                                  }
                              }
                              s << "]";
                          },
                          [&s](const ZeroInitializer &v) { s << "zeroinitializer"; }},
               kind);
    return s.str();
}

bool Constant::get_bool_value() const
{
    return !is_zero();
}
bool Constant::is_zero() const
{
    return std::visit(overloaded{[](const int const_int) { return const_int == 0; },
                                 [](const float const_float) { return const_float == 0.0f; },
                                 [](const auto const_zero) { return false; }},
                      kind);
}
} // namespace operand

Operand::Operand(OperandID id, OperandKind kind, TypePtr type) : id(id), kind(kind), type(type)
{
}

std::string Operand::to_string(bool with_type) const
{
    using namespace operand;
    std::string s;
    if (with_type && !is<ConstantPtr>())
    {
        s += type->to_string() + " ";
    }
    std::visit(overloaded{
                   [&s](const Global &op) { s += "@" + op.name; },
                   [&s, with_type](const ConstantPtr &op) { s += op->to_string(with_type); },
                   [&s](const Parameter &v) { s += "%" + v.name; },
                   [&s, this](const Local &) { s += "%t." + std::to_string(id); },
               },
               kind);
    return s;
}

void Operand::set_def(InstructionID def_id)
{
    maybe_def_id = def_id;
}

void Operand::add_use(InstructionID use_id)
{
    use_id_list.insert(use_id);
}

void Operand::remove_def()
{
    maybe_def_id == std::nullopt;
}

void Operand::remove_use(InstructionID use_id)
{
    use_id_list.erase(use_id);
}

bool Operand::is_int() const
{
    return type->is<type::Int>();
}

bool Operand::is_float() const
{
    return type->is<type::Float>();
}

bool Operand::is_zero() const
{
    return (is<operand::ConstantPtr>()) ? std::get<operand::ConstantPtr>(kind)->is_zero() : false;
}

} // namespace sed::ir
