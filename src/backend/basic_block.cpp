#include "backend/basic_block.h"
#include "backend/instruction.h"

namespace sed {
namespace backend {

BasicBlock::BasicBlock(BasicBlockID id, std::string parent_function_name)
  : id(id), parent_function_name(parent_function_name), next(nullptr) {
  this->head_instruction = create_dummy_instruction();
  this->tail_instruction = create_dummy_instruction();
  this->tail_instruction->insert_prev(this->head_instruction);
}

void BasicBlock::add_instruction_front(InstructionPtr instruction) {
  this->head_instruction->insert_next(instruction);
}

void BasicBlock::add_instruction_back(InstructionPtr instruction) {
  this->tail_instruction->insert_prev(instruction);
}

void BasicBlock::insert_next(BasicBlockPtr basic_block) {
  basic_block->next = this->next;
  basic_block->prev = this->shared_from_this();

  if (this->next) {
    this->next->prev = basic_block;
  }
  this->next = basic_block;
}

void BasicBlock::insert_prev(BasicBlockPtr basic_block) {
  basic_block->prev = this->prev;
  basic_block->next = this->shared_from_this();

  if (this->prev.lock()) {
    this->prev.lock()->next = basic_block;
  }
  this->prev = basic_block;
}

void BasicBlock::add_predecessor(BasicBlockID predecessor_id) {
  if (std::find(
        this->predecessor_list.begin(), this->predecessor_list.end(),
        predecessor_id
      ) != this->predecessor_list.end()) {
    return;
  }
  this->predecessor_list.push_back(predecessor_id);
}

void BasicBlock::add_successor(BasicBlockID successor_id) {
  if (std::find(
        this->successor_list.begin(), this->successor_list.end(), successor_id
      ) != this->successor_list.end()) {
    return;
  }
  this->successor_list.push_back(successor_id);
}

void BasicBlock::remove_predecessor(BasicBlockID predecessor_id) {
  auto id = std::find(
    this->predecessor_list.begin(), this->predecessor_list.end(), predecessor_id
  );
  if (id != this->predecessor_list.end()) {
    this->predecessor_list.erase(id);
  }
}

void BasicBlock::remove_successor(BasicBlockID successor_id) {
  auto id = std::find(
    this->successor_list.begin(), this->successor_list.end(), successor_id
  );
  if (id != this->successor_list.end()) {
    this->successor_list.erase(id);
  }
}

std::string BasicBlock::get_label() {
  return ".basicblock_" + std::to_string(id);
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

std::string BasicBlock::to_string(Context& context) {
  std::string result = this->get_label() + ":\n";
  auto now_intruction = this->head_instruction->next;

  result += "\t# preds = ";
  for (auto predecessor_id : this->predecessor_list) {
    result += std::to_string(predecessor_id) + ", ";
  }
  result += "\n\t# succs = ";
  for (auto successor_id : this->successor_list) {
    result += std::to_string(successor_id) + ", ";
  }
  result += "\n";

  while (now_intruction != this->tail_instruction) {
    result += "\t" + now_intruction->to_string(context) + "\n";
    now_intruction = now_intruction->next;
  }

  return result;
}

}  // namespace backend
}  // namespace sed