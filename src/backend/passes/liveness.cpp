#include "backend/passes/liveness.h"

#include "backend/basic_block.h"
#include "backend/function.h"
#include "index.h"

namespace sed
{
namespace backend
{

void LivenessAnalysis::clear(FunctionPtr function)
{
    clear_visit(function);
    instr_id2num.clear();
    instr_num2id.clear();
    now_instruction_num = 0;
}

void LivenessAnalysis::clear_visit(FunctionPtr function)
{
    auto block = function->head_basic_block->next;
    while (block != function->tail_basic_block)
    {
        visited_map[block->id] = false;
        block = block->next;
    }
}

void LivenessAnalysis::depth_first_numbering(BasicBlockPtr block, Builder &builder)
{
    if (visited_map[block->id])
        return;

    visited_map[block->id] = true;

    use_map[block->id] = std::set<OperandID>();
    def_map[block->id] = std::set<OperandID>();

    auto now_instruction = block->head_instruction->next;
    while (now_instruction != block->tail_instruction)
    {
        auto now_intr_num = create_instruction_num();
        instr_id2num[now_instruction->id] = now_intr_num;
        instr_num2id[now_intr_num] = now_instruction->id;

        for (auto operand_id : now_instruction->use_id_list)
        {
            auto operand = builder.context.get_operand(operand_id);
            // 检查use前无def.
            if ((operand->is_reg() || operand->is_virtual_reg()) && !def_map[block->id].count(operand_id))
            {
                use_map[block->id].insert(operand_id);
            }
        }

        for (auto operand_id : now_instruction->def_id_list)
        {
            auto operand = builder.context.get_operand(operand_id);
            if (operand->is_reg() || operand->is_virtual_reg())
            {
                def_map[block->id].insert(operand_id);
            }
        }

        now_instruction = now_instruction->next;
    }

    for (auto next_block : block->successor_list)
    {
        depth_first_numbering(builder.context.get_basic_block(next_block), builder);
    }
}

void LivenessAnalysis::analyze(FunctionPtr now_function, Builder &builder)
{
    clear(now_function);
    depth_first_numbering(now_function->head_basic_block->next, builder);

    bool changed = true;
    std::queue<BasicBlockID> block_queue;

    // live analysis: in and out.
    while (changed)
    {
        // init
        changed = false;
        clear_visit(now_function);
        // bfs order from exit(last block).
        block_queue.push(now_function->tail_basic_block->prev.lock()->id);
        while (!block_queue.empty())
        {
            auto now_block_id = block_queue.front();
            block_queue.pop();

            if (visited_map[now_block_id])
            {
                continue;
            }
            visited_map[now_block_id] = true;

            auto now_block = builder.context.get_basic_block(now_block_id);
            auto in = std::set<OperandID>();  // In[B]
            auto out = std::set<OperandID>(); // Out[B]
            // bfs
            for (auto pred_id : now_block->predecessor_list)
            {
                block_queue.push(pred_id);
            }

            // compute in and out.
            // Out[B] = \cup_{S is asuccessor of B} In[S]
            for (auto succ_id : now_block->successor_list)
            {
                if (!in_map.count(succ_id))
                {
                    in_map[succ_id] = std::set<OperandID>();
                }
                for (auto operand_id : in_map[succ_id])
                {
                    out.insert(operand_id);
                }
            }

            // In[B] = Use[B] \cup (Out[B] - def[B])
            for (auto operand_id : out)
            {
                if (!def_map[now_block_id].count(operand_id))
                {
                    in.insert(operand_id);
                }
            }
            for (auto operand_id : use_map[now_block_id])
            {
                in.insert(operand_id);
            }

            // Update In and Out, determined changed.
            if (!in_map.count(now_block_id))
                in_map[now_block_id] = std::set<OperandID>();
            if (!out_map.count(now_block_id))
                out_map[now_block_id] = std::set<OperandID>();

            if (in != in_map[now_block_id])
            {
                changed = true;
                in_map[now_block_id] = in;
            }
            if (out != out_map[now_block_id])
            {
                changed = true;
                out_map[now_block_id] = out;
            }
        }
    }

    // compute live range
    auto now_block = now_function->head_basic_block->next;
    while (now_block != now_function->tail_basic_block)
    {
        auto begin_instr = now_block->head_instruction->next;
        auto end_instr = now_block->tail_instruction->prev.lock();

        auto begin_instr_num = instr_id2num[begin_instr->id];
        auto end_instr_num = instr_id2num[end_instr->id];

        std::unordered_map<OperandID, Range> range_buffer;

        for (auto operand_id : out_map[now_block->id])
        {
            auto operand = builder.context.get_operand(operand_id);
            if (operand->is_reg() || operand->is_virtual_reg())
            {
                range_buffer[operand_id] = Range{begin_instr_num, end_instr_num, 0, now_block->id};
            }
        }
        // 参见Linear Scan Register Allocation for the Java HotSpot Client Compiler.
        while (end_instr != now_block->head_instruction)
        {
            auto now_instr_num = instr_id2num[end_instr->id];
            for (auto operand_id : end_instr->def_id_list)
            {
                auto operand = builder.context.operand_table[operand_id];

                if (operand->is_reg() || operand->is_virtual_reg())
                {
                    if (!range_buffer.count(operand_id))
                    {
                        // dead def. add range.
                        live_range_map[operand_id].push_back(Range{now_instr_num, now_instr_num, 1, now_block->id});
                    }
                    else
                    {
                        range_buffer[operand_id].begin = now_instr_num;
                        range_buffer[operand_id].instruction_cnt++;
                        live_range_map[operand_id].push_back(range_buffer[operand_id]);
                        range_buffer.erase(operand_id);
                    }
                }
            }

            for (auto operand_id : end_instr->use_id_list)
            {
                auto operand = builder.context.get_operand(operand_id);

                if (operand->is_reg() || operand->is_virtual_reg())
                {
                    if (!range_buffer.count(operand_id))
                    {
                        // new use.
                        range_buffer[operand_id] = Range{begin_instr_num, now_instr_num, 1, now_block->id};
                    }
                    else
                    {
                        range_buffer[operand_id].instruction_cnt++;
                    }
                }
            }

            end_instr = end_instr->prev.lock();
        }

        for (auto [operand_id, range] : range_buffer)
        {
            live_range_map[operand_id].push_back(range);
        }

        now_block = now_block->next;
    }

    // 按开始位置来递增排序。
    for (auto &[operand_id, range_list] : live_range_map)
    {
        std::sort(range_list.begin(), range_list.end(), [](const auto &a, const auto &b) { return a.begin < b.begin; });
    }
}
} // namespace backend
} // namespace sed