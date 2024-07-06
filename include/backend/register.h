#ifndef SED_BACKEND_REGISTER_H_
#define SED_BACKEND_REGISTER_H_

#include "index.h"

namespace sed
{
namespace backend
{

/// RISC-V general purpose registers.
enum class GeneralRegister
{
    ZERO = 0,
    RA = 1, // Return Address
    SP = 2,
    GP = 3, // Global Pointer
    TP = 4, // Thread Pointer
    T0 = 5, // Temporary
    T1 = 6,
    T2 = 7,
    S0 = 8, // [saved] Frame pointer. maybe it's not used?
    S1 = 9, // [saved]

    A0 = 10,
    A1 = 11,
    A2 = 12,
    A3 = 13,
    A4 = 14,
    A5 = 15,
    A6 = 16,
    A7 = 17,
    // [saved]
    S2 = 18,
    S3 = 19,
    S4 = 20,
    S5 = 21,
    S6 = 22,
    S7 = 23,
    S8 = 24,
    S9 = 25,
    S10 = 26,
    S11 = 27,
    T3 = 28,
    T4 = 29,
    T5 = 30,
    T6 = 31
};

// RISC-V Float Point registers.
enum class FloatRegister
{
    // FP Temporaries
    FT0 = 0,
    FT1 = 1,
    FT2 = 2,
    FT3 = 3,
    FT4 = 4,
    FT5 = 5,
    FT6 = 6,
    FT7 = 7,
    // FP Saved Registers
    FS0 = 8,
    FS1 = 9,
    // FP Arguments(RETURN VALUE)
    FA0 = 10,
    FA1 = 11,
    //
    FA2 = 12,
    FA3 = 13,
    FA4 = 14,
    FA5 = 15,
    FA6 = 16,
    FA7 = 17,

    FS2 = 18,
    FS3 = 19,
    FS4 = 20,
    FS5 = 21,
    FS6 = 22,
    FS7 = 23,
    FS8 = 24,
    FS9 = 25,
    FS10 = 26,
    FS11 = 27,

    FT8 = 28,
    FT9 = 29,
    FT10 = 30,
    FT11 = 31,
};

class Register
{
  public:
    std::variant<GeneralRegister, FloatRegister> reg;
    Register() = default;
    Register(GeneralRegister reg) : reg(reg)
    {
    }
    Register(FloatRegister reg) : reg(reg)
    {
    }

    bool is_general() const
    {
        return std::holds_alternative<GeneralRegister>(reg);
    };

    bool is_float() const
    {
        return std::holds_alternative<FloatRegister>(reg);
    };

    std::string to_string() const;
};

struct RegisterHash
{
    std::size_t operator()(const Register &reg) const
    {
        if (reg.is_general())
        {
            return (size_t)std::get<GeneralRegister>(reg.reg);
        }
        else
        {
            return (size_t)std::get<FloatRegister>(reg.reg) + 32;
        }
    }
};

bool operator==(const Register &lhs, const Register &rhs);
bool operator<(const Register &lhs, const Register &rhs);

/// Virtual register kind.
enum class VirtualRegisterKind
{
    /// General purpose register.
    GENERAL,
    /// Floating point register.
    FLOAT,
};

/// Virtual register.
class VirtualRegister
{
  public:
    VirtualRegisterID id;
    VirtualRegisterKind kind;

    VirtualRegister(VirtualRegisterID id, VirtualRegisterKind kind) : id(id), kind(kind)
    {
    }

    bool is_general() const
    {
        return kind == VirtualRegisterKind::GENERAL;
    };

    bool is_float() const
    {
        return kind == VirtualRegisterKind::FLOAT;
    };

