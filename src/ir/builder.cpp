#include "ir/builder.h"

#include "ir/basic_block.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace sed::ir {
TypePtr Builder::fetch_int_type(const int size) {
  return std::make_shared<Type>(type::Int{size});
}
TypePtr Builder::fetch_float_type() {
  return std::make_shared<Type>(type::Float());
}
TypePtr Builder::fetch_void_type() {
  return std::make_shared<Type>(type::Void());
}
TypePtr Builder::fetch_pointer_type() {
  return std::make_shared<Type>(type::Pointer());
}
TypePtr Builder::fetch_array_type(size_t length, TypePtr element_type) {
  return std::make_shared<Type>(type::Array{length, element_type});
}

OperandID Builder::fetch_operand(TypePtr type, OperandKind kind) {
  auto id = context.get_next_operand_id();
  auto operand = std::make_shared<Operand>(id, kind, type);
  context.add_operand(operand);
  return id;
}

operand::ConstantPtr
Builder::fetch_constant(TypePtr type, operand::ConstantKind kind) {
  return std::make_shared<operand::Constant>(operand::Constant{kind, type});
}

OperandID
Builder::fetch_constant_operand(TypePtr type, operand::ConstantKind kind) {
  return fetch_operand(
    type, std::make_shared<operand::Constant>(operand::Constant{kind, type})
  );
}

OperandID Builder::fetch_global_operand(
  TypePtr type,
  std::string name,
  bool is_constant,
  OperandID init
) {
  return fetch_operand(type, operand::Global{name, is_constant, init});
}

OperandID Builder::fetch_parameter_operand(TypePtr type, std::string name) {
  return fetch_operand(type, operand::Parameter{name});
}

OperandID Builder::fetch_local_operand(TypePtr type) {
  return fetch_operand(type, operand::Local{});
}

InstructionPtr Builder::fetch_binary_instruction(
  instruction::BinaryOp op,
  OperandID dst_id,
  OperandID lsrc_id,
  OperandID rsrc_id
) {
  auto id = context.get_next_instruction_id();

  auto kind =
    InstructionKind(instruction::Binary{op, dst_id, lsrc_id, rsrc_id});

  auto inst = create_instruction(id, kind, curr_block->id);

  context.add_instruction(inst);

  context.get_operand(dst_id)->set_def(id);
  context.get_operand(lsrc_id)->add_use(id);
  context.get_operand(rsrc_id)->add_use(id);

  inst->set_def(dst_id);
  inst->add_use(lsrc_id);
  inst->add_use(rsrc_id);

  return inst;
}

InstructionPtr Builder::fetch_icmp_instruction(
  instruction::ICmpOp op,
  OperandID dst_id,
  OperandID lsrc_id,
  OperandID rsrc_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::ICmp{op, dst_id, lsrc_id, rsrc_id});

  auto inst = create_instruction(id, kind, curr_block->id);

  context.add_instruction(inst);

  context.get_operand(dst_id)->set_def(id);
  context.get_operand(lsrc_id)->add_use(id);
  context.get_operand(rsrc_id)->add_use(id);

  inst->set_def(dst_id);
  inst->add_use(lsrc_id);
  inst->add_use(rsrc_id);

  return inst;
}

InstructionPtr Builder::fetch_fcmp_instruction(
  instruction::FCmpOp op,
  OperandID dst_id,
  OperandID lsrc_id,
  OperandID rsrc_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::FCmp{op, dst_id, lsrc_id, rsrc_id});

  auto inst = create_instruction(id, kind, curr_block->id);

  context.add_instruction(inst);

  context.get_operand(dst_id)->set_def(id);
  context.get_operand(lsrc_id)->add_use(id);
  context.get_operand(rsrc_id)->add_use(id);

  inst->set_def(dst_id);
  inst->add_use(lsrc_id);
  inst->add_use(rsrc_id);

  return inst;
}

InstructionPtr Builder::fetch_cast_instruction(
  instruction::CastOp op,
  OperandID dst_id,
  OperandID src_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Cast{op, dst_id, src_id});

  auto inst = create_instruction(id, kind, curr_block->id);

  context.add_instruction(inst);

  context.get_operand(dst_id)->set_def(id);
  context.get_operand(src_id)->add_use(id);

  inst->set_def(dst_id);
  inst->add_use(src_id);

  return inst;
}

