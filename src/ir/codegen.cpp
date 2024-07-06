#include "ir/codegen.h"

#include "backend/passes/regalloc.h"

namespace sed
{

inline AsmOperandID CodegenContext::get_asm_operand_id(IrOperandID ir_operand_id)
{
    return operand_map.at(ir_operand_id);
}

void codegen(IrContext &ir_context, AsmBuilder &builder, CodegenContext &codegen_context)
{
    for (auto ir_global_id : ir_context.global_list)
    {
        auto &ir_global = std::get<ir::operand::Global>(ir_context.get_operand(ir_global_id)->kind);
        auto &ir_global_init = std::get<ir::operand::ConstantPtr>(ir_context.get_operand(ir_global.init)->kind);

        std::visit(
            overloaded{
                [&](const int value) {
                    std::vector<uint32_t> asm_value = {*(uint32_t *)&value};
                    auto asm_operand_id = builder.fetch_global(ir_global.name, asm_value);
                    codegen_context.operand_map[ir_global_id] = asm_operand_id;
                },
                [&](const float value) {
                    std::vector<uint32_t> asm_value = {*(uint32_t *)&value};
                    auto asm_operand_id = builder.fetch_global(ir_global.name, asm_value);
                    codegen_context.operand_map[ir_global_id] = asm_operand_id;
                },
                [&](const ir::operand::ZeroInitializer) {
                    auto asm_operand_id = builder.fetch_global(ir_global.name, ir_global_init->type->size() / 8);
                    codegen_context.operand_map[ir_global_id] = asm_operand_id;
                },
                [&](const std::vector<ir::operand::ConstantPtr> &value) {
                    std::vector<uint32_t> asm_value;
                    std::function<void(const ir::operand::ConstantPtr &)> func =
                        [&func, &asm_value](const ir::operand::ConstantPtr &item) {
                            std::visit(
                                overloaded{
                                    [&asm_value](const int value) { asm_value.push_back(*(uint32_t *)&value); },
                                    [&asm_value](const float value) { asm_value.push_back(*(uint32_t *)&value); },
                                    [&asm_value, &item](const ir::operand::ZeroInitializer) {
                                        size_t zero_cnt = item->type->size() / 32;
                                        for (size_t i = 0; i < zero_cnt; ++i)
                                        {
                                            asm_value.push_back(0);
                                        }
                                    },
                                    [&func, &asm_value](const std::vector<ir::operand::ConstantPtr> &value) {
                                        for (auto &item : value)
                                        {
                                            func(item);
                                        }
                                    }},
                                item->kind);
                        };
                    for (auto &item : value)
                    {
                        func(item);
                    }
                    auto asm_operand_id = builder.fetch_global(ir_global.name, asm_value);
                    codegen_context.operand_map[ir_global_id] = asm_operand_id;
                }},
            ir_global_init->kind);
    }

    for (auto &[ir_function_name, ir_function] : ir_context.function_table)
    {
        if (ir_function->is_declare)
        {
            continue;
        }
        codegen_function(ir_function, ir_context, builder, codegen_context);
    }
}

void codegen_function(IrFunctionPtr ir_function, IrContext &ir_context, AsmBuilder &builder,
                      CodegenContext &codegen_context)
{
    builder.add_function(ir_function->name);
    builder.set_now_function(ir_function->name);

    auto curr_ir_block = ir_function->head_block->next;
    while (curr_ir_block != ir_function->tail_block)
    {
        auto asm_block = builder.fetch_basic_block();
        builder.now_function->add_basic_block(asm_block);
        codegen_context.basic_block_map[curr_ir_block->id] = asm_block->id;
        curr_ir_block = curr_ir_block->next;
    }

    int used_general_reg = 0, used_float_reg = 0;

    std::vector<IrOperandID> ir_stack_param_id_list;

    builder.set_now_basic_block(builder.now_function->head_basic_block->next);

    for (auto ir_param_id : ir_function->param_id_list)
    {
        if (ir_context.get_operand(ir_param_id)->type->is<ir::type::Float>())
        {
            if (used_float_reg <= 7)
            {
                auto asm_reg = builder.fetch_register(
                    backend::Register{(backend::FloatRegister)((int)backend::FloatRegister::FA0 + used_float_reg)});
                auto vreg = builder.fetch_virtual_register(backend::VirtualRegisterKind::FLOAT);

                builder.add_instruction_back(builder.fetch_float_binary_instruction(
                    backend::instruction::FloatBinary::FSGNJ, backend::instruction::FloatBinary::SINGLE, vreg, asm_reg,
                    asm_reg));

                codegen_context.operand_map[ir_param_id] = vreg;
                used_float_reg++;
            }
            else
            {
                ir_stack_param_id_list.push_back(ir_param_id);
            }
        }
        else
        {
            if (used_general_reg <= 7)
            {
                auto asm_reg = builder.fetch_register(backend::Register{
                    (backend::GeneralRegister)((int)backend::GeneralRegister::A0 + used_general_reg)});
                auto vreg = builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);

                builder.add_instruction_back(builder.fetch_binary_immediate_instruction(
                    backend::instruction::BinaryImmediate::ADDI, vreg, asm_reg, builder.fetch_immediate(0)));

                codegen_context.operand_map[ir_param_id] = vreg;
                used_general_reg++;
            }
            else
            {
                ir_stack_param_id_list.push_back(ir_param_id);
            }
        }
    }

    int offset = 0;
    for (auto ir_param_id : ir_stack_param_id_list)
    {
        codegen_context.operand_map[ir_param_id] =
            builder.fetch_local_memory(offset, backend::Register{backend::GeneralRegister::S0});
        offset += 8;
    }

    curr_ir_block = ir_function->head_block->next;
    while (curr_ir_block != ir_function->tail_block)
    {
        codegen_basic_block(curr_ir_block, ir_context, builder, codegen_context);
        curr_ir_block = curr_ir_block->next;
    }

    for (auto [ir_inst, asm_inst] : codegen_context.phi_list)
    {
        builder.set_now_basic_block(builder.context.get_basic_block(asm_inst->parent_block_id));
        codegen_phi_incoming_list(ir_inst, asm_inst, ir_context, builder, codegen_context);
    }
    codegen_context.phi_list.clear();
}

void codegen_rest(AsmBuilder &builder)
{
    asm_register_allocation(builder);

    for (auto &[ir_function_name, ir_function] : builder.context.function_table)
    {
        builder.set_now_function(ir_function_name);
        codegen_function_prolouge(builder);
        codegen_function_epilouge(builder);
    }
}

