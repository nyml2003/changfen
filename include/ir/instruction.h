#ifndef SED_IR_INSTRUCTION_H
#define SED_IR_INSTRUCTION_H

#include "index.h"

namespace sed::ir
{
namespace instruction
{
enum class BinaryOp
{
    // Integer operator
    IAdd,
    ISub,
    IMul,
    IDiv,
    Mod,

    // Float operator
    FAdd,
    FSub,
    FMul,
    FDiv,

    // Logic operator
    Sll,
    Srl,
    Sra,
};
struct Binary
{
    BinaryOp op;

    OperandID dst_id;
    OperandID lsrc_id;
    OperandID rsrc_id;

    bool operator==(const Binary &other) const;
};

enum class ICmpOp
{
    Eq,
    Ne,
    Lt,
    Le,
};
struct ICmp
{
    ICmpOp op;
    OperandID dst_id;
    OperandID lsrc_id;
    OperandID rsrc_id;

    bool operator==(const ICmp &other) const;
};

enum class FCmpOp
{
    // ordered cmp
    Eq,
    Ne,
    Lt,
    Le,
};
struct FCmp
{
    FCmpOp op;
    OperandID dst_id;
    OperandID lsrc_id;
    OperandID rsrc_id;

    bool operator==(const FCmp &other) const;
};

enum class CastOp
{
    F2I,
    I2F,
    Ext,
    Bitcast,
};
struct Cast
{
    CastOp op;
    OperandID dst_id;
    OperandID src_id;

    bool operator==(const Cast &other) const;
};

struct Ret
{
    std::optional<OperandID> maybe_value_id;

    bool operator==(const Ret &other) const;
};

struct Branch
{
    OperandID cond_id;
    BasicBlockID then_block_id;
    BasicBlockID else_block_id;

    bool operator==(const Branch &other) const;
};

struct Jump
{
    BasicBlockID then_block_id;

    bool operator==(const Jump &other) const;
};

struct Phi
{
    OperandID dst_id;
    std::set<std::tuple<OperandID, BasicBlockID>> src_id_list;

    bool operator==(const Phi &other) const;
};

struct Alloca
{
    OperandID dst_id;
    TypePtr allocated_type;
    std::optional<OperandID> maybe_size_id;
    std::optional<OperandID> maybe_align_id;
    std::optional<OperandID> maybe_addrspace_id;
    bool alloca_for_param;

    bool operator==(const Alloca &other) const;
};

struct GetElementPtr
{
    OperandID dst_id;
    TypePtr basis_type;
    OperandID ptr_id;
    std::vector<OperandID> index_id_list;

    bool operator==(const GetElementPtr &other) const;
};

struct Load
{
    OperandID dst_id;
    OperandID ptr_id;
    std::optional<OperandID> maybe_align_id;

    bool operator==(const Load &other) const;
};

struct Store
{
    OperandID value_id;
    OperandID ptr_id;
    std::optional<OperandID> maybe_align_id;

    bool operator==(const Store &other) const;
};

struct Call
{
    std::optional<OperandID> maybe_dst_id;
    std::string function_name;
    std::vector<OperandID> arg_id_list;

    bool operator==(const Call &other) const;
};

struct Dummy
{
    bool operator==(const Dummy &other) const;
};
} // namespace instruction

struct Instruction : std::enable_shared_from_this<Instruction>
{
    InstructionID id;
    InstructionKind kind;
    BasicBlockID parent_block_id;
    InstructionPtr next;
    InstructionPrevPtr prev;
    std::optional<OperandID> maybe_def_id;
    std::set<OperandID> use_id_list;

    Instruction(InstructionID id, InstructionKind kind, BasicBlockID parent_block_id);

    void insert_next(InstructionPtr inst);
    void insert_prev(InstructionPtr inst);
    void set_def(OperandID def_id);
    void add_use(OperandID use_id);
    void remove_use(OperandID use_id);

    void replace_operand(OperandID old_id, OperandID new_id, Context &context);
    void remove(Context &context);
    void raw_remove();

    bool is_terminator() const;

    void add_phi_operand(OperandID incoming_operand_id, BasicBlockID incoming_block_id, Context &context);
    std::optional<OperandID> remove_phi_operand(BasicBlockID incoming_block_id, Context &context);
    template <typename T> inline bool is() const
    {
        return std::holds_alternative<T>(kind);
    }

    template <typename T> std::optional<T> as()
    {
        if (std::holds_alternative<T>(kind))
        {
            return std::get<T>(kind);
        }
        else
        {
            return std::nullopt;
        }
    }

    bool operator==(const Instruction &other) const;
    std::string to_string(Context &context) const;
};

InstructionPtr create_instruction(InstructionID id, InstructionKind kind, BasicBlockID parent_block_id);

InstructionPtr create_dummy_instruction();

} // namespace sed::ir

#endif // SED_IR_INSTRUCTION_H