InstructionPtr Builder::fetch_ret_instruction(
  std::optional<OperandID> maybe_value_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Ret{maybe_value_id});

  auto inst = create_instruction(id, kind, curr_block->id);

  context.add_instruction(inst);

  if (maybe_value_id.has_value()) {
    context.get_operand(maybe_value_id.value())->add_use(id);
    inst->add_use(maybe_value_id.value());
  }

  return inst;
}

InstructionPtr Builder::fetch_branch_instruction(
  OperandID cond_id,
  BasicBlockID then_block_id,
  BasicBlockID else_block_id
) {
  auto id = context.get_next_instruction_id();

  auto kind =
    InstructionKind(instruction::Branch{cond_id, then_block_id, else_block_id});

  auto inst = create_instruction(id, kind, curr_block->id);

  context.add_instruction(inst);

  context.get_operand(cond_id)->add_use(id);
  context.get_basic_block(then_block_id)->add_use(id);
  context.get_basic_block(else_block_id)->add_use(id);

  inst->add_use(cond_id);

  return inst;
}

InstructionPtr Builder::fetch_jump_instruction(BasicBlockID then_block_id) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Jump{then_block_id});

  auto inst = create_instruction(id, kind, curr_block->id);

  context.add_instruction(inst);

  context.get_basic_block(then_block_id)->add_use(id);

  return inst;
}

InstructionPtr Builder::fetch_phi_instruction(
  OperandID dst_id,
  std::set<std::tuple<OperandID, BasicBlockID>> src_id_list
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Phi{dst_id, src_id_list});

  auto inst = create_instruction(id, kind, curr_block->id);

  context.add_instruction(inst);

  inst->set_def(dst_id);

  context.get_operand(dst_id)->set_def(id);
  for (auto [operand_id, block_id] : src_id_list) {
    context.get_operand(operand_id)->add_use(id);
    context.get_basic_block(block_id)->add_use(id);
    inst->add_use(operand_id);
  }

  return inst;
}

InstructionPtr Builder::fetch_alloca_instruction(
  OperandID dst_id,
  TypePtr allocated_type,
  std::optional<OperandID> maybe_size_id,
  std::optional<OperandID> maybe_align_id,
  std::optional<OperandID> maybe_addrspace_id,
  bool alloca_for_param
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Alloca{
    dst_id, allocated_type, maybe_size_id, maybe_align_id, maybe_addrspace_id,
    alloca_for_param
  });

  auto inst = create_instruction(id, kind, curr_block->id);

  context.add_instruction(inst);

  inst->set_def(dst_id);

  context.get_operand(dst_id)->set_def(id);
  if (maybe_size_id.has_value()) {
    context.get_operand(maybe_size_id.value())->add_use(id);
    inst->add_use(maybe_size_id.value());
  }

  if (maybe_align_id.has_value()) {
    context.get_operand(maybe_align_id.value())->add_use(id);
    inst->add_use(maybe_align_id.value());
  }

  if (maybe_addrspace_id.has_value()) {
    context.get_operand(maybe_addrspace_id.value())->add_use(id);
    inst->add_use(maybe_addrspace_id.value());
  }

  return inst;
}

InstructionPtr Builder::fetch_load_instruction(
  OperandID dst_id,
  OperandID ptr_id,
  std::optional<OperandID> maybe_align_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Load{
    dst_id,
    ptr_id,
    maybe_align_id,
  });

  auto inst = create_instruction(id, kind, curr_block->id);

  context.add_instruction(inst);

  context.get_operand(dst_id)->set_def(id);
  context.get_operand(ptr_id)->add_use(id);

  inst->set_def(dst_id);
  inst->add_use(ptr_id);

  if (maybe_align_id.has_value()) {
    context.get_operand(maybe_align_id.value())->add_use(id);
    inst->add_use(maybe_align_id.value());
  }

  return inst;
}

