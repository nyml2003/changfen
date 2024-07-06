#ifndef SED_BACKEND_INSTRUCTION_H_
#define SED_BACKEND_INSTRUCTION_H_

#include "backend/context.h"
#include "index.h"

namespace sed
{
namespace backend
{

namespace instruction
{
struct Load
{
    enum Op
    {
        LB,
        LBU,
        LH,
        LHU,
        LW,
        LD,
        LWU,
    };
    Op op;

    // Destination Register
    OperandID rd_id;
    // Source Register
    OperandID rs_id; // 手册里为rs1，与store的rs2区分
    // Immediate
    OperandID imm_id;
};

struct PseudoLoad
{
    enum Op
    {
        LA,
        LW,
    };
    Op op;

    // Destination Register
    OperandID rd_id;
    // Symbol(label), global..?
    OperandID symbol_id;
};

struct Store
{
    enum Op
    {
        SB,
        SH,
        SW,
        SD, // RV32I中没有
    };
    Op op;
    // Source Register
    OperandID rs1_id;
    // Base register
    OperandID rs2_id;
    // Immediate
    OperandID imm_id;
};

struct PseudoStore
{
    enum Op
    {
        SW,
    };
    Op op;
    // Source Register
    OperandID rd_id;
    OperandID symbol_id;
    // Base register
    OperandID rt_id;
    // Symbol(label), global..?
};

struct Binary
{
    enum Op
    {
        ADD,
        ADDW,
        SUB,
        SUBW,

        AND,
        OR,
        XOR,

        SLL, // shift left logical
        SLLW,
        SRL, // shift right logical
        SRLW,
        SRA, // shift right arithmetic
        SRAW,

        SLT,
        SLTU,

        MUL,
        MULW,
        MULH,
        MULHSU,
        MULHU,
        DIV,
        DIVU,
        DIVW,
        REM,
        REMU,
        REMW,
        REMUW,
    };
    Op op;

    // Destination Register
    OperandID rd_id;
    // Source Register
    OperandID rs1_id;
    // Source Register
    OperandID rs2_id;
};

struct BinaryImmediate
{
    enum Op
    {
        ADDI,
        ADDIW,

        SLLI,
        SLLIW,
        SRLI,
        SRLIW,
        SRAI,
        SRAIW,

        ANDI,
        ORI,
        XORI,
        SLTI,
        SLTIU,
    };
    Op op;

    OperandID rd_id;
    OperandID rs_id;
    OperandID imm_id;
};

struct FloatLoad
{
    enum Op
    {
        FLW,
        FLD,
    };
    Op op;
    OperandID rd_id;
    OperandID rs_id;
    OperandID imm_id;
};

struct FloatPseudoLoad
{
    enum Op
    {
        FLW,
    };
    Op op;
    OperandID rd_id;
    OperandID symbol_id;
    OperandID rt_id;
};

struct FloatStore
{
    enum Op
    {
        FSW,
        FSD,
    };
    Op op;
    OperandID rs1_id;
    OperandID rs2_id;
    OperandID imm_id;
};

struct FloatPseudoStore
{
    enum Op
    {
        FSW,
    };
    Op op;
    OperandID rd_id;
    OperandID symbol_id;
    OperandID rt_id;
};

struct FloatMove
{
    enum Mode
    {
        H,
        S,
        D,
        X,
    };
    Mode dest_mode, source_mode;

    OperandID rd_id;
    OperandID rs_id;
};

struct FloatConvert
{
    enum Mode
    {
        H,
        S,
        D,
        W,
        WU,
        L,
        LU,
    };
    Mode dest_mode, source_mode;
    OperandID rd_id;
    OperandID rs_id;
};

// 嗯哼，单精度
struct FloatBinary
{
    enum Op
    {
        FADD,
        FSUB,
        FMUL,
        FDIV,
        FMIN,
        FMAX,
        // compare
        FEQ,
        FLT,
        FLE,

        FSGNJ,
        FSGNJN,
        FSGNJX,
    };

    enum Mode
    {
        SINGLE,
        DOUBLE
    };
    Op op;
    Mode mode;

