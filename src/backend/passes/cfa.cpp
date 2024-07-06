#include "backend/passes/cfa.h"

#include "backend/basic_block.h"
#include "backend/function.h"
#include "index.h"

namespace sed
{
namespace backend
{

void CFA::clear()
{
    postorder_num = 0;
    postorder_vector.clear();
    postorder_number_map.clear();
    idom_map.clear();
    dom_tree.clear();
    block_id_set.clear();
    visited.clear();
    dominance_frontier_map.clear();
}

void CFA::init(FunctionPtr &now_function)
{
    auto now_block = now_function->head_basic_block->next;
    while (now_block != now_function->tail_basic_block)
    {
        idom_map[now_block->id] = std::nullopt;
        visited[now_block->id] = false;
        block_id_set.insert(now_block->id);
        now_block = now_block->next;
    }
}

void CFA::dfs(BasicBlockID block_id, Context &context)
{
    if (!visited[block_id])
    {
        return;
    }
    visited[block_id] = true;

    auto block = context.get_basic_block(block_id);
    for (auto successor : block->successor_list)
    {
        dfs(successor, context);
    }

    postorder_vector.push_back(block_id);
    postorder_number_map[block_id] = ++postorder_num;
}

void CFA::analyze(FunctionPtr now_function, Context &context)
{
    clear();
    init(now_function);

    // Postorder
    auto entry_block = now_function->head_basic_block->next;
    dfs(entry_block->id, context);

    // Reverse postorder.
    std::reverse(postorder_vector.begin(), postorder_vector.end());

    idom_map[entry_block->id] = entry_block->id;

    std::function<BasicBlockID(BasicBlockID, BasicBlockID)> intersect = [&](BasicBlockID a, BasicBlockID b) {
        auto id_a = a, id_b = b;

        while (id_a != id_b)
        {
            while (postorder_number_map[id_a] < postorder_number_map[id_b])
            {
                id_a = idom_map[id_a].value();
            }
            while (postorder_number_map[id_b] < postorder_number_map[id_a])
            {
                id_b = idom_map[id_b].value();
            }
        }

        return id_a;
    };

    // 迭代数据流分析求idom
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (auto block_id : postorder_vector)
        {
            // Skip entry block
            if (block_id == entry_block->id)
            {
                continue;
            }

            auto block = context.get_basic_block(block_id);

            std::optional<BasicBlockID> new_idom = std::nullopt;
            for (auto predecessor : block->predecessor_list)
            {
                if (idom_map[predecessor].has_value())
                {
                    new_idom = predecessor;
                    break;
                }
            }

            if (!new_idom.has_value())
                continue;

            for (auto predecessor : block->predecessor_list)
            {
                if (idom_map[predecessor].has_value())
                {
                    new_idom = intersect(new_idom.value(), predecessor);
                }
            }

            if (idom_map[block_id] != new_idom)
            {
                idom_map[block_id] = new_idom;
                changed = true;
            }
        }
    }

    idom_map[entry_block->id] = std::nullopt;

    for (const auto &[block_id, idom_id] : idom_map)
    {
        if (idom_id.has_value())
        {
            dom_tree[idom_id.value()].push_back(block_id);
        }
    }

    // Dominance Frontier

    for (auto bb_id : block_id_set)
    {
        auto bb = context.get_basic_block(bb_id);
        if (bb->predecessor_list.size() <= 1)
        {
            continue;
        }
        if (!idom_map[bb_id].has_value())
        {
            continue;
        }
        for (auto pred_id : bb->predecessor_list)
        {
            auto runner_id = pred_id;
            while (runner_id != idom_map[bb_id].value())
            {
                dominance_frontier_map[runner_id].insert(bb_id);
                runner_id = idom_map[runner_id].value();
            }
        }
    }
}

} // namespace backend
} // namespace sed