    std::string to_string() const;
};

#define GENERAL_REG(X) (Register(GeneralRegister::X))
#define FLOAT_REG(X) (Register(FloatRegister::X))

const std::vector<Register> REG_ALLOC = {
    GENERAL_REG(A0),
    GENERAL_REG(A1),
    GENERAL_REG(A2),
    GENERAL_REG(A3),
    GENERAL_REG(A4),
    GENERAL_REG(A5),
    GENERAL_REG(A6),
    GENERAL_REG(A7),

    FLOAT_REG(FA0),
    FLOAT_REG(FA1),
    FLOAT_REG(FA2),
    FLOAT_REG(FA3),
    FLOAT_REG(FA4),
    FLOAT_REG(FA5),
    FLOAT_REG(FA6),
    FLOAT_REG(FA7),

    // GENERAL_REG(T0), GENERAL_REG(T1), GENERAL_REG(T2),
    GENERAL_REG(T3),
    GENERAL_REG(T4),
    GENERAL_REG(T5),
    GENERAL_REG(T6),

    // FLOAT_REG(FT0), FLOAT_REG(FT1), FLOAT_REG(FT2),
    FLOAT_REG(FT3),
    FLOAT_REG(FT4),
    FLOAT_REG(FT5),
    FLOAT_REG(FT6),
    FLOAT_REG(FT7),
    FLOAT_REG(FT8),
    FLOAT_REG(FT9),
    FLOAT_REG(FT10),
    FLOAT_REG(FT11),

    GENERAL_REG(S1),
    GENERAL_REG(S2),
    GENERAL_REG(S3),
    GENERAL_REG(S4),
    GENERAL_REG(S5),
    GENERAL_REG(S6),
    GENERAL_REG(S7),
    GENERAL_REG(S8),
    GENERAL_REG(S9),
    GENERAL_REG(S10),
    GENERAL_REG(S11),

    FLOAT_REG(FS0),
    FLOAT_REG(FS1),
    FLOAT_REG(FS2),
    FLOAT_REG(FS3),
    FLOAT_REG(FS4),
    FLOAT_REG(FS5),
    FLOAT_REG(FS6),
    FLOAT_REG(FS7),
    FLOAT_REG(FS8),
    FLOAT_REG(FS9),
    FLOAT_REG(FS10),
    FLOAT_REG(FS11),
};

const std::vector<Register> REG_SPILL_GENERAL = {
    GENERAL_REG(T0),
    GENERAL_REG(T1),
};

const std::vector<Register> REG_SPILL_FLOAT = {
    FLOAT_REG(FT0),
    FLOAT_REG(FT1),
    FLOAT_REG(FT2),
};

const std::set<Register> REG_CALLEE_SAVED = {
    GENERAL_REG(RA), GENERAL_REG(SP), GENERAL_REG(GP), GENERAL_REG(TP),  GENERAL_REG(S0),

    GENERAL_REG(S1), GENERAL_REG(S2), GENERAL_REG(S3), GENERAL_REG(S4),  GENERAL_REG(S5),  GENERAL_REG(S6),
    GENERAL_REG(S7), GENERAL_REG(S8), GENERAL_REG(S9), GENERAL_REG(S10), GENERAL_REG(S11),

    FLOAT_REG(FS0),  FLOAT_REG(FS1),  FLOAT_REG(FS2),  FLOAT_REG(FS3),   FLOAT_REG(FS4),   FLOAT_REG(FS5),
    FLOAT_REG(FS6),  FLOAT_REG(FS7),  FLOAT_REG(FS8),  FLOAT_REG(FS9),   FLOAT_REG(FS10),  FLOAT_REG(FS11),
};

// register set
const std::set<Register> REG_ARGS = {
    GENERAL_REG(A0), GENERAL_REG(A1), GENERAL_REG(A2), GENERAL_REG(A3), GENERAL_REG(A4), GENERAL_REG(A5),
    GENERAL_REG(A6), GENERAL_REG(A7), FLOAT_REG(FA0),  FLOAT_REG(FA1),  FLOAT_REG(FA2),  FLOAT_REG(FA3),
    FLOAT_REG(FA4),  FLOAT_REG(FA5),  FLOAT_REG(FA6),  FLOAT_REG(FA7),
};

const std::set<Register> REG_TEMP = {
    GENERAL_REG(T0), GENERAL_REG(T1), GENERAL_REG(T2), GENERAL_REG(T3), GENERAL_REG(T4),
    GENERAL_REG(T5), GENERAL_REG(T6), FLOAT_REG(FT0),  FLOAT_REG(FT1),  FLOAT_REG(FT2),
    FLOAT_REG(FT3),  FLOAT_REG(FT4),  FLOAT_REG(FT5),  FLOAT_REG(FT6),  FLOAT_REG(FT7),
    FLOAT_REG(FT8),  FLOAT_REG(FT9),  FLOAT_REG(FT10), FLOAT_REG(FT11),
};

} // namespace backend
} // namespace sed

#endif