InstructionPtr Builder::fetch_store_instruction(
  OperandID value_id,
  OperandID ptr_id,
  std::optional<OperandID> maybe_align_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Store{
    value_id,
    ptr_id,
    maybe_align_id,
  });

  auto inst = create_instruction(id, kind, curr_block->id);

  context.add_instruction(inst);

  context.get_operand(value_id)->add_use(id);
  context.get_operand(ptr_id)->add_use(id);

  inst->add_use(value_id);
  inst->add_use(ptr_id);

  if (maybe_align_id.has_value()) {
    context.get_operand(maybe_align_id.value())->add_use(id);
    inst->add_use(maybe_align_id.value());
  }

  return inst;
}

InstructionPtr Builder::fetch_call_instruction(
  std::optional<OperandID> maybe_dst_id,
  std::string function_name,
  std::vector<OperandID> arg_id_list
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Call{
    maybe_dst_id,
    function_name,
    arg_id_list,
  });

  auto inst = create_instruction(id, kind, curr_block->id);

  context.add_instruction(inst);

  if (maybe_dst_id.has_value()) {
    context.get_operand(maybe_dst_id.value())->set_def(id);
    inst->set_def(maybe_dst_id.value());
  }

  for (auto arg_id : arg_id_list) {
    context.get_operand(arg_id)->add_use(id);
    inst->add_use(arg_id);
  }
  context.get_function(function_name)->caller_id_list.insert(id);

  return inst;
}

InstructionPtr Builder::fetch_getelementptr_instruction(
  OperandID dst_id,
  TypePtr basis_type,
  OperandID ptr_id,
  std::vector<OperandID> index_id_list
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::GetElementPtr{
    dst_id,
    basis_type,
    ptr_id,
    index_id_list,
  });

  auto inst = create_instruction(id, kind, curr_block->id);

  context.add_instruction(inst);

  context.operand_table[dst_id]->set_def(id);
  context.operand_table[ptr_id]->add_use(id);

  inst->set_def(dst_id);
  inst->add_use(ptr_id);

  for (auto index_id : index_id_list) {
    context.get_operand(index_id)->add_use(id);
    inst->add_use(index_id);
  }

  return inst;
}

void Builder::append_instruction(InstructionPtr inst) {
  if (curr_block->has_terminator()) {
    // inst->remove(context);
    // return;
    set_curr_block(fetch_basic_block());
  }

  curr_block->append_instruction(inst);
  inst->parent_block_id = curr_block->id;

  if (inst->is<instruction::Branch>()) {
    const auto& branch = inst->as<instruction::Branch>().value();

    curr_block->add_succ(branch.then_block_id);
    curr_block->add_succ(branch.else_block_id);

    context.get_basic_block(branch.then_block_id)->add_pred(curr_block->id);
    context.get_basic_block(branch.else_block_id)->add_pred(curr_block->id);
  } else if (inst->is<instruction::Jump>()) {
    const auto& jump = inst->as<instruction::Jump>().value();

    curr_block->add_succ(jump.then_block_id);
    context.get_basic_block(jump.then_block_id)->add_pred(curr_block->id);
  }
}

void Builder::prepend_instruction_to_curr_block(InstructionPtr inst) {
  curr_block->prepend_instruction(inst);
  inst->parent_block_id = curr_block->id;
}

void Builder::prepend_instruction_to_curr_function(InstructionPtr inst) {
  curr_function->head_block->next->prepend_instruction(inst);
  inst->parent_block_id = curr_function->head_block->next->id;
}

BasicBlockPtr Builder::fetch_basic_block() {
  auto id = context.get_next_block_id();
  auto block = create_basic_block(id, curr_function->name);

  context.add_block(block);
  return block;
}

void Builder::append_basic_block(BasicBlockPtr block) {
  curr_function->append_basic_block(block);
}

void Builder::set_curr_block(BasicBlockPtr block) {
  curr_block = block;
}

void Builder::switch_function(std::string function_name) {
  curr_function = context.get_function(function_name);
}

void Builder::add_function(
  std::string function_name,
  std::vector<OperandID> param_id_list,
  TypePtr return_type,
  bool is_declare
) {
  auto function = std::make_shared<Function>(
    function_name, return_type, param_id_list, is_declare
  );
  context.add_function(function);
  curr_function = function;
}

}  // namespace sed::ir
