#include "backend/operand.h"

namespace sed
{
namespace backend
{

std::string Operand::to_string(int width) const
{
    std::string result;

    if (modifier == Modifier::LO)
    {
        result += "%%lo(";
    }
    else if (modifier == Modifier::HI)
    {
        result += "%%hi(";
    }

    std::visit(overloaded{
                   [&](const Immediate &imm) { result += imm.to_string(width); },
                   [&](const Register &reg) { result += reg.to_string(); },
                   [&](const VirtualRegister &reg) { result += reg.to_string(); },
                   [&](const Global &global) { result += global.name; },
                   [&](const LocalMemory &local) {
                       result += "offset " + std::to_string(local.offset) + " from " + local.reg.to_string();
                   },
               },
               kind);

    if (modifier != Modifier::NONE)
    {
        result += ")";
    }

    return result;
}

void Operand::add_def(InstructionID def_id)
{
    def_id_list.push_back(def_id);
}
void Operand::add_use(InstructionID use_id)
{
    use_id_list.push_back(use_id);
}

void Operand::remove_def(InstructionID def_id)
{
    def_id_list.erase(std::remove(def_id_list.begin(), def_id_list.end(), def_id), def_id_list.end());
}

void Operand::remove_use(InstructionID use_id)
{
    use_id_list.erase(std::remove(use_id_list.begin(), use_id_list.end(), use_id), use_id_list.end());
}

bool Operand::is_virtual_reg() const
{
    return std::holds_alternative<VirtualRegister>(kind);
}
bool Operand::is_reg() const
{
    return std::holds_alternative<Register>(kind);
}

bool Operand::is_immediate() const
{
    return std::holds_alternative<Immediate>(kind);
}

bool Operand::is_global() const
{
    return std::holds_alternative<Global>(kind);
}

bool Operand::is_local_memory() const
{
    return std::holds_alternative<LocalMemory>(kind);
}

bool Operand::is_float() const
{
    if (is_reg())
    {
        return std::get<Register>(kind).is_float();
    }
    else if (is_virtual_reg())
    {
        return std::get<VirtualRegister>(kind).is_float();
    }
    return false;
}

bool Operand::is_zero() const
{
    if (is_immediate())
    {
        return std::get<Immediate>(kind).is_zero();
    }
    else if (is_reg())
    {
        auto reg = std::get<Register>(kind);
        if (reg.is_general())
        {
            return std::get<GeneralRegister>(reg.reg) == GeneralRegister::ZERO;
        }
    }
    return false;
}

bool Operand::is_sp() const
{
    if (is_reg())
    {
        auto reg = std::get<Register>(kind);

        if (reg.is_general())
        {
            return std::get<GeneralRegister>(reg.reg) == GeneralRegister::SP;
        }
    }
    return false;
}

bool Operand::operator==(const Operand &other) const
{
    if (this->is_virtual_reg() && other.is_virtual_reg())
    {
        return this->id == other.id;
    }
    else if (this->is_reg() && other.is_reg())
    {
        const auto &lhs_reg = std::get<Register>(this->kind);
        const auto &rhs_reg = std::get<Register>(other.kind);

        if (lhs_reg.reg.index() != rhs_reg.reg.index())
        {
            return false;
        }

        if (lhs_reg.is_general())
        {
            return std::get<GeneralRegister>(lhs_reg.reg) == std::get<GeneralRegister>(rhs_reg.reg);
        }
        else
        {
            return std::get<FloatRegister>(lhs_reg.reg) == std::get<FloatRegister>(rhs_reg.reg);
        }
    }
    return false;
}

} // namespace backend
} // namespace sed