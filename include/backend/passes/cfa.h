#ifndef SED_BACKEND_OPTIMIZE_CFA_H
#define SED_BACKEND_OPTIMIZE_CFA_H

#include "backend/context.h"
#include "index.h"

namespace sed
{
namespace backend
{

// Control Flow Analysis: dominate tree.
class CFA
{
    std::size_t postorder_num = 0;
    std::vector<std::size_t> postorder_vector;
    std::unordered_map<BasicBlockID, std::size_t> postorder_number_map;

    std::unordered_map<BasicBlockID, bool> visited;
    std::set<BasicBlockID> block_id_set;
    void dfs(BasicBlockID block_id, Context &context);

  public:
    CFA() = default;
    std::unordered_map<BasicBlockID, std::optional<BasicBlockID>> idom_map;
    std::unordered_map<BasicBlockID, std::vector<BasicBlockID>> dom_tree;

    std::unordered_map<BasicBlockID, std::set<BasicBlockID>> dominance_frontier_map;
    void analyze(FunctionPtr now_function, Context &context);
    void clear();
    void init(FunctionPtr &now_function);
};

} // namespace backend
} // namespace sed

#endif