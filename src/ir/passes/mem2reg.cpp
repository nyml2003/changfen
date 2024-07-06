#include "ir/passes/mem2reg.h"

#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/context.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace sed::ir {

bool Mem2reg::isAllocaPromotable(OperandID alloca) {
  for (auto use_id : builder.context.get_operand(alloca)->use_id_list) {
    auto inst = builder.context.get_instruction(use_id);
    if (!std::visit(
          overloaded{
            [&](const instruction::Store& store) {
              return store.value_id != alloca;
            },
            [&](const instruction::Load& load) { return true; },
            [&](const auto& any) { return false; },
          },
          inst->kind
        )) {
      return false;
    }
  }
  return true;
}

void Mem2reg::gen_promotable() {
  auto entry_bb = curr_function->head_block->next;
  auto curr_inst = entry_bb->head_inst->next;
  while (curr_inst != entry_bb->tail_inst) {
    auto next_inst = curr_inst->next;
    if (curr_inst->is<instruction::Alloca>()) {
      auto alloca = curr_inst->as<instruction::Alloca>().value();
      auto ptr_id = alloca.dst_id;
      if ((alloca.allocated_type->is<type::Int>() ||
           alloca.allocated_type->is<type::Float>() ||
           alloca.allocated_type->is<type::Pointer>()) &&
          isAllocaPromotable(ptr_id)) {
        promotable.insert(ptr_id);
        def_bb[alloca.dst_id] = {};
        inserted_bb[alloca.dst_id] = {};
        worklist_map[alloca.dst_id] = {};
        var_type[ptr_id] = alloca.allocated_type;
        curr_inst->remove(builder.context);
      }
    }
    curr_inst = next_inst;
  }
}

void Mem2reg::insert_phi() {
  auto curr_bb = curr_function->head_block->next;
  while (curr_bb != curr_function->tail_block) {
    auto curr_inst = curr_bb->head_inst->next;
    while (curr_inst != curr_bb->tail_inst) {
      if (curr_inst->is<instruction::Store>()) {
        auto ptr_id = curr_inst->as<instruction::Store>().value().ptr_id;
        if (promotable.count(ptr_id) &&
            def_bb[ptr_id].insert(curr_bb->id).second) {
          worklist_map[ptr_id].push(curr_bb->id);
        }
      }
      curr_inst = curr_inst->next;
    }
    curr_bb = curr_bb->next;
  }

  for (auto& [op_id, worklist] : worklist_map) {
    while (!worklist.empty()) {
      auto bb_id = worklist.front();
      worklist.pop();

      for (auto df_id : cfa.df_set[bb_id]) {
        if (inserted_bb[op_id].count(df_id)) {
          continue;
        }

        builder.set_curr_block(builder.context.get_basic_block(df_id));
        auto phi = builder.fetch_phi_instruction(
          builder.fetch_local_operand(var_type[op_id]), {}
        );
        builder.prepend_instruction_to_curr_block(phi);
        phi_map[phi->id] = op_id;
        inserted_bb[op_id].insert(df_id);

        if (!def_bb[op_id].count(df_id)) {
          worklist.push(df_id);
        }
      }
    }
  }
}

void Mem2reg::rename(BasicBlockPtr curr_bb) {
  std::map<OperandID, size_t> def_cnt;

  auto curr_inst = curr_bb->head_inst->next;
  while (curr_inst != curr_bb->tail_inst) {
    auto next_inst = curr_inst->next;
    if (std::visit(
          overloaded{
            [&](const instruction::Store& store) {
              if (promotable.count(store.ptr_id)) {
                def_stack[store.ptr_id].push(store.value_id);
                def_cnt[store.ptr_id]++;
                return true;
              }
              return false;
            },
            [&](const instruction::Load& load) {
              if (promotable.count(load.ptr_id)) {
                OperandID new_id;
                if (def_stack.empty()) {
                  new_id = var_type[load.ptr_id]->is<type::Float>()
                             ? builder.fetch_constant_operand(
                                 var_type[load.ptr_id], 0.0f
                               )
                             : builder.fetch_constant_operand(
                                 var_type[load.ptr_id], 0
                               );
                } else {
                  new_id = def_stack[load.ptr_id].top();
                }

                auto use_id_list =
                  builder.context.get_operand(load.dst_id)->use_id_list;

                for (auto use_id : use_id_list) {
                  builder.context.get_instruction(use_id)->replace_operand(
                    load.dst_id, new_id, builder.context
                  );
                }
                return true;
              }
              return false;
            },
            [&](const instruction::Phi& phi) {
              if (phi_map.count(curr_inst->id)) {
                auto var_id = phi_map.at(curr_inst->id);
                if (promotable.count(var_id)) {
                  def_stack[var_id].push(phi.dst_id);
                  def_cnt[var_id]++;
                }
              }
              return false;
            },
            [&](const auto& any) { return false; }
          },
          curr_inst->kind
        )) {
      curr_inst->remove(builder.context);
    }
    curr_inst = next_inst;
  }

  for (auto succ_id : curr_bb->succ_list) {
    curr_inst = builder.context.get_basic_block(succ_id)->head_inst->next;
    while (curr_inst->is<instruction::Phi>()) {
      if (phi_map.count(curr_inst->id)) {
        auto var_id = phi_map.at(curr_inst->id);
        curr_inst->add_phi_operand(
          def_stack[var_id].empty()
            ? (var_type[var_id]->is<type::Float>()
                 ? builder.fetch_constant_operand(var_type[var_id], 0.0f)
                 : builder.fetch_constant_operand(var_type[var_id], 0))
            : def_stack[var_id].top(),
          curr_bb->id, builder.context
        );
      }

      curr_inst = curr_inst->next;
    }
  }

  for (auto next_bb_id : cfa.dom_tree[curr_bb->id]) {
    rename(builder.context.get_basic_block(next_bb_id));
  }

  for (auto [var_id, cnt] : def_cnt) {
    while (cnt--) {
      def_stack[var_id].pop();
    }
  }
}

Mem2reg::Mem2reg(Builder& builder) : builder(builder), cfa(builder.context) {}

void Mem2reg::run() {
  for (auto& [func_name, func] : builder.context.function_table) {
    if (func->is_declare) {
      continue;
    }
    curr_function = func;
    cfa.run(func);
    promotable.clear();
    var_type.clear();
    def_bb.clear();
    inserted_bb.clear();
    worklist_map.clear();
    phi_map.clear();
    def_stack.clear();

    gen_promotable();

    insert_phi();

    rename(func->head_block->next);
  }
}

}  // namespace sed::ir