void codegen_function_prolouge(AsmBuilder &builder)
{
    using namespace backend;

    builder.now_function->add_saved_register(Register{GeneralRegister::S0});
    auto entry_block = builder.now_function->head_basic_block->next;

    builder.now_function->stack_frame_size += 8 * (1 + builder.now_function->saved_register_set.size());

    int32_t aligned_stack_frame_size = (builder.now_function->stack_frame_size + 15) / 16 * 16;
    builder.now_function->align_frame_size = aligned_stack_frame_size - builder.now_function->stack_frame_size;

    auto ra_id = builder.fetch_register(Register{GeneralRegister::RA});
    auto sp_id = builder.fetch_register(Register{GeneralRegister::SP});

    if (backend::check_itype_immediate(aligned_stack_frame_size))
    {
        entry_block->add_instruction_front(builder.fetch_binary_immediate_instruction(
            instruction::BinaryImmediate::Op::ADDI, builder.fetch_register(Register{GeneralRegister::S0}), sp_id,
            builder.fetch_immediate(aligned_stack_frame_size)));
    }
    else
    {
        auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});

        entry_block->add_instruction_front(builder.fetch_binary_instruction(
            instruction::Binary::Op::ADD, builder.fetch_register(Register{GeneralRegister::S0}), sp_id, asm_tmp_id));
        entry_block->add_instruction_front(
            builder.fetch_li_instruction(asm_tmp_id, builder.fetch_immediate(aligned_stack_frame_size)));
    }

    int32_t curr_frame_pos = aligned_stack_frame_size - 16;
    for (auto reg : builder.now_function->saved_register_set)
    {
        auto reg_id = builder.fetch_register(reg);

        if (reg.is_general())
        {
            if (backend::check_itype_immediate(curr_frame_pos))
            {
                entry_block->add_instruction_front(builder.fetch_store_instruction(
                    instruction::Store::Op::SD, sp_id, reg_id, builder.fetch_immediate(curr_frame_pos)));
            }
            else
            {
                auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});

                entry_block->add_instruction_front(builder.fetch_store_instruction(
                    instruction::Store::Op::SD, asm_tmp_id, reg_id, builder.fetch_immediate(0)));
                entry_block->add_instruction_front(
                    builder.fetch_binary_instruction(instruction::Binary::Op::ADD, asm_tmp_id, sp_id, asm_tmp_id));
                entry_block->add_instruction_front(
                    builder.fetch_li_instruction(asm_tmp_id, builder.fetch_immediate(curr_frame_pos)));
            }
        }
        else
        {
            if (backend::check_itype_immediate(curr_frame_pos))
            {
                entry_block->add_instruction_front(builder.fetch_float_store_instruction(
                    instruction::FloatStore::Op::FSD, sp_id, reg_id, builder.fetch_immediate(curr_frame_pos)));
            }
            else
            {
                auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});

                entry_block->add_instruction_front(builder.fetch_float_store_instruction(
                    instruction::FloatStore::Op::FSD, asm_tmp_id, reg_id, builder.fetch_immediate(0)));
                entry_block->add_instruction_front(
                    builder.fetch_binary_instruction(instruction::Binary::Op::ADD, asm_tmp_id, sp_id, asm_tmp_id));
                entry_block->add_instruction_front(
                    builder.fetch_li_instruction(asm_tmp_id, builder.fetch_immediate(curr_frame_pos)));
            }
        }
        curr_frame_pos -= 8;
    }

    curr_frame_pos = aligned_stack_frame_size - 8;
    if (backend::check_itype_immediate(curr_frame_pos))
    {
        entry_block->add_instruction_front(builder.fetch_store_instruction(instruction::Store::Op::SD, sp_id, ra_id,
                                                                           builder.fetch_immediate(curr_frame_pos)));
    }
    else
    {
        auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});

        entry_block->add_instruction_front(
            builder.fetch_store_instruction(instruction::Store::Op::SD, asm_tmp_id, ra_id, builder.fetch_immediate(0)));
        entry_block->add_instruction_front(
            builder.fetch_binary_instruction(instruction::Binary::Op::ADD, asm_tmp_id, sp_id, asm_tmp_id));
        entry_block->add_instruction_front(
            builder.fetch_li_instruction(asm_tmp_id, builder.fetch_immediate(curr_frame_pos)));
    }

    curr_frame_pos = -aligned_stack_frame_size;
    if (backend::check_itype_immediate(curr_frame_pos))
    {
        entry_block->add_instruction_front(builder.fetch_binary_immediate_instruction(
            instruction::BinaryImmediate::Op::ADDI, sp_id, sp_id, builder.fetch_immediate(curr_frame_pos)));
    }
    else
    {
        auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});

        entry_block->add_instruction_front(
            builder.fetch_binary_instruction(instruction::Binary::Op::ADD, sp_id, sp_id, asm_tmp_id));
        entry_block->add_instruction_front(
            builder.fetch_li_instruction(asm_tmp_id, builder.fetch_immediate(curr_frame_pos)));
    }
}

