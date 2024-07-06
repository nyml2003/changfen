#ifndef SED_IR_BUILDER_H
#define SED_IR_BUILDER_H

#include "index.h"
#include "ir/context.h"

namespace sed::ir
{
struct Builder
{
    Context context;

    FunctionPtr curr_function;
    BasicBlockPtr curr_block;

    /// Stack to record the condition basic block of while statement.
    std::stack<BasicBlockPtr> while_cond_basic_block_stack;

    /// Stack to record the tail basic block of while statement.
    std::stack<BasicBlockPtr> while_tail_basic_block_stack;

    Builder() = default;

    TypePtr fetch_int_type(const int size);
    TypePtr fetch_float_type();
    TypePtr fetch_void_type();
    TypePtr fetch_pointer_type();
    TypePtr fetch_array_type(size_t length, TypePtr element_type);

    OperandID fetch_operand(TypePtr type, OperandKind kind);
    operand::ConstantPtr fetch_constant(TypePtr type, operand::ConstantKind kind);
    OperandID fetch_constant_operand(TypePtr type, operand::ConstantKind kind);
    OperandID fetch_global_operand(TypePtr type, std::string name, bool is_constant, OperandID init);
    OperandID fetch_parameter_operand(TypePtr type, std::string name);
    OperandID fetch_local_operand(TypePtr type);

    InstructionPtr fetch_binary_instruction(instruction::BinaryOp op, OperandID dst_id, OperandID lsrc_id,
                                            OperandID rsrc_id);

    InstructionPtr fetch_icmp_instruction(instruction::ICmpOp op, OperandID dst_id, OperandID lsrc_id,
                                          OperandID rsrc_id);

    InstructionPtr fetch_fcmp_instruction(instruction::FCmpOp op, OperandID dst_id, OperandID lsrc_id,
                                          OperandID rsrc_id);

    InstructionPtr fetch_cast_instruction(instruction::CastOp op, OperandID dst_id, OperandID src_id);

    InstructionPtr fetch_ret_instruction(std::optional<OperandID> maybe_value_id = std::nullopt);

    InstructionPtr fetch_branch_instruction(OperandID cond_id, BasicBlockID then_block_id, BasicBlockID else_block_id);

    InstructionPtr fetch_jump_instruction(BasicBlockID then_block_id);

    InstructionPtr fetch_phi_instruction(OperandID dst_id, std::set<std::tuple<OperandID, BasicBlockID>> src_id_list);

    InstructionPtr fetch_alloca_instruction(OperandID dst_id, TypePtr allocated_type,
                                            std::optional<OperandID> maybe_size_id,
                                            std::optional<OperandID> maybe_align_id,
                                            std::optional<OperandID> maybe_addrspace_id, bool alloca_for_param = false);

    InstructionPtr fetch_load_instruction(OperandID dst_id, OperandID ptr_id, std::optional<OperandID> maybe_align_id);

    InstructionPtr fetch_store_instruction(OperandID value_id, OperandID ptr_id,
                                           std::optional<OperandID> maybe_align_id);

    InstructionPtr fetch_call_instruction(std::optional<OperandID> maybe_dst_id, std::string function_name,
                                          std::vector<OperandID> arg_id_list);

    InstructionPtr fetch_getelementptr_instruction(OperandID dst_id, TypePtr basis_type, OperandID ptr_id,
                                                   std::vector<OperandID> index_id_list);

    void append_instruction(InstructionPtr inst);

    void prepend_instruction_to_curr_block(InstructionPtr inst);

    void prepend_instruction_to_curr_function(InstructionPtr inst);

    BasicBlockPtr fetch_basic_block();

    void append_basic_block(BasicBlockPtr block);

    void set_curr_block(BasicBlockPtr block);

    void switch_function(std::string function_name);

    void add_function(std::string function_name, std::vector<OperandID> param_id_list, TypePtr return_type,
                      bool is_declare);
};
} // namespace sed::ir

#endif // SED_IR_BUILDER_H