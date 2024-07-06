#ifndef SED_BACKEND_BUILDER_H_
#define SED_BACKEND_BUILDER_H_

#include "backend/basic_block.h"
#include "backend/context.h"
#include "backend/function.h"
#include "backend/global.h"
#include "backend/immediate.h"
#include "backend/instruction.h"
#include "backend/operand.h"
#include "backend/register.h"
#include "index.h"

namespace sed
{
namespace backend
{
class Builder
{
  public:
    Context context;
    FunctionPtr now_function;
    BasicBlockPtr now_basic_block;
    std::unordered_map<Register, OperandID, RegisterHash> register2operand;

    Builder() = default;

    OperandID fetch_operand(OperandKind kind, Modifier modifier);
    OperandID fetch_immediate(ImmediateValue value);
    OperandID fetch_global(std::string name, GlobalValue value);
    OperandID fetch_local_memory(int offset, Register reg);

    OperandID fetch_register(Register reg);
    OperandID fetch_virtual_register(VirtualRegisterKind kind);

    BasicBlockPtr fetch_basic_block();

    // fetch instruction
    // add use&def
    InstructionPtr fetch_load_instruction(instruction::Load::Op op, OperandID rd_id, OperandID rs_id, OperandID imm_id);
    InstructionPtr fetch_pseudo_load_instruction(instruction::PseudoLoad::Op op, OperandID rd_id, OperandID symbol_id);
    InstructionPtr fetch_store_instruction(instruction::Store::Op op, OperandID rs1_id, OperandID rs2_id,
                                           OperandID imm_id);
    InstructionPtr fetch_pseudo_store_instruction(instruction::PseudoStore::Op op, OperandID rd_id, OperandID symbol_id,
                                                  OperandID rt_id);

    InstructionPtr fetch_binary_instruction(instruction::Binary::Op op, OperandID rd_id, OperandID rs1_id,
                                            OperandID rs2_id);
    InstructionPtr fetch_binary_immediate_instruction(instruction::BinaryImmediate::Op op, OperandID rd_id,
                                                      OperandID rs1_id, OperandID imm_id);

    InstructionPtr fetch_float_load_instruction(instruction::FloatLoad::Op op, OperandID rd_id, OperandID rs1_id,
                                                OperandID imm_id);
    InstructionPtr fetch_float_pseudo_load_instruction(instruction::FloatPseudoLoad::Op op, OperandID rd_id,
                                                       OperandID symbol_id, OperandID rt_id);
    InstructionPtr fetch_float_store_instruction(instruction::FloatStore::Op op, OperandID rs1_id, OperandID rs2_id,
                                                 OperandID imm_id);
    InstructionPtr fetch_float_pseudo_store_instruction(instruction::FloatPseudoStore::Op op, OperandID rd_id,
                                                        OperandID symbol_id, OperandID rt_id);

    InstructionPtr fetch_float_move_instruction(instruction::FloatMove::Mode dest_mode,
                                                instruction::FloatMove::Mode source_mode, OperandID rd_id,
                                                OperandID rs1_id);
    InstructionPtr fetch_float_convert_instruction(instruction::FloatConvert::Mode dest_mode,
                                                   instruction::FloatConvert::Mode source_mode, OperandID rd_id,
                                                   OperandID rs1_id);
    InstructionPtr fetch_float_binary_instruction(instruction::FloatBinary::Op op, instruction::FloatBinary::Mode mode,
                                                  OperandID rd_id, OperandID rs1_id, OperandID rs2_id);
    InstructionPtr fetch_float_multiply_add_instruction(instruction::FloatMultiplyAdd::Op op,
                                                        instruction::FloatMultiplyAdd::Mode mode, OperandID rd_id,
                                                        OperandID rs1_id, OperandID rs2_id, OperandID rs3_id);
    InstructionPtr fetch_float_unary_instruction(instruction::FloatUnary::Op op, instruction::FloatUnary::Mode mode,
                                                 OperandID rd_id, OperandID rs1_id);

    InstructionPtr fetch_lui_instruction(OperandID rd_id, OperandID imm_id);
    InstructionPtr fetch_li_instruction(OperandID rd_id, OperandID imm_id);
    InstructionPtr fetch_ret_instruction();
    InstructionPtr fetch_call_instruction(std::string name, std::set<Register> args_reg_set);
    InstructionPtr fetch_branch_instruction(instruction::Branch::Op op, OperandID rs1_id, OperandID rs2_id,
                                            BasicBlockID block_id);
    InstructionPtr fetch_jump_instruction(BasicBlockID block_id);
    InstructionPtr fetch_phi_instruction(OperandID rd_id,
                                         std::vector<std::tuple<OperandID, BasicBlockID>> incoming_list);

    void add_instruction_front(InstructionPtr instruction);
    void add_instruction_back(InstructionPtr instruction);

    // current status switch
    void set_now_basic_block(BasicBlockPtr basicblock);
    void set_now_function(FunctionPtr function);
    void set_now_function(std::string name);

    void add_function(std::string name);
};

} // namespace backend
} // namespace sed

#endif