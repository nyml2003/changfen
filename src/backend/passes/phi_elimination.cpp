#include "backend/passes/phi_elimination.h"

#include "backend/basic_block.h"
#include "backend/function.h"
#include "backend/instruction.h"
#include "backend/operand.h"
#include "backend/register.h"
#include "backend/utils.h"
#include "index.h"

namespace sed
{
namespace backend
{

void phi_elimination(Builder &builder)
{
    for (auto [now_function_name, now_function] : builder.context.function_table)
    {
        builder.set_now_function(now_function_name);
        phi_elimination_function(now_function, builder);
    }
}

void phi_elimination_function(FunctionPtr now_function, Builder &builder)
{
    auto now_block = now_function->head_basic_block->next;
    while (now_block != now_function->tail_basic_block)
    {
        auto predecessor_id_set = std::set<BasicBlockID>();
        for (auto predecessor_id : now_block->predecessor_list)
        {
            predecessor_id_set.insert(predecessor_id);
        }

        auto now_instruction = now_block->head_instruction->next;
        auto phi_def_set = std::set<OperandID>();
        auto conflict_set = std::set<OperandID>();

        while (now_instruction->is_phi() && now_instruction != now_block->tail_instruction)
        {
            auto now_phi = now_instruction->as<instruction::Phi>().value();

            for (auto [operand_id, _] : now_phi.incoming_list)
            {
                if (phi_def_set.count(operand_id))
                {
                    conflict_set.insert(operand_id);
                }
            }
            phi_def_set.insert(now_phi.rd_id);

            now_instruction = now_instruction->next;
        }

        auto next_block = now_block->next;
        auto block_jump_map = std::map<BasicBlockID, BasicBlockID>();
        auto block_instruction_map = std::map<BasicBlockID, std::vector<InstructionPtr>>();
        auto operand_map = std::map<OperandID, OperandID>();

        now_instruction = now_block->head_instruction->next;

        while (now_instruction->is_phi() && now_instruction != now_block->tail_instruction)
        {
            auto now_phi = now_instruction->as<instruction::Phi>().value();
            auto now_phi_operand = builder.context.get_operand(now_phi.rd_id);

            auto temp_phi_operand_id = now_phi_operand->id;
            if (conflict_set.count(temp_phi_operand_id))
            {
                if (now_phi_operand->is_float())
                {
                    temp_phi_operand_id = builder.fetch_virtual_register(VirtualRegisterKind::FLOAT);
                }
                else
                {
                    temp_phi_operand_id = builder.fetch_virtual_register(VirtualRegisterKind::GENERAL);
                }

                operand_map[now_phi.rd_id] = temp_phi_operand_id;
            }

            for (const auto &[operand_id, predecessor_block_id] : now_phi.incoming_list)
            {
                auto operand = builder.context.get_operand(operand_id);
                auto predecessor_block = builder.context.get_basic_block(predecessor_block_id);

                if (!predecessor_id_set.count(predecessor_block_id))
                    continue;

                auto insert_block_id = predecessor_block_id;

                if (predecessor_block->successor_list.size() > 1)
                {
                    if (block_jump_map.count(predecessor_block_id))
                    {
                        insert_block_id = block_jump_map[predecessor_block_id];
                    }
                    else
                    {
                        // split critical edge.
                        auto new_block = builder.fetch_basic_block();
                        block_jump_map[predecessor_block_id] = new_block->id;
                        insert_block_id = new_block->id;

                        builder.set_now_basic_block(new_block);
                        now_block->remove_predecessor(predecessor_block_id);

                        auto jump_instruction = builder.fetch_jump_instruction(now_block->id);
                        builder.add_instruction_back(jump_instruction);
                        predecessor_block->remove_successor(now_block->id);

                        auto exit_instruction = predecessor_block->tail_instruction->prev.lock();

                        while (exit_instruction->is_branch_or_jmp() &&
                               exit_instruction != predecessor_block->head_instruction)
                        {
                            if (auto jump = std::get_if<instruction::Jump>(&exit_instruction->kind))
                            {
                                if (jump->block_id == now_block->id)
                                {
                                    jump->block_id = new_block->id;
                                    break;
                                }
                            }
                            else if (auto branch = std::get_if<instruction::Branch>(&exit_instruction->kind))
                            {
                                if (branch->block_id == now_block->id)
                                {
                                    branch->block_id = new_block->id;
                                    break;
                                }
                            }

                            exit_instruction = exit_instruction->prev.lock();
                        }

                        predecessor_block->add_successor(new_block->id);
                        new_block->add_predecessor(predecessor_block->id);

                        predecessor_block->insert_next(new_block);
                    }
                }

                if (!block_instruction_map.count(insert_block_id))
                {
                    block_instruction_map[insert_block_id] = std::vector<InstructionPtr>();
                }

                if (operand->is_immediate())
                {
                    if (now_phi_operand->is_float())
                    {
                        auto temp_register_id = builder.fetch_virtual_register(VirtualRegisterKind::GENERAL);
                        auto li_instruction = builder.fetch_li_instruction(temp_register_id, operand_id);
                        auto fmv_instruction = builder.fetch_float_move_instruction(
                            instruction::FloatMove::Mode::S, instruction::FloatMove::Mode::X, temp_phi_operand_id,
                            temp_register_id);
                        block_instruction_map[insert_block_id].push_back(li_instruction);
                        block_instruction_map[insert_block_id].push_back(fmv_instruction);
                    }
                    else
                    {
                        auto li_instruction = builder.fetch_li_instruction(temp_phi_operand_id, operand_id);
                        block_instruction_map[insert_block_id].push_back(li_instruction);
                    }
                }
                else if (operand->is_local_memory())
                {
                    auto offset = std::get<LocalMemory>(operand->kind).offset;
                    auto base_reg = std::get<LocalMemory>(operand->kind).reg;
                    auto asm_base_reg_id = builder.fetch_register(base_reg);

                    if (base_reg == Register{GeneralRegister::S0})
                    {
                        if (check_itype_immediate(offset))
                        {
                            if (now_phi_operand->is_float())
                            {
                                auto fld_instruction = builder.fetch_float_load_instruction(
                                    instruction::FloatLoad::Op::FLD, temp_phi_operand_id, asm_base_reg_id,
                                    builder.fetch_immediate(offset));
                                block_instruction_map[insert_block_id].push_back(fld_instruction);
                            }
                            else
                            {
                                auto ld_instruction =
                                    builder.fetch_load_instruction(instruction::Load::Op::LD, temp_phi_operand_id,
                                                                   asm_base_reg_id, builder.fetch_immediate(offset));
                                block_instruction_map[insert_block_id].push_back(ld_instruction);
                            }
                        }
                        else
                        {
                            auto temp_register_id = builder.fetch_virtual_register(VirtualRegisterKind::GENERAL);
                            auto li_instruction =
                                builder.fetch_li_instruction(temp_register_id, builder.fetch_immediate(offset));
                            auto add_instruction = builder.fetch_binary_instruction(
                                instruction::Binary::Op::ADD, temp_register_id, asm_base_reg_id, temp_register_id);
                            block_instruction_map[insert_block_id].push_back(li_instruction);
                            block_instruction_map[insert_block_id].push_back(add_instruction);

                            if (now_phi_operand->is_float())
                            {
                                auto fld_instruction = builder.fetch_float_load_instruction(
                                    instruction::FloatLoad::Op::FLD, temp_phi_operand_id, temp_register_id,
                                    builder.fetch_immediate(0));
                                block_instruction_map[insert_block_id].push_back(fld_instruction);
                            }
                            else
                            {
                                auto ld_instruction =
                                    builder.fetch_load_instruction(instruction::Load::Op::LD, temp_phi_operand_id,
                                                                   temp_register_id, builder.fetch_immediate(0));
                                block_instruction_map[insert_block_id].push_back(ld_instruction);
                            }
                        }
                    }
                    else
                    {
                        if (check_itype_immediate(offset))
                        {
                            auto addi_instruction = builder.fetch_binary_immediate_instruction(
                                instruction::BinaryImmediate::Op::ADDI, temp_phi_operand_id, asm_base_reg_id,
                                builder.fetch_immediate(offset));
                            block_instruction_map[insert_block_id].push_back(addi_instruction);
                        }
                        else
                        {
                            auto temp_register_id = builder.fetch_virtual_register(VirtualRegisterKind::GENERAL);
                            auto li_instruction =
                                builder.fetch_li_instruction(temp_register_id, builder.fetch_immediate(offset));
                            auto add_instruction = builder.fetch_binary_instruction(
                                instruction::Binary::Op::ADD, temp_phi_operand_id, asm_base_reg_id, temp_register_id);
                            block_instruction_map[insert_block_id].push_back(li_instruction);
                            block_instruction_map[insert_block_id].push_back(add_instruction);
                        }
                    }
                }
                else if (operand->is_global())
                {
                    auto la_instruction = builder.fetch_pseudo_load_instruction(instruction::PseudoLoad::Op::LA,
                                                                                temp_phi_operand_id, operand_id);
                    block_instruction_map[insert_block_id].push_back(la_instruction);
                }
                else
                {
                    if (now_phi_operand->is_float())
                    {
                        auto fsgnjs_instruction = builder.fetch_float_binary_instruction(
                            instruction::FloatBinary::Op::FSGNJ, instruction::FloatBinary::Mode::SINGLE,
                            temp_phi_operand_id, operand_id, operand_id);
                        block_instruction_map[insert_block_id].push_back(fsgnjs_instruction);
                    }
                    else
                    {
                        auto addi_instruction = builder.fetch_binary_immediate_instruction(
                            instruction::BinaryImmediate::Op::ADDI, temp_phi_operand_id, operand_id,
                            builder.fetch_immediate(0));
                        block_instruction_map[insert_block_id].push_back(addi_instruction);
                    }
                }
            }

            now_instruction->remove(builder.context);

            now_instruction = now_instruction->next;
        }

        for (auto &[block_id, instruction_list] : block_instruction_map)
        {
            auto block = builder.context.get_basic_block(block_id);
            builder.set_now_basic_block(block);

            auto exit_instruction = block->tail_instruction->prev.lock();
            while (exit_instruction != block->head_instruction && exit_instruction->is_branch_or_jmp())
            {
                exit_instruction = exit_instruction->prev.lock();
            }

            exit_instruction = exit_instruction->next;
            for (auto &instruction : instruction_list)
            {
                exit_instruction->insert_prev(instruction);
                instruction->parent_block_id = block_id;
            }
        }

        for (auto [rd_id, temp_rd_id] : operand_map)
        {
            auto rd_operand = builder.context.get_operand(rd_id);

            builder.set_now_basic_block(now_block);
            if (rd_operand->is_float())
            {
                auto fsgnjs_instruction = builder.fetch_float_binary_instruction(instruction::FloatBinary::Op::FSGNJ,
                                                                                 instruction::FloatBinary::Mode::SINGLE,
                                                                                 rd_id, temp_rd_id, temp_rd_id);
                builder.add_instruction_front(fsgnjs_instruction);
            }
            else
            {
                auto addi_instruction = builder.fetch_binary_immediate_instruction(
                    instruction::BinaryImmediate::Op::ADDI, rd_id, temp_rd_id, builder.fetch_immediate(0));
                builder.add_instruction_front(addi_instruction);
            }
        }

        now_block = next_block;
    }
}

} // namespace backend
} // namespace sed
