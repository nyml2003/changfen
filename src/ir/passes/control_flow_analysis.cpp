#include "ir/passes/control_flow_analysis.h"

#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/function.h"

namespace sed::ir {

CFA::CFA(Context& context) : context(context) {}

size_t CFA::dfs(BasicBlockID curr, size_t fa) {
  size_t dfs_id = dfs_number++;
  dfs2id_map.push_back(curr);
  id2dfs_map[curr] = dfs_id;
  dfs_tree.emplace_back();
  father.push_back(fa);
  for (auto next : context.get_basic_block(curr)->succ_list) {
    if (id2dfs_map.count(next)) {
      continue;
    }
    dfs(next, dfs_id);
    dfs_tree[dfs_id].push_back(id2dfs_map[next]);
  }
  return dfs_id;
}

void CFA::gen_sdom(size_t curr, size_t fa) {
  std::vector<size_t> vlist(dfs_tree[curr].size());
  std::reverse_copy(
    dfs_tree[curr].begin(), dfs_tree[curr].end(), vlist.begin()
  );
  size_t curr_sdom = std::numeric_limits<size_t>::max();
  for (auto next : vlist) {
    if (next == fa) {
      continue;
    }
    gen_sdom(next, curr);
  }
  for (auto pred : context.get_basic_block(dfs2id_map[curr])->pred_list) {
    pred = id2dfs_map[pred];
    curr_sdom = std::min(curr_sdom, pred);
    while (pred > curr) {
      curr_sdom = std::min(curr_sdom, sdom[pred]);
      pred = father[pred];
    }
  }
  sdom[curr] = curr_sdom;
}

void CFA::gen_idom(size_t curr, size_t fa) {
  dfs_idom[curr] = fa;
  while (dfs_idom[curr] > sdom[curr]) {
    dfs_idom[curr] = dfs_idom[dfs_idom[curr]];
  }

  for (auto next : dfs_tree[curr]) {
    if (next == fa) {
      continue;
    }
    gen_idom(next, curr);
  }
}

void CFA::run(FunctionPtr function) {
  sdom.clear();
  dfs2id_map.clear();
  id2dfs_map.clear();
  dfs_tree.clear();
  father.clear();
  dfs_idom.clear();
  dfs_number = 0;
  idom.clear();
  df_set.clear();
  dom_tree.clear();

  auto entry_bb_id = function->head_block->next->id;

  // calculate idom using semi-NCA
  dfs(entry_bb_id, 0);
  sdom.resize(dfs_number);
  dfs_idom.resize(dfs_number);
  gen_sdom(0, 0);
  gen_idom(0, 0);

  // transform dfs_order-based idom to bb_id-based idom
  for (size_t i = 0; i < dfs_number; ++i) {
    idom[dfs2id_map[i]] = dfs2id_map[dfs_idom[i]];
  }

  // construct dom_tree
  for (auto [u, v] : idom) {
    if (u != v) {
      dom_tree[v].push_back(u);
    }
  }

  // generate df_set
  for (auto bb_id : dfs2id_map) {
    auto& pred_list = context.get_basic_block(bb_id)->pred_list;
    if (pred_list.size() > 1) {
      for (auto runner : pred_list) {
        while (runner != idom[bb_id]) {
          df_set[runner].insert(bb_id);
          runner = idom[runner];
        }
      }
    }
  }
}

}  // namespace sed::ir
