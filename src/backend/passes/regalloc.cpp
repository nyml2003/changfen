#include "backend/passes/regalloc.h"

#include "backend/basic_block.h"
#include "backend/builder.h"
#include "backend/function.h"
#include "backend/passes/cfa.h"
#include "backend/passes/liveness.h"
#include "backend/register.h"
#include "backend/utils.h"
#include "index.h"

namespace sed
{
namespace backend
{

void RegAlloc::init(FunctionPtr now_function, Builder &builder)
{
    liveness.analyze(now_function, builder);
    gen_alloc_hint(now_function, builder);

    alloc_map.clear();

    for (auto &[operand_id, live_range_list] : liveness.live_range_map)
    {
        auto operand = builder.context.get_operand(operand_id);

        if (operand->is_virtual_reg())
        {
            auto alloc_num = create_alloc_num();
            alloc_operand_map[alloc_num] = operand_id;
            alloc_range_map[alloc_num] = live_range_list;
            alloc_status_map[alloc_num] = AllocStatus::New;
        }
    }

    for (auto reg : REG_ALLOC)
    {
        occupied_map[reg] = std::set<AllocNum>();
        occupied_range_map[reg] = std::set<AllocRange>();
    }
}

void RegAlloc::get_block_weight(FunctionPtr now_function, Builder &builder)
{
    auto now_block = now_function->head_basic_block->next;
    while (now_block != now_function->tail_basic_block)
    {
        block_weight_map[now_block->id] = 1.0;
        now_block = now_block->next;
    }

    auto cfa = CFA();
    cfa.analyze(now_function, builder.context);

    now_block = now_function->head_basic_block->next;
    auto dom_map = std::unordered_map<BasicBlockID, std::set<BasicBlockID>>();
    auto queue = std::queue<BasicBlockID>();

    queue.push(now_block->id);
    while (!queue.empty())
    {
        auto now_block_id = queue.front();
        queue.pop();

        if (cfa.dom_tree.count(now_block_id))
        {
            for (auto child_id : cfa.dom_tree[now_block_id])
            {
                queue.push(child_id);
            }
        }

        auto now_dom_set = std::set<BasicBlockID>();
        if (cfa.idom_map[now_block_id].has_value())
        {
            now_dom_set.insert(cfa.idom_map[now_block_id].value());
            for (auto idom_map_id : dom_map[cfa.idom_map[now_block_id].value()])
            {
                now_dom_set.insert(idom_map_id);
            }
        }

        dom_map[now_block_id] = std::move(now_dom_set);
    }

    // back_edge: 谁家dom block是我的successor?
    auto back_edge_set = std::set<std::pair<BasicBlockID, BasicBlockID>>();
    for (const auto &[block_id, dom_set] : dom_map)
    {
        for (auto dom_id : dom_set)
        {
            auto block = builder.context.get_basic_block(block_id);
            auto dom_block = builder.context.get_basic_block(dom_id);

            if (std::find(block->successor_list.begin(), block->successor_list.end(), dom_id) !=
                block->successor_list.end())
            {
                back_edge_set.insert({block_id, dom_id});
            }
        }
    }

    for (auto [block_id, dom_id] : back_edge_set)
    {
        std::stack<BasicBlockID> stack;
        auto head_id = dom_id;
        std::set<BasicBlockID> loop_body_id_set;

        loop_body_id_set.insert(block_id);
        loop_body_id_set.insert(head_id);

        stack.push(block_id);

        while (!stack.empty())
        {
            auto now_block_id = stack.top();
            stack.pop();

            auto now_block = builder.context.get_basic_block(now_block_id);
            for (auto pred_id : now_block->predecessor_list)
            {
                if (loop_body_id_set.count(pred_id) == 0)
                {
                    loop_body_id_set.insert(pred_id);
                    stack.push(pred_id);
                }
            }
        }

        // loop weight.
        for (auto loop_body_id : loop_body_id_set)
        {
            block_weight_map[loop_body_id] *= 4.0;
        }
    }
}

double RegAlloc::get_range_weight(Range range)
{
    return range.instruction_cnt * block_weight_map[range.block_id];
}

double RegAlloc::get_spill_weight(AllocNum alloc_num)
{
    double weight = 0;
    for (auto range : alloc_range_map[alloc_num])
    {
        weight += get_range_weight(range);
    }

    auto operand_id = alloc_operand_map[alloc_num];
    if (coalesce_map.count(operand_id))
    {
        weight *= 1.5;
    }
    else if (hint_map.count(operand_id))
    {
        weight *= 2.0;
    }
    return weight;
}

AllocPriority RegAlloc::get_alloc_priority(AllocNum alloc_num)
{
    std::size_t block_cnt = 0;
    InstructionNum instruction_cnt = 0;

    for (auto range : alloc_range_map[alloc_num])
    {
        block_cnt++;
        instruction_cnt += range.instruction_cnt;
    }

    auto operand_id = alloc_operand_map[alloc_num];
    bool hinted = coalesce_map.count(operand_id) || hint_map.count(operand_id);

    return std::make_tuple(block_cnt, instruction_cnt, alloc_status_map[alloc_num], hinted);
}

void RegAlloc::try_allocate(AllocNum alloc_num, Builder &builder)
{
    auto operand = builder.context.get_operand(alloc_operand_map[alloc_num]);
    auto range_list = alloc_range_map[alloc_num];

    std::optional<Register> allocated_reg = std::nullopt;

    auto conflict_map = std::unordered_map<Register, std::set<AllocNum>, RegisterHash>();
    auto conflict_hard_set = std::set<Register>();
    std::vector<Register> register_alloc_vector = {};

    if (hint_map.count(operand->id))
    {
        if (std::find(REG_ALLOC.begin(), REG_ALLOC.end(), hint_map[operand->id]) != REG_ALLOC.end())
        {
            register_alloc_vector.push_back(hint_map[operand->id]);
        }
        for (auto reg : REG_ALLOC)
        {
            if (!(reg == hint_map[operand->id]))
            {
                register_alloc_vector.push_back(reg);
            }
        }
    }
    else
    {
        register_alloc_vector = REG_ALLOC;
    }

    for (auto reg : register_alloc_vector)
    {
        if (reg.is_float() != operand->is_float())
            continue;

        // liveness: sorted, begin smallest.
        auto alloc_range = AllocRange(range_list[0], alloc_num);
        bool is_conflict = false;
        conflict_map[reg] = std::set<AllocNum>();

        // check conflict (maybe used)
        if (REG_ARGS.count(reg) || REG_TEMP.count(reg))
        {
            auto reg_id = builder.fetch_register(reg);
            for (auto &reg_range : liveness.live_range_map[reg_id])
            {
                for (auto &range : range_list)
                {
                    // traversal to find the conflict
                    // between alloc range and the register(try to alloc) range.
                    if (range.conflict(reg_range))
                    {
                        is_conflict = true;
                        conflict_hard_set.insert(reg);
                        break;
                    }
                }
                if (is_conflict)
                    break;
            }
        }
        // 冲突，那就再见
        if (is_conflict)
            continue;

        // 无occupied，该寄存器空闲，直接alloc。
        if (occupied_map[reg].empty())
        {
            allocated_reg = reg;
            break;
        }

        // 检查最近的occupied range（且之后的range）是否冲突
        auto iter = occupied_range_map[reg].lower_bound(alloc_range);
        if (iter != occupied_range_map[reg].begin())
        {
            --iter;
        }
        while (iter != occupied_range_map[reg].end())
        {
            const auto &occupied_range = iter->range;
            for (const auto &range : range_list)
            {
                if (range.conflict(occupied_range))
                {
                    is_conflict = true;
                    conflict_map[reg].insert(iter->alloc_num);
                    break;
                }
            }
            ++iter;
        }
        // 没conflict就分配
        if (!is_conflict)
        {
            allocated_reg = reg;
            break;
        }
    }

    if (allocated_reg.has_value())
    {
        auto now_reg = allocated_reg.value();
        occupied_map[now_reg].insert(alloc_num);
        for (auto &range : range_list)
        {
            occupied_range_map[now_reg].insert(AllocRange(range, alloc_num));
        }
        // update other infomathion.
        alloc_map[alloc_num] = now_reg;
        alloc_status_map[alloc_num] = AllocStatus::Assign;

        // TODO: Hint.
        if (coalesce_map.count(operand->id))
        {
            auto coalesce_target = builder.context.get_operand(coalesce_map[operand->id]);
            hint_map[coalesce_target->id] = now_reg;
        }
    }
    else
    {
        // evict
        auto min_weight = std::numeric_limits<float>::max();
        Register min_weight_reg;

        for (const auto &[reg, conflict_list] : conflict_map)
        {
            if (conflict_hard_set.count(reg))
                continue;

            double weight = 0.0;
            for (auto conflict_id : conflict_list)
            {
                weight += get_spill_weight(conflict_id);
            }

            if (weight < min_weight)
            {
                min_weight = weight;
                min_weight_reg = reg;
            }
        }
        // min_weight_reg 必定有值。
        // 这里只是因为register没有默认的构造函数才定义为optional.
        auto alloc_weight = get_spill_weight(alloc_num);

        if (alloc_weight <= min_weight)
        {
            // Split
            alloc_status_map[alloc_num] = AllocStatus::Split;
            auto priority = get_alloc_priority(alloc_num);
            alloc_priority_queue.push(PrioritizedAlloc(alloc_num, priority));
        }
        else
        {
            // Evict the origin min_weight_reg.
            for (auto conflict_num : conflict_map[min_weight_reg])
            {
                // update status
                occupied_map[min_weight_reg].erase(conflict_num);
                auto iter = occupied_range_map[min_weight_reg].begin();
                while (iter != occupied_range_map[min_weight_reg].end())
                {
                    if (iter->alloc_num == conflict_num)
                    {
                        iter = occupied_range_map[min_weight_reg].erase(iter);
                        break;
                    }
                    else
                    {
                        ++iter;
                    }
                }

                alloc_map.erase(conflict_num);

                auto priority = get_alloc_priority(conflict_num);
                alloc_priority_queue.push(PrioritizedAlloc(conflict_num, priority));
            }

            // update min_weight_reg with alloc_num.
            occupied_map[min_weight_reg].insert(alloc_num);
            for (auto &range : range_list)
            {
                occupied_range_map[min_weight_reg].insert(AllocRange(range, alloc_num));
            }
            alloc_map[alloc_num] = min_weight_reg;
        }
    }
}

void RegAlloc::try_split(AllocNum alloc_num, Builder &builder)
{
    // simple split
    alloc_status_map[alloc_num] = AllocStatus::Spill;
    auto priority = get_alloc_priority(alloc_num);
    alloc_priority_queue.push(PrioritizedAlloc(alloc_num, priority));
}

void RegAlloc::spill(AllocNum alloc_num, FunctionPtr now_function, Builder &builder)
{
    auto instruction_def_worklist = std::vector<InstructionPtr>();
    auto instruction_use_worklist = std::vector<InstructionPtr>();

    auto operand = builder.context.get_operand(alloc_operand_map[alloc_num]);
    auto range_list = alloc_range_map[alloc_num];

    // find the instruction that all def use the operand.
    for (auto &range : range_list)
    {
        for (auto instr_num = range.begin; instr_num <= range.end; ++instr_num)
        {
            auto instruction_id = liveness.instr_num2id[instr_num];
            auto instruction = builder.context.get_instruction(instruction_id);

            if (std::find(instruction->def_id_list.begin(), instruction->def_id_list.end(), operand->id) !=
                instruction->def_id_list.end())
            {
                instruction_def_worklist.push_back(instruction);
            }
            if (std::find(instruction->use_id_list.begin(), instruction->use_id_list.end(), operand->id) !=
                instruction->use_id_list.end())
            {
                instruction_use_worklist.push_back(instruction);
            }
        }
    }

    size_t offset = 0;
    if (operand_spill_map.count(operand->id))
    {
        offset = operand_spill_map[operand->id];
    }
    else
    {
        offset = now_function->stack_frame_size;
        now_function->stack_frame_size += 8; // 令人感叹
        operand_spill_map[operand->id] = offset;
    }

    // spill at def.
    for (auto instruction : instruction_def_worklist)
    {
        auto sp_id = builder.fetch_register(Register{GeneralRegister::SP});

        if (operand->is_float())
        {
            // fload store.
            if (check_itype_immediate(offset))
            {
                auto ft0_id = builder.fetch_register(Register{FloatRegister::FT0});
                auto fsd_instr = builder.fetch_float_store_instruction(instruction::FloatStore::FSD, sp_id, ft0_id,
                                                                       builder.fetch_immediate((int32_t)offset));

                instruction->replace_def_operand(operand->id, ft0_id, builder.context);
                instruction->insert_next(fsd_instr);
            }
            else
            {
                // Too big immediate
                auto t0_id = builder.fetch_register(Register{GeneralRegister::T0});
                auto ft0_id = builder.fetch_register(Register{FloatRegister::FT0});
                auto li_instr = builder.fetch_li_instruction(t0_id, builder.fetch_immediate(offset));
                auto add_instr = builder.fetch_binary_instruction(instruction::Binary::ADD, t0_id, sp_id, t0_id);
                auto fsd_instr = builder.fetch_float_store_instruction(instruction::FloatStore::FSD, t0_id, ft0_id,
                                                                       builder.fetch_immediate(0));

                instruction->replace_def_operand(operand->id, ft0_id, builder.context);
                instruction->insert_next(li_instr);
                li_instr->insert_next(add_instr);
                add_instr->insert_next(fsd_instr);
            }
        }
        else
        {
            if (check_itype_immediate(offset))
            {
                auto t0_id = builder.fetch_register(Register{GeneralRegister::T0});
                auto sd_instr = builder.fetch_store_instruction(instruction::Store::SD, sp_id, t0_id,
                                                                builder.fetch_immediate((int32_t)offset));
                instruction->replace_def_operand(operand->id, t0_id, builder.context);
                instruction->insert_next(sd_instr);
            }
            else
            {
                auto t0_id = builder.fetch_register(Register{GeneralRegister::T0});
                auto t1_id = builder.fetch_register(Register{GeneralRegister::T1});
                auto li_instr = builder.fetch_li_instruction(t0_id, builder.fetch_immediate(offset));
                auto add_instr = builder.fetch_binary_instruction(instruction::Binary::ADD, t0_id, sp_id, t0_id);
                auto sd_instr =
                    builder.fetch_store_instruction(instruction::Store::SD, t0_id, t1_id, builder.fetch_immediate(0));

                instruction->replace_def_operand(operand->id, t1_id, builder.context);
                instruction->insert_next(li_instr);
                li_instr->insert_next(add_instr);
                add_instr->insert_next(sd_instr);
            }
        }
    }

    // Spill at use.
    for (auto instruction : instruction_use_worklist)
    {
        auto sp_id = builder.fetch_register(Register{GeneralRegister::SP});

        if (!used_temp_map.count(instruction->id))
        {
            used_temp_map[instruction->id] = std::set<Register>();
        }

        if (operand->is_float())
        {
            size_t float_temp_index = 0;
            while (float_temp_index < REG_SPILL_FLOAT.size())
            {
                if (used_temp_map[instruction->id].count(REG_SPILL_FLOAT[float_temp_index]))
                {
                    float_temp_index++;
                }
                else
                {
                    break;
                }
            }

            auto float_temp_id = builder.fetch_register(REG_SPILL_FLOAT[float_temp_index]);
            used_temp_map[instruction->id].insert(REG_SPILL_FLOAT[float_temp_index]);

            if (check_itype_immediate(offset))
            {
                auto fld_instr = builder.fetch_float_load_instruction(instruction::FloatLoad::FLD, float_temp_id, sp_id,
                                                                      builder.fetch_immediate((int32_t)offset));
                instruction->replace_use_operand(operand->id, float_temp_id, builder.context);
                instruction->insert_prev(fld_instr);
            }
            else
            {
                size_t temp_index = 0;
                while (temp_index < REG_SPILL_GENERAL.size())
                {
                    if (used_temp_map[instruction->id].count(REG_SPILL_GENERAL[temp_index]))
                    {
                        temp_index++;
                    }
                    else
                    {
                        break;
                    }
                }

                auto temp_id = builder.fetch_register(REG_SPILL_GENERAL[temp_index]);
                used_temp_map[instruction->id].insert(REG_SPILL_GENERAL[temp_index]);

                auto li_instr = builder.fetch_li_instruction(temp_id, builder.fetch_immediate(offset));
                auto add_instr = builder.fetch_binary_instruction(instruction::Binary::ADD, temp_id, sp_id, temp_id);
                auto fld_instr = builder.fetch_float_load_instruction(instruction::FloatLoad::FLD, float_temp_id,
                                                                      temp_id, builder.fetch_immediate(0));

                instruction->replace_use_operand(operand->id, float_temp_id, builder.context);
                instruction->insert_prev(fld_instr);
                fld_instr->insert_prev(add_instr);
                add_instr->insert_prev(li_instr);
            }
        }
        else
        {
            size_t temp_index = 0;
            while (temp_index < REG_SPILL_GENERAL.size())
            {
                if (used_temp_map[instruction->id].count(REG_SPILL_GENERAL[temp_index]))
                {
                    temp_index++;
                }
                else
                {
                    break;
                }
            }

            auto temp_id = builder.fetch_register(REG_SPILL_GENERAL[temp_index]);
            used_temp_map[instruction->id].insert(REG_SPILL_GENERAL[temp_index]);
            if (temp_index > 1
                // builder.register2operand[Register{backend::GeneralRegister::ZERO}]
                // ==
                //   builder.register2operand[REG_SPILL_GENERAL[temp_index]]
            )
            {
                throw std::runtime_error("ZERO!!!!!!!!!!!!!!");
            }

            if (check_itype_immediate(offset))
            {
                auto ld_instr = builder.fetch_load_instruction(instruction::Load::LD, temp_id, sp_id,
                                                               builder.fetch_immediate((int32_t)offset));
                instruction->replace_use_operand(operand->id, temp_id, builder.context);
                instruction->insert_prev(ld_instr);
            }
            else
            {
                auto li_instr = builder.fetch_li_instruction(temp_id, builder.fetch_immediate(offset));
                auto add_instr = builder.fetch_binary_instruction(instruction::Binary::ADD, temp_id, sp_id, temp_id);
                auto ld_instr =
                    builder.fetch_load_instruction(instruction::Load::LD, temp_id, temp_id, builder.fetch_immediate(0));

                instruction->replace_use_operand(operand->id, temp_id, builder.context);
                instruction->insert_prev(ld_instr);
                ld_instr->insert_prev(add_instr);
                add_instr->insert_prev(li_instr);
            }
        }
    }
}

void RegAlloc::rewrite_operands(FunctionPtr now_function, Builder &builder)
{
    for (auto [alloc_num, reg] : alloc_map)
    {
        auto operand_id = alloc_operand_map[alloc_num];
        auto range_list = alloc_range_map[alloc_num];

        for (const auto &range : range_list)
        {
            for (auto instr_num = range.begin; instr_num <= range.end; ++instr_num)
            {
                auto instruction = builder.context.get_instruction(liveness.instr_num2id[instr_num]);
                auto reg_id = builder.fetch_register(reg);

                if (std::find(instruction->def_id_list.begin(), instruction->def_id_list.end(), operand_id) !=
                    instruction->def_id_list.end())
                {
                    instruction->replace_def_operand(operand_id, reg_id, builder.context);
                }

                if (std::find(instruction->use_id_list.begin(), instruction->use_id_list.end(), operand_id) !=
                    instruction->use_id_list.end())
                {
                    instruction->replace_use_operand(operand_id, reg_id, builder.context);
                }
            }
        }
        now_function->add_saved_register(reg);
    }
}

void RegAlloc::greedy_allocation(FunctionPtr now_function, Builder &builder)
{
    init(now_function, builder);

    get_block_weight(now_function, builder);

    for (auto &[alloc_num, _] : alloc_range_map)
    {
        alloc_priority_queue.emplace(alloc_num, get_alloc_priority(alloc_num));
    }

    while (!alloc_priority_queue.empty())
    {
        auto priority_alloc = alloc_priority_queue.top();
        alloc_priority_queue.pop();

        auto alloc_num = priority_alloc.alloc_id;
        auto alloc_status = alloc_status_map[alloc_num];

        switch (alloc_status)
        {
        case AllocStatus::New:
        case AllocStatus::Assign:
            try_allocate(alloc_num, builder);
            break;
        case AllocStatus::Split:
            try_split(alloc_num, builder);
            break;
        case AllocStatus::Spill:
            spill(alloc_num, now_function, builder);
            break;
        case AllocStatus::Memory:
        case AllocStatus::Done:
            break;
        }
    }

    rewrite_operands(now_function, builder);
}

void RegAlloc::gen_alloc_hint(FunctionPtr function, Builder &builder)
{
    using namespace instruction;
    for (auto bb = function->head_basic_block->next; bb != function->tail_basic_block; bb = bb->next)
    {
        for (auto instr = bb->head_instruction->next; instr != bb->tail_instruction; instr = instr->next)
        {
            auto maybe_binary = instr->as<Binary>();
            auto maybe_binary_imm = instr->as<BinaryImmediate>();
            auto maybe_float_binary = instr->as<FloatBinary>();

            if (maybe_binary.has_value())
            {
                auto binary = maybe_binary.value();

                auto op = binary.op;
                auto rd = builder.context.get_operand(binary.rd_id);
                auto rs1 = builder.context.get_operand(binary.rs1_id);
                auto rs2 = builder.context.get_operand(binary.rs2_id);

                if (binary.op == Binary::ADD || binary.op == Binary::ADDW)
                {
                    if (rd->is_virtual_reg() && rs1->is_virtual_reg() && rs2->is_zero())
                    {
                        coalesce_map[rd->id] = rs1->id;
                        coalesce_map[rs1->id] = rd->id;
                    }
                    else if (rd->is_virtual_reg() && rs1->is_zero() && rs2->is_virtual_reg())
                    {
                        coalesce_map[rd->id] = rs2->id;
                        coalesce_map[rs2->id] = rd->id;
                    }
                    else if (rd->is_reg() && rs1->is_virtual_reg() && rs2->is_zero())
                    {
                        hint_map[rs1->id] = std::get<Register>(rd->kind);
                    }
                    else if (rd->is_reg() && rs1->is_zero() && rs2->is_virtual_reg())
                    {
                        hint_map[rs2->id] = std::get<Register>(rd->kind);
                    }
                    else if (rd->is_virtual_reg() && rs1->is_reg() && rs2->is_zero())
                    {
                        hint_map[rd->id] = std::get<Register>(rs1->kind);
                    }
                    else if (rd->is_virtual_reg() && rs1->is_zero() && rs2->is_reg())
                    {
                        hint_map[rd->id] = std::get<Register>(rs2->kind);
                    }
                }
            }
            else if (maybe_binary_imm.has_value())
            {
                auto binary_imm = maybe_binary_imm.value();

                auto op = binary_imm.op;
                auto rd = builder.context.get_operand(binary_imm.rd_id);
                auto rs = builder.context.get_operand(binary_imm.rs_id);
                auto imm = builder.context.get_operand(binary_imm.imm_id);

                if (op == BinaryImmediate::ADDI || BinaryImmediate::ADDIW)
                {
                    if (rd->is_virtual_reg() && rs->is_virtual_reg() && imm->is_zero())
                    {
                        coalesce_map[rd->id] = rs->id;
                        coalesce_map[rs->id] = rd->id;
                    }
                    else if (rd->is_reg() && rs->is_virtual_reg() && imm->is_zero())
                    {
                        hint_map[rs->id] = std::get<Register>(rd->kind);
                    }
                    else if (rd->is_virtual_reg() && rs->is_reg() && imm->is_zero())
                    {
                        hint_map[rd->id] = std::get<Register>(rs->kind);
                    }
                }
            }
            else if (maybe_float_binary.has_value())
            {
                auto float_binary = maybe_float_binary.value();

                auto op = float_binary.op;
                auto rd = builder.context.get_operand(float_binary.rd_id);
                auto rs1 = builder.context.get_operand(float_binary.rs1_id);
                auto rs2 = builder.context.get_operand(float_binary.rs2_id);

                if (float_binary.op == FloatBinary::FSGNJ && rs1->id == rs2->id)
                {
                    if (rd->is_virtual_reg() && rs1->is_virtual_reg())
                    {
                        coalesce_map[rd->id] = rs1->id;
                        coalesce_map[rs1->id] = rd->id;
                    }
                    else if (rd->is_reg() && rs1->is_virtual_reg())
                    {
                        hint_map[rs1->id] = std::get<Register>(rd->kind);
                    }
                    else if (rd->is_virtual_reg() && rs1->is_reg())
                    {
                        hint_map[rd->id] = std::get<Register>(rs1->kind);
                    }
                }
            }
        }
    }
}

} // namespace backend
} // namespace sed