void codegen_function_epilouge(AsmBuilder &builder)
{
    using namespace backend;

    auto exit_block = builder.now_function->tail_basic_block->prev.lock();
    auto last_inst = exit_block->tail_instruction->prev.lock();

    auto ra_id = builder.fetch_register(Register{GeneralRegister::RA});
    auto sp_id = builder.fetch_register(Register{GeneralRegister::SP});

    int32_t aligned_stack_frame_size = builder.now_function->stack_frame_size + builder.now_function->align_frame_size;

    int32_t curr_frame_pos = aligned_stack_frame_size - 16;
    for (auto &reg : builder.now_function->saved_register_set)
    {
        auto reg_id = builder.fetch_register(reg);

        if (reg.is_general())
        {
            if (backend::check_itype_immediate(curr_frame_pos))
            {
                last_inst->insert_prev(builder.fetch_load_instruction(instruction::Load::Op::LD, reg_id, sp_id,
                                                                      builder.fetch_immediate(curr_frame_pos)));
            }
            else
            {
                auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});

                last_inst->insert_prev(
                    builder.fetch_li_instruction(asm_tmp_id, builder.fetch_immediate(curr_frame_pos)));
                last_inst->insert_prev(
                    builder.fetch_binary_instruction(instruction::Binary::Op::ADD, asm_tmp_id, sp_id, asm_tmp_id));
                last_inst->insert_prev(builder.fetch_load_instruction(instruction::Load::Op::LD, reg_id, asm_tmp_id,
                                                                      builder.fetch_immediate(0)));
            }
        }
        else
        {
            if (backend::check_itype_immediate(curr_frame_pos))
            {
                last_inst->insert_prev(builder.fetch_float_load_instruction(
                    instruction::FloatLoad::Op::FLD, reg_id, sp_id, builder.fetch_immediate(curr_frame_pos)));
            }
            else
            {
                auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});

                last_inst->insert_prev(
                    builder.fetch_li_instruction(asm_tmp_id, builder.fetch_immediate(curr_frame_pos)));
                last_inst->insert_prev(
                    builder.fetch_binary_instruction(instruction::Binary::Op::ADD, asm_tmp_id, sp_id, asm_tmp_id));
                last_inst->insert_prev(builder.fetch_float_load_instruction(instruction::FloatLoad::Op::FLD, reg_id,
                                                                            asm_tmp_id, builder.fetch_immediate(0)));
            }
        }
        curr_frame_pos -= 8;
    }

    curr_frame_pos = aligned_stack_frame_size - 8;
    if (backend::check_itype_immediate(curr_frame_pos))
    {
        last_inst->insert_prev(builder.fetch_load_instruction(instruction::Load::Op::LD, ra_id, sp_id,
                                                              builder.fetch_immediate(curr_frame_pos)));
    }
    else
    {
        auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});

        last_inst->insert_prev(builder.fetch_li_instruction(asm_tmp_id, builder.fetch_immediate(curr_frame_pos)));
        last_inst->insert_prev(
            builder.fetch_binary_instruction(instruction::Binary::Op::ADD, asm_tmp_id, sp_id, asm_tmp_id));
        last_inst->insert_prev(
            builder.fetch_load_instruction(instruction::Load::Op::LD, ra_id, asm_tmp_id, builder.fetch_immediate(0)));
    }

    curr_frame_pos = aligned_stack_frame_size;
    if (backend::check_itype_immediate(curr_frame_pos))
    {
        last_inst->insert_prev(builder.fetch_binary_immediate_instruction(
            instruction::BinaryImmediate::Op::ADDI, sp_id, sp_id, builder.fetch_immediate(curr_frame_pos)));
    }
    else
    {
        auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});

        last_inst->insert_prev(builder.fetch_li_instruction(asm_tmp_id, builder.fetch_immediate(curr_frame_pos)));
        last_inst->insert_prev(
            builder.fetch_binary_instruction(instruction::Binary::Op::ADD, sp_id, sp_id, asm_tmp_id));
    }
}

void codegen_basic_block(IrBasicBlockPtr ir_basic_block, IrContext &ir_context, AsmBuilder &builder,
                         CodegenContext &codegen_context)
{
    builder.set_now_basic_block(builder.context.get_basic_block(codegen_context.basic_block_map[ir_basic_block->id]));

    auto curr_ir_inst = ir_basic_block->head_inst->next;
    while (curr_ir_inst != ir_basic_block->tail_inst)
    {
        codegen_instruction(curr_ir_inst, ir_context, builder, codegen_context);
        curr_ir_inst = curr_ir_inst->next;
    }
}

