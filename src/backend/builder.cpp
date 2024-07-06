#include "backend/builder.h"
#include "index.h"

namespace sed
{
namespace backend
{

OperandID Builder::fetch_operand(OperandKind kind, Modifier modifier)
{
    auto id = context.create_operand_id();
    auto operand = std::make_shared<Operand>(id, kind, modifier);
    context.register_operand(operand);
    return id;
}

OperandID Builder::fetch_global(std::string name, GlobalValue value)
{
    auto kind = Global(name, value);
    return fetch_operand(kind, Modifier::NONE);
}

OperandID Builder::fetch_immediate(ImmediateValue value)
{
    auto kind = Immediate(value);
    return fetch_operand(kind, Modifier::NONE);
}

OperandID Builder::fetch_local_memory(int offset, Register reg)
{
    auto kind = LocalMemory(offset, reg);
    return fetch_operand(kind, Modifier::NONE);
}

OperandID Builder::fetch_virtual_register(VirtualRegisterKind kind)
{
    auto vred_id = context.create_virtual_register_id();
    auto vreg = VirtualRegister(vred_id, kind);
    return fetch_operand(vreg, Modifier::NONE);
}

BasicBlockPtr Builder::fetch_basic_block()
{
    auto id = context.create_basic_block_id();
    auto basic_block = create_basic_block(id, now_function->name);

    context.register_basic_block(basic_block);

    return basic_block;
}

OperandID Builder::fetch_register(Register reg)
{
    if (register2operand.count(reg))
    {
        return register2operand.at(reg);
    }
    else
    {
        auto operand_id = fetch_operand(reg, Modifier::NONE);
        register2operand[reg] = operand_id;
        return operand_id;
    }
}

InstructionPtr Builder::fetch_load_instruction(instruction::Load::Op op, OperandID rd_id, OperandID rs_id,
                                               OperandID imm_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::Load{op, rd_id, rs_id, imm_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);
    context.operand_table[rd_id]->add_def(instruction->id);
    context.operand_table[rs_id]->add_use(instruction->id);

    instruction->add_def(rd_id);
    instruction->add_use(rs_id);

    return instruction;
}

InstructionPtr Builder::fetch_float_load_instruction(instruction::FloatLoad::Op op, OperandID rd_id, OperandID rs_id,
                                                     OperandID imm_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::FloatLoad{op, rd_id, rs_id, imm_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_def(instruction->id);
    context.operand_table[rs_id]->add_use(instruction->id);

    instruction->add_def(rd_id);
    instruction->add_use(rs_id);

    return instruction;
}

InstructionPtr Builder::fetch_store_instruction(instruction::Store::Op op, OperandID rs1_id, OperandID rs2_id,
                                                OperandID imm_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::Store{op, rs1_id, rs2_id, imm_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rs1_id]->add_use(instruction->id);
    context.operand_table[rs2_id]->add_use(instruction->id);

    instruction->add_use(rs1_id);
    instruction->add_use(rs2_id);

    return instruction;
}

InstructionPtr Builder::fetch_float_store_instruction(instruction::FloatStore::Op op, OperandID rs1_id,
                                                      OperandID rs2_id, OperandID imm_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::FloatStore{op, rs1_id, rs2_id, imm_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rs1_id]->add_use(instruction->id);
    context.operand_table[rs2_id]->add_use(instruction->id);

    instruction->add_use(rs1_id);
    instruction->add_use(rs2_id);

    return instruction;
}

InstructionPtr Builder::fetch_pseudo_load_instruction(instruction::PseudoLoad::Op op, OperandID rd_id,
                                                      OperandID symbol_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::PseudoLoad{op, rd_id, symbol_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_def(instruction->id);

    instruction->add_def(rd_id);

    return instruction;
}
InstructionPtr Builder::fetch_pseudo_store_instruction(instruction::PseudoStore::Op op, OperandID rd_id,
                                                       OperandID symbol_id, OperandID rt_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::PseudoStore{op, rd_id, symbol_id, rt_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_use(instruction->id);
    context.operand_table[rt_id]->add_def(instruction->id);
    context.operand_table[rt_id]->add_use(instruction->id);

    instruction->add_use(rd_id);
    instruction->add_def(rt_id);
    instruction->add_use(rt_id);

    return instruction;
}

InstructionPtr Builder::fetch_float_pseudo_load_instruction(instruction::FloatPseudoLoad::Op op, OperandID rd_id,
                                                            OperandID symbol_id, OperandID rt_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::FloatPseudoLoad{op, rd_id, symbol_id, rt_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_def(instruction->id);
    context.operand_table[rt_id]->add_def(instruction->id);
    context.operand_table[rt_id]->add_use(instruction->id);

    instruction->add_def(rd_id);
    instruction->add_def(rt_id);
    instruction->add_use(rt_id);

    return instruction;
}

InstructionPtr Builder::fetch_float_pseudo_store_instruction(instruction::FloatPseudoStore::Op op, OperandID rd_id,
                                                             OperandID symbol_id, OperandID rt_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::FloatPseudoStore{op, rd_id, symbol_id, rt_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_use(instruction->id);
    context.operand_table[rt_id]->add_def(instruction->id);
    context.operand_table[rt_id]->add_use(instruction->id);

    instruction->add_use(rd_id);
    instruction->add_def(rt_id);
    instruction->add_use(rt_id);

    return instruction;
}

InstructionPtr Builder::fetch_float_move_instruction(instruction::FloatMove::Mode dest_mode,
                                                     instruction::FloatMove::Mode source_mode, OperandID rd_id,
                                                     OperandID rs_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::FloatMove{dest_mode, source_mode, rd_id, rs_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_def(instruction->id);
    context.operand_table[rs_id]->add_use(instruction->id);

    instruction->add_def(rd_id);
    instruction->add_use(rs_id);

    return instruction;
}

InstructionPtr Builder::fetch_float_convert_instruction(instruction::FloatConvert::Mode dest_mode,
                                                        instruction::FloatConvert::Mode source_mode, OperandID rd_id,
                                                        OperandID rs_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::FloatConvert{dest_mode, source_mode, rd_id, rs_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_def(instruction->id);
    context.operand_table[rs_id]->add_use(instruction->id);

    instruction->add_def(rd_id);
    instruction->add_use(rs_id);

    return instruction;
}

InstructionPtr Builder::fetch_binary_instruction(instruction::Binary::Op op, OperandID rd_id, OperandID rs1_id,
                                                 OperandID rs2_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::Binary{op, rd_id, rs1_id, rs2_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_def(instruction->id);
    context.operand_table[rs1_id]->add_use(instruction->id);
    context.operand_table[rs2_id]->add_use(instruction->id);

    instruction->add_def(rd_id);
    instruction->add_use(rs1_id);
    instruction->add_use(rs2_id);

    return instruction;
}

InstructionPtr Builder::fetch_binary_immediate_instruction(instruction::BinaryImmediate::Op op, OperandID rd_id,
                                                           OperandID rs_id, OperandID imm_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::BinaryImmediate{op, rd_id, rs_id, imm_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_def(instruction->id);
    context.operand_table[rs_id]->add_use(instruction->id);

    instruction->add_def(rd_id);
    instruction->add_use(rs_id);

    return instruction;
}

InstructionPtr Builder::fetch_float_binary_instruction(instruction::FloatBinary::Op op,
                                                       instruction::FloatBinary::Mode mode, OperandID rd_id,
                                                       OperandID rs1_id, OperandID rs2_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::FloatBinary{op, mode, rd_id, rs1_id, rs2_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_def(instruction->id);
    context.operand_table[rs1_id]->add_use(instruction->id);
    context.operand_table[rs2_id]->add_use(instruction->id);

    instruction->add_def(rd_id);
    instruction->add_use(rs1_id);
    instruction->add_use(rs2_id);

    return instruction;
}

InstructionPtr Builder::fetch_float_multiply_add_instruction(instruction::FloatMultiplyAdd::Op op,
                                                             instruction::FloatMultiplyAdd::Mode mode, OperandID rd_id,
                                                             OperandID rs1_id, OperandID rs2_id, OperandID rs3_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::FloatMultiplyAdd{op, mode, rd_id, rs1_id, rs2_id, rs3_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_def(instruction->id);
    context.operand_table[rs1_id]->add_use(instruction->id);
    context.operand_table[rs2_id]->add_use(instruction->id);
    context.operand_table[rs3_id]->add_use(instruction->id);

    instruction->add_def(rd_id);
    instruction->add_use(rs1_id);
    instruction->add_use(rs2_id);
    instruction->add_use(rs3_id);

    return instruction;
}

InstructionPtr Builder::fetch_float_unary_instruction(instruction::FloatUnary::Op op,
                                                      instruction::FloatUnary::Mode mode, OperandID rd_id,
                                                      OperandID rs_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::FloatUnary{op, mode, rd_id, rs_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_def(instruction->id);
    context.operand_table[rs_id]->add_use(instruction->id);

    instruction->add_def(rd_id);
    instruction->add_use(rs_id);

    return instruction;
}

InstructionPtr Builder::fetch_lui_instruction(OperandID rd_id, OperandID imm_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::Lui{rd_id, imm_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_def(instruction->id);

    instruction->add_def(rd_id);

    return instruction;
}
InstructionPtr Builder::fetch_li_instruction(OperandID rd_id, OperandID imm_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::Li{rd_id, imm_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_def(instruction->id);

    instruction->add_def(rd_id);

    return instruction;
}

InstructionPtr Builder::fetch_call_instruction(std::string name, std::set<Register> args_reg_set)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::Call{name});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    // TODO: 加什么？

    for (auto reg : REG_ARGS)
    {
        auto operator_id = fetch_register(reg);
        instruction->add_def(operator_id);
        context.operand_table[operator_id]->add_def(instruction->id);
    }

    for (auto reg : args_reg_set)
    {
        auto operator_id = fetch_register(reg);
        instruction->add_use(operator_id);
        context.operand_table[operator_id]->add_use(instruction->id);
    }

    for (auto reg : REG_TEMP)
    {
        auto operator_id = fetch_register(reg);
        instruction->add_def(operator_id);
        context.operand_table[operator_id]->add_def(instruction->id);
    }

    return instruction;
}

InstructionPtr Builder::fetch_branch_instruction(instruction::Branch::Op op, OperandID rs1_id, OperandID rs2_id,
                                                 BasicBlockID block_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::Branch{op, rs1_id, rs2_id, block_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rs1_id]->add_use(instruction->id);
    context.operand_table[rs2_id]->add_use(instruction->id);

    instruction->add_use(rs1_id);
    instruction->add_use(rs2_id);

    return instruction;
}

InstructionPtr Builder::fetch_jump_instruction(BasicBlockID block_id)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::Jump{block_id});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    return instruction;
}

InstructionPtr Builder::fetch_ret_instruction()
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::Ret{});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    return instruction;
}

InstructionPtr Builder::fetch_phi_instruction(OperandID rd_id,
                                              std::vector<std::tuple<OperandID, BasicBlockID>> incoming_list)
{
    auto id = context.create_instruction_id();
    auto kind = InstructionKind(instruction::Phi{rd_id, incoming_list});
    auto instruction = create_instruction(id, kind, now_basic_block->id);

    context.register_instruction(instruction);

    context.operand_table[rd_id]->add_def(instruction->id);

    instruction->add_def(rd_id);

    for (auto [operand_id, block_id] : incoming_list)
    {
        context.operand_table[operand_id]->add_use(instruction->id);
        instruction->add_use(operand_id);
    }

    return instruction;
}

void Builder::add_instruction_front(InstructionPtr instruction)
{
    instruction->parent_block_id = now_basic_block->id;
    now_basic_block->add_instruction_front(instruction);
}

void Builder::add_instruction_back(InstructionPtr instruction)
{
    instruction->parent_block_id = now_basic_block->id;
    now_basic_block->add_instruction_back(instruction);
    auto basic_block_id = instruction->get_basic_block_id_if_branch();
    if (basic_block_id.has_value())
    {
        now_basic_block->add_successor(basic_block_id.value());
        context.basic_block_table[basic_block_id.value()]->add_predecessor(now_basic_block->id);
    }
}

void Builder::set_now_basic_block(BasicBlockPtr basicblock)
{
    now_basic_block = basicblock;
}

void Builder::set_now_function(std::string name)
{
    now_function = context.get_function(name);
}

void Builder::add_function(std::string name)
{
    auto function = std::make_shared<sed::backend::Function>(name);
    context.register_function(function);
}

} // namespace backend
} // namespace sed