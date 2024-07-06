#include "ir/context.h"

#include "ir/basic_block.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace sed::ir {
OperandID Context::get_next_operand_id() {
  return next_operand_id++;
}

InstructionID Context::get_next_instruction_id() {
  return next_instruction_id++;
}

BasicBlockID Context::get_next_block_id() {
  return next_block_id++;
}

void Context::add_operand(OperandPtr operand) {
  auto id = operand->id;
  if (operand_table.find(id) != operand_table.end()) {
    throw std::runtime_error("OperandID already exists.");
  }
  operand_table[id] = operand;

  if (operand->is<operand::Global>()) {
    global_list.insert(id);
  }
}

void Context::add_block(BasicBlockPtr block) {
  auto id = block->id;
  if (block_table.find(id) != block_table.end()) {
    throw std::runtime_error("BasicBlockID already exists.");
  }
  block_table[id] = block;
}

void Context::add_instruction(InstructionPtr inst) {
  auto id = inst->id;
  if (inst_table.find(id) != inst_table.end()) {
    throw std::runtime_error("InstructionID already exists.");
  }
  inst_table[id] = inst;
}

void Context::add_function(FunctionPtr function) {
  auto name = function->name;
  if (function_table.find(name) != function_table.end()) {
    throw std::runtime_error("Function Name already exists.");
  }
  function_table[name] = function;
}

OperandPtr Context::get_operand(OperandID id) {
  return operand_table.at(id);
}

InstructionPtr Context::get_instruction(InstructionID id) {
  return inst_table.at(id);
}

BasicBlockPtr Context::get_basic_block(BasicBlockID id) {
  return block_table.at(id);
}

FunctionPtr Context::get_function(std::string name) {
  return function_table.at(name);
}

std::string Context::to_string() {
  std::string s;
  for (auto i : global_list) {
    auto global = std::get<operand::Global>(operand_table[i]->kind);
    s += " @" + global.name + " = dso_local " +
         (global.is_constant ? "constant " : "global ") +
         operand_table[global.init]->to_string(true) + '\n';
  }
  s += '\n';

  for (auto& [func_name, func] : function_table) {
    s += func->to_string(*this) + '\n';
  }
  return s;
}

}  // namespace sed::ir