    OperandID rd_id;
    // Soucrce Register
    OperandID rs1_id;
    // Soucrce Register
    OperandID rs2_id;
};

struct FloatMultiplyAdd
{
    enum Op
    {
        FMADD,
        FMSUB,
        FNMADD,
        FNMSUB,
    };

    enum Mode
    {
        SINGLE,
        DOUBLE
    };
    Op op;
    Mode mode;

    OperandID rd_id;
    // Soucrce Register
    OperandID rs1_id;
    // Soucrce Register
    OperandID rs2_id;
    // Soucrce Register
    OperandID rs3_id;
};

struct FloatUnary
{
    enum Op
    {
        FCLASS,
        FSQRT
    };
    enum Mode
    {
        SINGLE,
        DOUBLE
    };
    Op op;
    Mode mode;

    OperandID rd_id;
    // Soucrce Register
    OperandID rs_id;
};

struct Lui
{
    OperandID rd_id;
    OperandID imm_id;
};

struct Li
{
    OperandID rd_id;
    OperandID imm_id;
};

struct Ret
{
};

struct Call
{
    std::string function_name;
};

struct Branch
{
    enum Op
    {
        BEQ,
        BNE,
        BLT,
        BGE,
        BLTU,
        BGEU,
    };
    Op op;
    OperandID rs1_id;
    OperandID rs2_id;

    // label
    BasicBlockID block_id;
};

struct Jump
{
    // 无条件跳转。
    BasicBlockID block_id;
};

struct Dummy
{
};

struct Phi
{
    OperandID rd_id;
    std::vector<std::tuple<OperandID, BasicBlockID>> incoming_list;
};

}; // namespace instruction

/// Machine instruction
struct Instruction : std::enable_shared_from_this<Instruction>
{
    /// Instruction ID in the context.
    InstructionID id;
    /// Instruction kind.
    InstructionKind kind;
    /// ID of the parent basic block.
    BasicBlockID parent_block_id;

    /// Next instruction.
    InstructionPtr next;
    /// Previous instruction.
    InstructionPrevPtr prev;

    std::vector<OperandID> def_id_list;
    std::vector<OperandID> use_id_list;

    void add_def(OperandID def_id);
    void add_use(OperandID use_id);

    /// Constructor
    Instruction(InstructionID id, InstructionKind kind, BasicBlockID parent_block_id);

    /// Insert instruction to the next of the current instruction.
    void insert_next(InstructionPtr instruction);
    /// Insert instruction to the prev of the current instruction.
    void insert_prev(InstructionPtr instruction);

    void remove(Context &context);

    void raw_remove();

    /// Convert the instruction to a string of assembly code.
    std::string to_string(Context &context);

    std::optional<BasicBlockID> get_basic_block_id_if_branch() const;

    void replace_operand(OperandID old_operand_id, OperandID new_operand_id, Context &context);

    void replace_def_operand(OperandID old_operand_id, OperandID new_operand_id, Context &context);

    void replace_use_operand(OperandID old_operand_id, OperandID new_operand_id, Context &context);

    bool is_phi() const;

    bool is_branch_or_jmp() const;

    bool is_binary() const;

    bool is_binary_imm() const;

    bool is_store() const;

    bool is_load() const;

    bool is_float_store() const;

    bool is_float_load() const;

    bool is_li() const;

    bool is_lui() const;

    template <typename T> std::optional<T> as()
    {
        if (std::holds_alternative<T>(this->kind))
        {
            return std::get<T>(this->kind);
        }
        else
        {
            return std::nullopt;
        }
    }

    template <typename T> std::optional<std::reference_wrapper<T>> as_ref()
    {
        if (std::holds_alternative<T>(this->kind))
        {
            return std::get<T>(this->kind);
        }
        else
        {
            return std::nullopt;
        }
    }
};

InstructionPtr create_instruction(InstructionID id, InstructionKind kind, BasicBlockID parent_block_id);

InstructionPtr create_dummy_instruction();

} // namespace backend
} // namespace sed

#endif