void codegen_instruction(IrInstructionPtr ir_inst, IrContext &ir_context, AsmBuilder &builder,
                         CodegenContext &codegen_context)
{
    std::visit(
        overloaded{
            [&](const ir::instruction::Alloca &inst) {
                codegen_context.operand_map[inst.dst_id] = builder.fetch_local_memory(
                    builder.now_function->stack_frame_size, backend::Register{backend::GeneralRegister::SP});

                builder.now_function->stack_frame_size += std::max(inst.allocated_type->size(), (size_t)32) / 8;
            },
            [&](ir::instruction::Store &inst) {
                auto asm_value_id = codegen_operand(inst.value_id, ir_context, builder, codegen_context);
                auto asm_ptr_id = codegen_context.get_asm_operand_id(inst.ptr_id);

                bool is_float = builder.context.get_operand(asm_value_id)->is_float();
                bool is_global = builder.context.get_operand(asm_ptr_id)->is_global();

                if (is_float)
                {
                    if (is_global)
                    {
                        builder.add_instruction_back(builder.fetch_float_pseudo_store_instruction(
                            backend::instruction::FloatPseudoStore::FSW, asm_value_id, asm_ptr_id,
                            builder.fetch_register(backend::Register{backend::GeneralRegister::T2})));
                    }
                    else
                    {
                        auto [offset, base_reg] = codegen_ptr2reg(asm_ptr_id, builder);
                        builder.add_instruction_back(
                            builder.fetch_float_store_instruction(backend::instruction::FloatStore::Op::FSW, base_reg,
                                                                  asm_value_id, builder.fetch_immediate(offset)));
                    }
                }
                else
                {
                    if (is_global)
                    {
                        builder.add_instruction_back(builder.fetch_pseudo_store_instruction(
                            backend::instruction::PseudoStore::SW, asm_value_id, asm_ptr_id,
                            builder.fetch_register(backend::Register{backend::GeneralRegister::T2})));
                    }
                    else
                    {
                        auto [offset, base_reg] = codegen_ptr2reg(asm_ptr_id, builder);
                        builder.add_instruction_back(
                            builder.fetch_store_instruction((ir_context.get_operand(inst.value_id)->type->size() == 64)
                                                                ? backend::instruction::Store::Op::SD
                                                                : backend::instruction::Store::Op::SW,
                                                            base_reg, asm_value_id, builder.fetch_immediate(offset)));
                    }
                }
            },
            [&](ir::instruction::Load &inst) {
                auto asm_dst_id = codegen_vreg(inst.dst_id, ir_context, builder, codegen_context);
                auto asm_ptr_id = codegen_context.get_asm_operand_id(inst.ptr_id);

                bool is_float = builder.context.get_operand(asm_dst_id)->is_float();
                bool is_global = builder.context.get_operand(asm_ptr_id)->is_global();

                if (is_float)
                {
                    if (is_global)
                    {
                        builder.add_instruction_back(builder.fetch_float_pseudo_load_instruction(
                            backend::instruction::FloatPseudoLoad::FLW, asm_dst_id, asm_ptr_id,
                            builder.fetch_register(backend::Register{backend::GeneralRegister::T2})));
                    }
                    else
                    {
                        auto [offset, base_reg] = codegen_ptr2reg(asm_ptr_id, builder);
                        builder.add_instruction_back(
                            builder.fetch_float_load_instruction(backend::instruction::FloatLoad::Op::FLW, asm_dst_id,
                                                                 base_reg, builder.fetch_immediate(offset)));
                    }
                }
                else
                {
                    if (is_global)
                    {
                        builder.add_instruction_back(builder.fetch_pseudo_load_instruction(
                            ir_context.get_operand(inst.dst_id)->type->is<ir::type::Pointer>()
                                ? backend::instruction::PseudoLoad::LA
                                : backend::instruction::PseudoLoad::LW,
                            asm_dst_id, asm_ptr_id));
                    }
                    else
                    {
                        auto [offset, base_reg] = codegen_ptr2reg(asm_ptr_id, builder);
                        builder.add_instruction_back(
                            builder.fetch_load_instruction((ir_context.get_operand(inst.dst_id)->type->size() == 64)
                                                               ? backend::instruction::Load::Op::LD
                                                               : backend::instruction::Load::Op::LW,
                                                           asm_dst_id, base_reg, builder.fetch_immediate(offset)));
                    }
                }
            },
            [&](ir::instruction::Binary &inst) {
                auto ir_rhs_id = inst.rsrc_id;
                auto asm_dst_id = codegen_vreg(inst.dst_id, ir_context, builder, codegen_context);
                auto asm_lhs_id = codegen_operand(inst.lsrc_id, ir_context, builder, codegen_context, false, true);
                AsmOperandID asm_rhs_id;

                switch (inst.op)
                {
                case ir::instruction::BinaryOp::IAdd:
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context, true);
                    builder.add_instruction_back(
                        builder.context.get_operand(asm_rhs_id)->is_immediate()
                            ? builder.fetch_binary_immediate_instruction(
                                  backend::instruction::BinaryImmediate::Op::ADDIW, asm_dst_id, asm_lhs_id, asm_rhs_id)
                            : builder.fetch_binary_instruction(backend::instruction::Binary::Op::ADDW, asm_dst_id,
                                                               asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::BinaryOp::ISub:
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context);
                    builder.add_instruction_back(builder.fetch_binary_instruction(
                        backend::instruction::Binary::Op::SUBW, asm_dst_id, asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::BinaryOp::IMul:
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context);
                    builder.add_instruction_back(builder.fetch_binary_instruction(
                        backend::instruction::Binary::Op::MULW, asm_dst_id, asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::BinaryOp::IDiv:
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context);
                    builder.add_instruction_back(builder.fetch_binary_instruction(
                        backend::instruction::Binary::Op::DIVW, asm_dst_id, asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::BinaryOp::Mod:
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context);
                    builder.add_instruction_back(builder.fetch_binary_instruction(
                        backend::instruction::Binary::Op::REMW, asm_dst_id, asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::BinaryOp::FAdd:
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context, false, true);
                    builder.add_instruction_back(builder.fetch_float_binary_instruction(
                        backend::instruction::FloatBinary::Op::FADD, backend::instruction::FloatBinary::Mode::SINGLE,
                        asm_dst_id, asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::BinaryOp::FSub:
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context, false, true);
                    builder.add_instruction_back(builder.fetch_float_binary_instruction(
                        backend::instruction::FloatBinary::Op::FSUB, backend::instruction::FloatBinary::Mode::SINGLE,
                        asm_dst_id, asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::BinaryOp::FMul:
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context, false, true);
                    builder.add_instruction_back(builder.fetch_float_binary_instruction(
                        backend::instruction::FloatBinary::Op::FMUL, backend::instruction::FloatBinary::Mode::SINGLE,
                        asm_dst_id, asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::BinaryOp::FDiv:
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context, false, true);
                    builder.add_instruction_back(builder.fetch_float_binary_instruction(
                        backend::instruction::FloatBinary::Op::FDIV, backend::instruction::FloatBinary::Mode::SINGLE,
                        asm_dst_id, asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::BinaryOp::Sll:
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context, true);
                    builder.add_instruction_back(
                        builder.context.get_operand(asm_rhs_id)->is_immediate()
                            ? builder.fetch_binary_immediate_instruction(
                                  backend::instruction::BinaryImmediate::Op::SLLIW, asm_dst_id, asm_lhs_id, asm_rhs_id)
                            : builder.fetch_binary_instruction(backend::instruction::Binary::Op::SLLW, asm_dst_id,
                                                               asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::BinaryOp::Srl:
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context, true);
                    builder.add_instruction_back(
                        builder.context.get_operand(asm_rhs_id)->is_immediate()
                            ? builder.fetch_binary_immediate_instruction(
                                  backend::instruction::BinaryImmediate::Op::SRLIW, asm_dst_id, asm_lhs_id, asm_rhs_id)
                            : builder.fetch_binary_instruction(backend::instruction::Binary::Op::SRLW, asm_dst_id,
                                                               asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::BinaryOp::Sra:
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context, true);
                    builder.add_instruction_back(
                        builder.context.get_operand(asm_rhs_id)->is_immediate()
                            ? builder.fetch_binary_immediate_instruction(
                                  backend::instruction::BinaryImmediate::Op::SRAIW, asm_dst_id, asm_lhs_id, asm_rhs_id)
                            : builder.fetch_binary_instruction(backend::instruction::Binary::Op::SRAW, asm_dst_id,
                                                               asm_lhs_id, asm_rhs_id));
                    break;
                default:
                    throw std::runtime_error("unexpact binary op.");
                    break;
                }
            },
            [&](ir::instruction::ICmp &inst) {
                auto asm_dst_id = codegen_vreg(inst.dst_id, ir_context, builder, codegen_context);
                AsmOperandID asm_lhs_id, asm_rhs_id, tmp_reg_id;
                switch (inst.op)
                {
                case ir::instruction::ICmpOp::Eq:
                case ir::instruction::ICmpOp::Ne:
                    tmp_reg_id = builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);
                    asm_lhs_id = codegen_operand(inst.lsrc_id, ir_context, builder, codegen_context);
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context, true);

                    builder.add_instruction_back(
                        builder.context.get_operand(asm_rhs_id)->is_immediate()
                            ? builder.fetch_binary_immediate_instruction(backend::instruction::BinaryImmediate::XORI,
                                                                         tmp_reg_id, asm_lhs_id, asm_rhs_id)
                            : builder.fetch_binary_instruction(backend::instruction::Binary::XOR, tmp_reg_id,
                                                               asm_lhs_id, asm_rhs_id));
                    builder.add_instruction_back(
                        inst.op == ir::instruction::ICmpOp::Eq
                            ? builder.fetch_binary_immediate_instruction(backend::instruction::BinaryImmediate::SLTIU,
                                                                         asm_dst_id, tmp_reg_id,
                                                                         builder.fetch_immediate(1))
                            : builder.fetch_binary_instruction(
                                  backend::instruction::Binary::Op::SLTU, asm_dst_id,
                                  builder.fetch_register(backend::Register(backend::GeneralRegister::ZERO)),
                                  tmp_reg_id));
                    break;
                case ir::instruction::ICmpOp::Lt:
                    asm_lhs_id = codegen_operand(inst.lsrc_id, ir_context, builder, codegen_context);
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context, true);

                    builder.add_instruction_back(
                        builder.context.get_operand(asm_rhs_id)->is_immediate()
                            ? builder.fetch_binary_immediate_instruction(backend::instruction::BinaryImmediate::SLTI,
                                                                         asm_dst_id, asm_lhs_id, asm_rhs_id)
                            : builder.fetch_binary_instruction(backend::instruction::Binary::SLT, asm_dst_id,
                                                               asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::ICmpOp::Le:
                    // a<=b ==> !(a>b) ==> !(b<a)
                    tmp_reg_id = builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);
                    asm_lhs_id = codegen_operand(inst.lsrc_id, ir_context, builder, codegen_context, true);
                    asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context);

                    builder.add_instruction_back(
                        builder.context.get_operand(asm_lhs_id)->is_immediate()
                            ? builder.fetch_binary_immediate_instruction(backend::instruction::BinaryImmediate::SLTI,
                                                                         tmp_reg_id, asm_rhs_id, asm_lhs_id)
                            : builder.fetch_binary_instruction(backend::instruction::Binary::SLT, tmp_reg_id,
                                                               asm_rhs_id, asm_lhs_id));
                    builder.add_instruction_back(
                        builder.fetch_binary_immediate_instruction(backend::instruction::BinaryImmediate::SLTIU,
                                                                   asm_dst_id, tmp_reg_id, builder.fetch_immediate(1)));
                    break;
                default:
                    throw std::runtime_error("unexpact icmp op.");
                    break;
                }
            },
            [&](ir::instruction::FCmp &inst) {
                auto asm_dst_id = codegen_vreg(inst.dst_id, ir_context, builder, codegen_context);
                AsmOperandID tmp_reg_id;
                auto asm_lhs_id = codegen_operand(inst.lsrc_id, ir_context, builder, codegen_context, false, true);
                auto asm_rhs_id = codegen_operand(inst.rsrc_id, ir_context, builder, codegen_context, false, true);

                switch (inst.op)
                {
                case ir::instruction::FCmpOp::Eq:
                    builder.add_instruction_back(builder.fetch_float_binary_instruction(
                        backend::instruction::FloatBinary::FEQ, backend::instruction::FloatBinary::Mode::SINGLE,
                        asm_dst_id, asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::FCmpOp::Ne:
                    tmp_reg_id = builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);

                    builder.add_instruction_back(builder.fetch_float_binary_instruction(
                        backend::instruction::FloatBinary::FEQ, backend::instruction::FloatBinary::Mode::SINGLE,
                        tmp_reg_id, asm_lhs_id, asm_rhs_id));
                    builder.add_instruction_back(
                        builder.fetch_binary_immediate_instruction(backend::instruction::BinaryImmediate::SLTIU,
                                                                   asm_dst_id, tmp_reg_id, builder.fetch_immediate(1)));
                    break;
                case ir::instruction::FCmpOp::Lt:
                    builder.add_instruction_back(builder.fetch_float_binary_instruction(
                        backend::instruction::FloatBinary::FLT, backend::instruction::FloatBinary::Mode::SINGLE,
                        asm_dst_id, asm_lhs_id, asm_rhs_id));
                    break;
                case ir::instruction::FCmpOp::Le:
                    builder.add_instruction_back(builder.fetch_float_binary_instruction(
                        backend::instruction::FloatBinary::FLE, backend::instruction::FloatBinary::Mode::SINGLE,
                        asm_dst_id, asm_lhs_id, asm_rhs_id));
                    break;
                default:
                    throw std::runtime_error("unexpact icmp op.");
                    break;
                }
            },
            [&](ir::instruction::Cast &inst) {
                auto asm_dst_id = codegen_vreg(inst.dst_id, ir_context, builder, codegen_context);
                auto asm_src_id = codegen_operand(inst.src_id, ir_context, builder, codegen_context, false, true);

                switch (inst.op)
                {
                case ir::instruction::CastOp::Ext:
                    builder.add_instruction_back(
                        builder.fetch_binary_immediate_instruction(backend::instruction::BinaryImmediate::Op::ADDI,
                                                                   asm_dst_id, asm_src_id, builder.fetch_immediate(0)));
                    break;
                case ir::instruction::CastOp::Bitcast:
                    builder.add_instruction_back(builder.context.get_operand(asm_src_id)->is_global()
                                                     ? builder.fetch_pseudo_load_instruction(
                                                           backend::instruction::PseudoLoad::LA, asm_dst_id, asm_src_id)
                                                     : builder.fetch_binary_immediate_instruction(
                                                           backend::instruction::BinaryImmediate::Op::ADDI, asm_dst_id,
                                                           asm_src_id, builder.fetch_immediate(0)));
                    break;
                case ir::instruction::CastOp::F2I:
                    builder.add_instruction_back(builder.fetch_float_convert_instruction(
                        backend::instruction::FloatConvert::Mode::W, backend::instruction::FloatConvert::Mode::S,
                        asm_dst_id, asm_src_id));
                    break;
                case ir::instruction::CastOp::I2F:
                    builder.add_instruction_back(builder.fetch_float_convert_instruction(
                        backend::instruction::FloatConvert::Mode::S, backend::instruction::FloatConvert::Mode::W,
                        asm_dst_id, asm_src_id));
                    break;
                default:
                    throw std::runtime_error("unexpact cast op.");
                    break;
                }
            },
            [&](ir::instruction::Branch &inst) {
                auto asm_cond_id = codegen_operand(inst.cond_id, ir_context, builder, codegen_context);

                builder.add_instruction_back(builder.fetch_branch_instruction(
                    backend::instruction::Branch::BNE, asm_cond_id,
                    builder.fetch_register(backend::Register(backend::GeneralRegister::ZERO)),
                    codegen_context.basic_block_map.at(inst.then_block_id)));
                builder.add_instruction_back(
                    builder.fetch_jump_instruction(codegen_context.basic_block_map.at(inst.else_block_id)));
            },
            [&](ir::instruction::Jump &inst) {
                builder.add_instruction_back(
                    builder.fetch_jump_instruction(codegen_context.basic_block_map.at(inst.then_block_id)));
            },
            [&](ir::instruction::Phi &inst) {
                auto asm_dst_id = codegen_vreg(inst.dst_id, ir_context, builder, codegen_context);

                auto asm_inst = builder.fetch_phi_instruction(asm_dst_id, {});

                codegen_context.phi_list.emplace_back(ir_inst, asm_inst);

                builder.add_instruction_back(asm_inst);
            },
            [&](ir::instruction::Call &inst) {
                int curr_general_reg = 0, curr_float_reg = 0;
                std::vector<AsmOperandID> asm_arg_id_list, asm_stack_arg_id_list;
                std::set<backend::Register> used_arg_reg_set;

                for (auto ir_arg_id : inst.arg_id_list)
                {
                    auto asm_arg_id = codegen_operand(ir_arg_id, ir_context, builder, codegen_context, false, true);

                    asm_arg_id_list.push_back(asm_arg_id);

                    if (builder.context.get_operand(asm_arg_id)->is_float())
                    {
                        if (curr_float_reg <= 7)
                        {
                            auto reg = backend::Register{
                                (backend::FloatRegister)((int)(backend::FloatRegister::FA0) + curr_float_reg)};
                            used_arg_reg_set.insert(reg);
                            auto asm_reg_id = builder.fetch_register(reg);

                            builder.add_instruction_back(builder.fetch_float_binary_instruction(
                                backend::instruction::FloatBinary::FSGNJ, backend::instruction::FloatBinary::SINGLE,
                                asm_reg_id, asm_arg_id, asm_arg_id));

                            curr_float_reg++;
                        }
                        else
                        {
                            asm_stack_arg_id_list.push_back(asm_arg_id);
                        }
                    }
                    else
                    {
                        if (curr_general_reg <= 7)
                        {
                            auto reg = backend::Register{
                                (backend::GeneralRegister)((int)(backend::GeneralRegister::A0) + curr_general_reg)};
                            used_arg_reg_set.insert(reg);
                            auto asm_reg_id = builder.fetch_register(reg);

                            builder.add_instruction_back(
                                builder.context.get_operand(asm_arg_id)->is_global()
                                    ? builder.fetch_pseudo_load_instruction(backend::instruction::PseudoLoad::LA,
                                                                            asm_reg_id, asm_arg_id)
                                    : builder.fetch_binary_immediate_instruction(
                                          backend::instruction::BinaryImmediate::ADDI, asm_reg_id, asm_arg_id,
                                          builder.fetch_immediate(0)));

                            curr_general_reg++;
                        }
                        else
                        {
                            asm_stack_arg_id_list.push_back(asm_arg_id);
                        }
                    }
                }

                int stack_size = 8 * asm_stack_arg_id_list.size();
                int aligned_stack_size = (stack_size + 15) / 16 * 16;
                auto sp_reg_id = builder.fetch_register(backend::Register{backend::GeneralRegister::SP});

                if (stack_size > 0)
                {
                    auto asm_arg_stack_reg_id = builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);
                    auto asm_imm_id = builder.fetch_immediate(-aligned_stack_size);

                    if (backend::check_itype_immediate(-aligned_stack_size))
                    {
                        builder.add_instruction_back(builder.fetch_binary_immediate_instruction(
                            backend::instruction::BinaryImmediate::ADDI, asm_arg_stack_reg_id, sp_reg_id, asm_imm_id));
                    }
                    else
                    {
                        auto asm_tmp_id = builder.fetch_register(backend::Register{backend::GeneralRegister::T2});

                        builder.add_instruction_back(builder.fetch_li_instruction(asm_tmp_id, asm_imm_id));
                        builder.add_instruction_back(builder.fetch_binary_instruction(
                            backend::instruction::Binary::Op::ADD, asm_arg_stack_reg_id, sp_reg_id, asm_tmp_id));
                    }

                    int offset = 0;

                    for (auto asm_arg_id : asm_stack_arg_id_list)
                    {
                        if (builder.context.get_operand(asm_arg_id)->is_global())
                        {
                            auto asm_load_dst_id =
                                builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);
                            builder.add_instruction_back(builder.fetch_pseudo_load_instruction(
                                backend::instruction::PseudoLoad::LA, asm_load_dst_id, asm_arg_id));
                            asm_arg_id = asm_load_dst_id;
                        }

                        if (backend::check_itype_immediate(offset))
                        {
                            builder.add_instruction_back(
                                builder.context.get_operand(asm_arg_id)->is_float()
                                    ? builder.fetch_float_store_instruction(backend::instruction::FloatStore::Op::FSD,
                                                                            asm_arg_stack_reg_id, asm_arg_id,
                                                                            builder.fetch_immediate(offset))
                                    : builder.fetch_store_instruction(backend::instruction::Store::Op::SD,
                                                                      asm_arg_stack_reg_id, asm_arg_id,
                                                                      builder.fetch_immediate(offset)));
                        }
                        else
                        {
                            auto asm_tmp_id = builder.fetch_register(backend::Register{backend::GeneralRegister::T2});
                            builder.add_instruction_back(
                                builder.fetch_li_instruction(asm_tmp_id, builder.fetch_immediate(offset)));
                            builder.add_instruction_back(builder.fetch_binary_instruction(
                                backend::instruction::Binary::Op::ADD, asm_tmp_id, asm_arg_stack_reg_id, asm_tmp_id));

                            builder.add_instruction_back(
                                builder.context.get_operand(asm_arg_id)->is_float()
                                    ? builder.fetch_float_store_instruction(backend::instruction::FloatStore::Op::FSD,
                                                                            asm_tmp_id, asm_arg_id,
                                                                            builder.fetch_immediate(0))
                                    : builder.fetch_store_instruction(backend::instruction::Store::Op::SD, asm_tmp_id,
                                                                      asm_arg_id, builder.fetch_immediate(0)));
                        }

                        offset += 8;
                    }
                    builder.add_instruction_back(builder.fetch_binary_immediate_instruction(
                        backend::instruction::BinaryImmediate::ADDI, sp_reg_id, asm_arg_stack_reg_id,
                        builder.fetch_immediate(0)));
                }

                builder.add_instruction_back(builder.fetch_call_instruction(inst.function_name, used_arg_reg_set));

                if (stack_size > 0)
                {
                    auto asm_imm_id = builder.fetch_immediate(aligned_stack_size);
                    if (backend::check_itype_immediate(aligned_stack_size))
                    {
                        builder.add_instruction_back(builder.fetch_binary_immediate_instruction(
                            backend::instruction::BinaryImmediate::ADDI, sp_reg_id, sp_reg_id, asm_imm_id));
                    }
                    else
                    {
                        auto asm_tmp_id = builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);

                        builder.add_instruction_back(builder.fetch_li_instruction(asm_tmp_id, asm_imm_id));
                        builder.add_instruction_back(builder.fetch_binary_instruction(
                            backend::instruction::Binary::Op::ADD, sp_reg_id, sp_reg_id, asm_tmp_id));
                    }
                }

                if (inst.maybe_dst_id.has_value())
                {
                    auto asm_dst_id = codegen_vreg(inst.maybe_dst_id.value(), ir_context, builder, codegen_context);

                    if (builder.context.get_operand(asm_dst_id)->is_float())
                    {
                        auto fa0_reg_id = builder.fetch_register(backend::Register{backend::FloatRegister::FA0});

                        builder.add_instruction_back(builder.fetch_float_binary_instruction(
                            backend::instruction::FloatBinary::FSGNJ, backend::instruction::FloatBinary::SINGLE,
                            asm_dst_id, fa0_reg_id, fa0_reg_id));
                    }
                    else
                    {
                        builder.add_instruction_back(builder.fetch_binary_immediate_instruction(
                            backend::instruction::BinaryImmediate::ADDI, asm_dst_id,
                            builder.fetch_register(backend::Register{backend::GeneralRegister::A0}),
                            builder.fetch_immediate(0)));
                    }
                }
            },
            [&](ir::instruction::GetElementPtr &inst) {
                auto asm_dst_id = codegen_vreg(inst.dst_id, ir_context, builder, codegen_context);
                auto asm_ptr_id = codegen_operand(inst.ptr_id, ir_context, builder, codegen_context);

                if (builder.context.get_operand(asm_ptr_id)->is_global())
                {
                    auto asm_tmp_id = builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);

                    builder.add_instruction_back(builder.fetch_pseudo_load_instruction(
                        backend::instruction::PseudoLoad::LA, asm_tmp_id, asm_ptr_id));
                    asm_ptr_id = asm_tmp_id;
                }

                auto basis_type = inst.basis_type;

                for (auto ir_index_id : inst.index_id_list)
                {
                    auto asm_index_id = codegen_operand(ir_index_id, ir_context, builder, codegen_context);

                    auto asm_size_id = builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);
                    builder.add_instruction_back(
                        builder.fetch_li_instruction(asm_size_id, builder.fetch_immediate(basis_type->size() / 8)));

                    auto asm_mul_dst_id = builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);

                    builder.add_instruction_back(builder.fetch_binary_instruction(
                        backend::instruction::Binary::MUL, asm_mul_dst_id, asm_index_id, asm_size_id));

                    // auto asm_add_dst_id = builder.fetch_register(
                    //   backend::Register{backend::GeneralRegister::T2}
                    // );

                    auto asm_add_dst_id = builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);

                    builder.add_instruction_back(builder.fetch_binary_instruction(
                        backend::instruction::Binary::ADD, asm_add_dst_id, asm_ptr_id, asm_mul_dst_id));

                    asm_ptr_id = asm_add_dst_id;

                    if (basis_type->is<ir::type::Array>())
                    {
                        basis_type = basis_type->as<ir::type::Array>()->type;
                    }
                }

                builder.add_instruction_back(builder.fetch_binary_immediate_instruction(
                    backend::instruction::BinaryImmediate::ADDI, asm_dst_id, asm_ptr_id, builder.fetch_immediate(0)));
            },
            [&](ir::instruction::Ret &inst) {
                if (inst.maybe_value_id.has_value())
                {
                    auto asm_ret_value_id =
                        codegen_operand(inst.maybe_value_id.value(), ir_context, builder, codegen_context);

                    builder.add_instruction_back(
                        builder.context.get_operand(asm_ret_value_id)->is_float()
                            ? builder.fetch_float_binary_instruction(
                                  backend::instruction::FloatBinary::FSGNJ, backend::instruction::FloatBinary::SINGLE,
                                  builder.fetch_register(backend::Register{backend::FloatRegister::FA0}),
                                  asm_ret_value_id, asm_ret_value_id)
                            : builder.fetch_binary_immediate_instruction(
                                  backend::instruction::BinaryImmediate::ADDI,
                                  builder.fetch_register(backend::Register{backend::GeneralRegister::A0}),
                                  asm_ret_value_id, builder.fetch_immediate(0)));
                }

                builder.add_instruction_back(builder.fetch_ret_instruction());
            },
            [&](ir::instruction::Dummy &inst) {}},
        ir_inst->kind);
}

void codegen_phi_incoming_list(IrInstructionPtr ir_inst, AsmInstructionPtr asm_inst, IrContext &ir_context,
                               AsmBuilder &builder, CodegenContext &codegen_context)
{
    auto &asm_incoming_list = std::get<backend::instruction::Phi>(asm_inst->kind).incoming_list;
    auto ir_incoming_list = ir_inst->as<ir::instruction::Phi>().value().src_id_list;
    for (auto [ir_operand_id, ir_block_id] : ir_incoming_list)
    {
        asm_incoming_list.emplace_back(
            codegen_operand(ir_operand_id, ir_context, builder, codegen_context, false, false, true),
            codegen_context.basic_block_map.at(ir_block_id));
    }
}

AsmOperandID codegen_operand(IrOperandID ir_operand_id, IrContext &ir_context, AsmBuilder &builder,
                             CodegenContext &codegen_context, bool try_keep_imm, bool fmv_imm, bool in_phi)
{
    return std::visit(
        overloaded{
            [&](const ir::operand::Local &) {
                auto asm_operand_id = codegen_context.get_asm_operand_id(ir_operand_id);
                if (builder.context.get_operand(asm_operand_id)->is_local_memory())
                {
                    asm_operand_id = std::get<1>(codegen_ptr2reg(asm_operand_id, builder, true));
                }
                return asm_operand_id;
            },
            [&](const ir::operand::Global &) { return codegen_context.get_asm_operand_id(ir_operand_id); },
            [&](const ir::operand::ConstantPtr &ir_constant) {
                return std::visit(
                    overloaded{[&](const int imm) {
                                   auto asm_imm_id = builder.fetch_immediate(imm);
                                   if (in_phi || (try_keep_imm && backend::check_itype_immediate(imm)))
                                   {
                                       return asm_imm_id;
                                   }
                                   auto reg_id = builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);
                                   builder.add_instruction_back(builder.fetch_li_instruction(reg_id, asm_imm_id));
                                   return reg_id;
                               },
                               [&](const float imm) {
                                   auto tmp_reg_id =
                                       builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);
                                   auto asm_imm_id = builder.fetch_immediate(*(uint32_t *)&imm);
                                   if (in_phi)
                                   {
                                       return asm_imm_id;
                                   }
                                   builder.add_instruction_back(builder.fetch_li_instruction(tmp_reg_id, asm_imm_id));
                                   if (fmv_imm)
                                   {
                                       auto reg_id =
                                           builder.fetch_virtual_register(backend::VirtualRegisterKind::FLOAT);
                                       builder.add_instruction_back(builder.fetch_float_move_instruction(
                                           backend::instruction::FloatMove::Mode::S,
                                           backend::instruction::FloatMove::Mode::X, reg_id, tmp_reg_id));
                                       return reg_id;
                                   }
                                   else
                                   {
                                       return tmp_reg_id;
                                   }
                               },
                               [&](const auto &) -> AsmOperandID {
                                   throw std::runtime_error("try to trans un-imm operand to reg.");
                               }},
                    ir_constant->kind);
            },
            [&](const ir::operand::Parameter &param) {
                auto asm_param_id = codegen_context.get_asm_operand_id(ir_operand_id);
                if (in_phi)
                {
                    return asm_param_id;
                }
                return std::visit(
                    overloaded{
                        [&](const backend::Register &) { return asm_param_id; },
                        [&](const backend::VirtualRegister &) { return asm_param_id; },
                        [&](const backend::LocalMemory &local_mem) {
                            int offset = local_mem.offset;
                            bool is_float = ir_context.get_operand(ir_operand_id)->is_float();
                            auto asm_fp_id = builder.fetch_register(backend::Register{backend::GeneralRegister::S0});
                            auto asm_reg_id = codegen_vreg(ir_operand_id, ir_context, builder, codegen_context);
                            if (backend::check_itype_immediate(offset))
                            {
                                builder.add_instruction_back(
                                    is_float
                                        ? builder.fetch_float_load_instruction(backend::instruction::FloatLoad::Op::FLD,
                                                                               asm_reg_id, asm_fp_id,
                                                                               builder.fetch_immediate(offset))
                                        : builder.fetch_load_instruction(backend::instruction::Load::Op::LD, asm_reg_id,
                                                                         asm_fp_id, builder.fetch_immediate(offset)));
                            }
                            else
                            {
                                auto asm_tmp_id =
                                    builder.fetch_register(backend::Register{backend::GeneralRegister::T2});
                                builder.add_instruction_back(
                                    builder.fetch_li_instruction(asm_tmp_id, builder.fetch_immediate(offset)));
                                builder.add_instruction_back(builder.fetch_binary_instruction(
                                    backend::instruction::Binary::Op::ADD, asm_tmp_id, asm_fp_id, asm_tmp_id));
                                builder.add_instruction_back(
                                    is_float
                                        ? builder.fetch_float_load_instruction(backend::instruction::FloatLoad::Op::FLD,
                                                                               asm_reg_id, asm_tmp_id,
                                                                               builder.fetch_immediate(0))
                                        : builder.fetch_load_instruction(backend::instruction::Load::Op::LD, asm_reg_id,
                                                                         asm_tmp_id, builder.fetch_immediate(0)));
                            }
                            return asm_reg_id;
                        },
                        [&](const auto &) -> AsmOperandID { throw std::runtime_error("unexpact param type."); }},
                    builder.context.get_operand(asm_param_id)->kind);
            },
        },
        ir_context.get_operand(ir_operand_id)->kind);
}

