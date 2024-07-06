#include "ir/basic_block.h"

#include "ir/builder.h"
#include "ir/instruction.h"

namespace sed::ir {
BasicBlock::BasicBlock(BasicBlockID id, std::string parent_function_name)
  : id(id), parent_function_name(parent_function_name), next(nullptr) {
  head_inst = create_dummy_instruction();
  tail_inst = create_dummy_instruction();

  head_inst->insert_next(tail_inst);
}

void BasicBlock::append_instruction(InstructionPtr inst) {
  tail_inst->insert_prev(inst);
}

void BasicBlock::prepend_instruction(InstructionPtr inst) {
  head_inst->insert_next(inst);
}

void BasicBlock::add_use(InstructionID use_id) {
  use_id_list.insert(use_id);
}

void BasicBlock::remove_use(InstructionID use_id) {
  use_id_list.erase(use_id);
}

void BasicBlock::split(InstructionPtr inst, Builder& builder) {
  auto new_block = builder.fetch_basic_block();
  new_block->succ_list = succ_list;

  for (auto succ_id : succ_list) {
    auto succ = builder.context.get_basic_block(succ_id);
    succ->remove_pred(id);
    succ->add_pred(new_block->id);
  }
  succ_list.clear();

  insert_next(new_block);

  builder.set_curr_block(new_block);
  auto curr_inst = inst->next;

  inst->insert_next(tail_inst);

  while (curr_inst != tail_inst) {
    auto next_inst = curr_inst->next;
    builder.append_instruction(curr_inst);
    curr_inst = next_inst;
  }

  auto use_id_list_copy = use_id_list;
  for (auto use_id : use_id_list_copy) {
    auto use = builder.context.get_instruction(use_id);
    if (use->is<instruction::Phi>()) {
      auto& src_id_list = std::get<instruction::Phi>(use->kind).src_id_list;
      auto it = std::find_if(
        src_id_list.begin(), src_id_list.end(),
        [this](const auto& item) { return std::get<1>(item) == id; }
      );
      src_id_list.emplace(std::get<0>(*it), new_block->id);
      src_id_list.erase(it);
      remove_use(use_id);
      new_block->add_use(use_id);
    }
  }
}

void BasicBlock::add_pred(BasicBlockID pred_id) {
  pred_list.insert(pred_id);
}

void BasicBlock::add_succ(BasicBlockID succ_id) {
  succ_list.insert(succ_id);
}

void BasicBlock::remove_pred(BasicBlockID pred_id) {
  pred_list.erase(pred_id);
}

void BasicBlock::remove_succ(BasicBlockID succ_id) {
  succ_list.erase(succ_id);
}

void BasicBlock::insert_next(BasicBlockPtr block) {
  block->next = next;
  block->prev = shared_from_this();

  if (next) {
    next->prev = block;
  }

  next = block;
}

void BasicBlock::insert_prev(BasicBlockPtr block) {
  block->next = shared_from_this();
  block->prev = prev;

  if (auto p = prev.lock()) {
    p->next = block;
  }

  prev = block;
}

void BasicBlock::remove(Context& context) {
  auto curr_inst = head_inst->next;

  while (curr_inst != tail_inst) {
    auto next_inst = curr_inst->next;
    curr_inst->remove(context);
    curr_inst = next_inst;
  }

  if (auto p = prev.lock()) {
    p->next = next;
  }

  if (next) {
    next->prev = prev;
  }
}

bool BasicBlock::has_terminator() const {
  return head_inst->next != tail_inst &&
         tail_inst->prev.lock()->is_terminator();
}

bool BasicBlock::has_use() const {
  return use_id_list.size() - use_id_list.count(id);
}

std::set<BasicBlockID> BasicBlock::get_succ() const {
  std::set<BasicBlockID> result;
  if (head_inst->next == tail_inst) {
    return result;
  }
  auto tail_inst_ptr = tail_inst->prev.lock();
  if (tail_inst_ptr) {
    std::visit(
      overloaded{
        [&](const ir::instruction::Jump& kind) {
          result = std::set<BasicBlockID>{kind.then_block_id};
        },
        [&](const ir::instruction::Branch& kind) {
          result =
            std::set<BasicBlockID>{kind.then_block_id, kind.else_block_id};
        },
        [](const auto&) {}
      },
      tail_inst_ptr->kind
    );
  }
  return result;
}

std::string BasicBlock::get_label() const {
  return "BB_" + std::to_string(id);
}

std::string BasicBlock::to_string(Context& context) const {
  std::string s;
  s += get_label() + ": ; use count: " + std::to_string(use_id_list.size()) +
       " succ: ";
  for (auto& i : succ_list) {
    s += std::to_string(i) + ", ";
  }
  s += " pred: ";
  for (auto& i : pred_list) {
    s += std::to_string(i) + ", ";
  }
  s += '\n';
  auto curr_inst = head_inst->next;
  while (curr_inst != tail_inst) {
    s += "  " + curr_inst->to_string(context) + '\n';
    curr_inst = curr_inst->next;
  }
  return s;
}

BasicBlockPtr
create_basic_block(BasicBlockID id, std::string parent_function_name) {
  return std::make_shared<BasicBlock>(id, parent_function_name);
}

BasicBlockPtr create_dummy_basic_block() {
  return std::make_shared<BasicBlock>(
    std::numeric_limits<BasicBlockID>::max(), ""
  );
}

}  // namespace sed::ir
