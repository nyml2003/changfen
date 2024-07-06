#ifndef SED_BACKEND_OPERAND_H_
#define SED_BACKEND_OPERAND_H_

#include "backend/global.h"
#include "backend/immediate.h"
#include "backend/register.h"
#include "index.h"

namespace sed
{
namespace backend
{

/// Local memory place for variables.
class LocalMemory
{
  public:
    LocalMemory(int offset, Register reg) : offset(offset), reg(reg){};
    int offset;
    Register reg;
};

enum class Modifier
{
    NONE,
    LO,
    HI,
};

class Operand
{
  public:
    std::vector<InstructionID> def_id_list;
    std::vector<InstructionID> use_id_list;
    OperandID id;
    OperandKind kind;
    Modifier modifier;
    Operand(OperandID id, OperandKind kind, Modifier modifier) : id(id), kind(kind), modifier(modifier){};

    // TODO: USE DEF,优化部分
    void add_def(InstructionID def_id);
    void add_use(InstructionID use_id);

    void remove_def(InstructionID def_id);
    void remove_use(InstructionID use_id);

    std::string to_string(int width = 0) const;

    bool is_virtual_reg() const;
    bool is_reg() const;
    bool is_local_memory() const;
    bool is_immediate() const;
    bool is_global() const;
    bool is_float() const;
    bool is_zero() const;
    bool is_sp() const;

    bool operator==(const Operand &other) const;
};

} // namespace backend
} // namespace sed

#endif