#ifndef SED_IR_PASSES_CONTROL_FLOW_ANALYSIS_H
#define SED_IR_PASSES_CONTROL_FLOW_ANALYSIS_H

#include "index.h"

namespace sed::ir
{
struct CFA
{
  private:
    Context &context;

    std::vector<size_t> sdom;                  // 半支配节点
    std::vector<BasicBlockID> dfs2id_map;      // dfs序映射表(也是bb_id的集合)
    std::map<BasicBlockID, size_t> id2dfs_map; // dfs序映射表
    std::vector<std::vector<size_t>> dfs_tree; // dfs树
    std::vector<size_t> father;                // 父节点
    std::vector<size_t> dfs_idom;              // dfs序表示的最近支配点
    size_t dfs_number;

    // trans bb_id to dfs_order
    size_t dfs(BasicBlockID curr, size_t fa);

    // use dfs_id
    void gen_sdom(size_t curr, size_t fa);

    // Semi-NCA算法
    void gen_idom(size_t curr, size_t fa);

  public:
    std::map<BasicBlockID, BasicBlockID> idom;                  // 最近支配点
    std::map<BasicBlockID, std::set<BasicBlockID>> df_set;      // 支配边界
    std::map<BasicBlockID, std::vector<BasicBlockID>> dom_tree; // 支配树

    CFA(Context &context);

    void run(FunctionPtr function);
};
} // namespace sed::ir

#endif // SED_IR_PASSES_CONTROL_FLOW_ANALYSIS_H
