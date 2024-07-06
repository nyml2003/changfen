#include "ir/function.h"

#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace sed::ir {
Function::Function(
  std::string name,
  TypePtr return_type,
  std::vector<OperandID> param_id_list,
  bool is_declare
)
  : name(name),
    return_type(return_type),
    param_id_list(param_id_list),
    is_declare(is_declare) {
  head_block = create_dummy_basic_block();
  tail_block = create_dummy_basic_block();
  head_block->insert_next(tail_block);

  maybe_return_id = std::nullopt;
  maybe_return_block = std::nullopt;
}

void Function::append_basic_block(BasicBlockPtr block) {
  if (maybe_return_block.has_value()) {
    maybe_return_block.value()->insert_prev(block);
  } else {
    tail_block->insert_prev(block);
  }
}

void Function::prepend_basic_block(BasicBlockPtr block) {
  head_block->insert_next(block);
}

void Function::add_terminator(Builder& builder) {
  auto curr_block = head_block->next;
  while (curr_block != tail_block) {
    if (curr_block->has_terminator()) {
      curr_block = curr_block->next;
      continue;
    }
    builder.set_curr_block(curr_block);
    builder.append_instruction(
      builder.fetch_jump_instruction(curr_block->next->id)
    );
    curr_block = curr_block->next;
  }
}

void Function::remove_caller(InstructionID caller_id) {
  caller_id_list.erase(caller_id);
}

void Function::remove_unused_block(Context& context) {
  auto curr_block = head_block->next->next;
  while (curr_block != tail_block) {
    auto next_block = curr_block->next;
    if (!curr_block->has_use()) {
      curr_block->remove(context);
    }
    curr_block = next_block;
  }
}

std::string Function::to_string(Context& context) const {
  std::string s;
  if (is_declare) {
    s += "declare ";
  } else {
    s += "define dso_local ";
  }
  s += return_type->to_string() + " @" + name + "(";
  for (auto i : param_id_list) {
    s += context.get_operand(i)->to_string(true) + ", ";
  }
  if (!param_id_list.empty()) {
    s.pop_back();
    s.pop_back();
  }
  s += ")";
  if (is_declare) {
    s += "\n";
    return s;
  }
  s += " {\n";
  auto curr_block = head_block->next;
  while (curr_block != tail_block) {
    s += curr_block->to_string(context);
    curr_block = curr_block->next;
  }
  s += "}\n";
  return s;
}

}  // namespace sed::ir