inline AsmOperandID codegen_vreg(IrOperandID ir_local_id, IrContext &ir_context, AsmBuilder &builder,
                                 CodegenContext &codegen_context)
{
    return codegen_context.operand_map[ir_local_id] = builder.fetch_virtual_register(
               ir_context.get_operand(ir_local_id)->is_float() ? backend::VirtualRegisterKind::FLOAT
                                                               : backend::VirtualRegisterKind::GENERAL);
}

std::tuple<int, AsmOperandID> codegen_ptr2reg(AsmOperandID asm_ptr_id, AsmBuilder &builder, bool only_reg)
{
    if (builder.context.get_operand(asm_ptr_id)->is_virtual_reg() || builder.context.get_operand(asm_ptr_id)->is_reg())
    {
        return {0, asm_ptr_id};
    }
    int offset = std::get<backend::LocalMemory>(builder.context.get_operand(asm_ptr_id)->kind).offset;
    if (backend::check_itype_immediate(offset) && !only_reg)
    {
        return {offset, builder.fetch_register(backend::GeneralRegister::SP)};
    }
    else
    {
        auto target_reg_id = builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);
        auto tmp_reg_id = builder.fetch_virtual_register(backend::VirtualRegisterKind::GENERAL);
        builder.add_instruction_back(builder.fetch_li_instruction(tmp_reg_id, builder.fetch_immediate(offset)));
        builder.add_instruction_back(
            builder.fetch_binary_instruction(backend::instruction::Binary::Op::ADD, target_reg_id,
                                             builder.fetch_register(backend::GeneralRegister::SP), tmp_reg_id));
        return {0, target_reg_id};
    }
}

void asm_register_allocation(AsmBuilder &builder)
{
    for (auto &[function_name, function] : builder.context.function_table)
    {
        backend::RegAlloc().greedy_allocation(function, builder);
    }
}

void asm_instruction_scheduling(AsmContext &context)
{
    // TODO
}

} // namespace sed
