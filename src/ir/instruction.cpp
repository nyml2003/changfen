#include "ir/instruction.h"

#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/function.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace sed::ir {
namespace instruction {
bool Binary::operator==(const Binary& other) const {
  return op == other.op && lsrc_id == other.lsrc_id && rsrc_id == other.rsrc_id;
}

bool ICmp::operator==(const ICmp& other) const {
  return op == other.op && lsrc_id == other.lsrc_id && rsrc_id == other.rsrc_id;
}

bool FCmp::operator==(const FCmp& other) const {
  return op == other.op && lsrc_id == other.lsrc_id && rsrc_id == other.rsrc_id;
}

bool Cast::operator==(const Cast& other) const {
  return op == other.op && src_id == other.src_id;
}

bool Ret::operator==(const Ret& other) const {
  return maybe_value_id == other.maybe_value_id;
}

bool Branch::operator==(const Branch& other) const {
  return cond_id == other.cond_id && then_block_id == other.then_block_id &&
         else_block_id == other.else_block_id;
}

bool Jump::operator==(const Jump& other) const {
  return then_block_id == other.then_block_id;
}

bool Phi::operator==(const Phi& other) const {
  return src_id_list == other.src_id_list;
}

bool Alloca::operator==(const Alloca& other) const {
  return allocated_type == other.allocated_type &&
         maybe_size_id == other.maybe_size_id &&
         maybe_align_id == other.maybe_align_id &&
         maybe_addrspace_id == other.maybe_addrspace_id &&
         alloca_for_param == other.alloca_for_param;
}

bool Load::operator==(const Load& other) const {
  return ptr_id == other.ptr_id && maybe_align_id == other.maybe_align_id;
}

bool Store::operator==(const Store& other) const {
  return value_id == other.value_id && ptr_id == other.ptr_id &&
         maybe_align_id == other.maybe_align_id;
}

bool Call::operator==(const Call& other) const {
  return function_name == other.function_name &&
         arg_id_list == other.arg_id_list;
}

bool GetElementPtr::operator==(const GetElementPtr& other) const {
  return basis_type == other.basis_type && ptr_id == other.ptr_id &&
         index_id_list == other.index_id_list;
}

bool Dummy::operator==(const Dummy& other) const {
  return true;
}

}  // namespace instruction
Instruction::Instruction(
  InstructionID id,
  InstructionKind kind,
  BasicBlockID parent_block_id
)
  : id(id), kind(kind), parent_block_id(parent_block_id) {}

void Instruction::insert_next(InstructionPtr inst) {
  inst->next = next;
  inst->prev = shared_from_this();
  if (next) {
    next->prev = inst;
  }
  next = inst;
}

void Instruction::insert_prev(InstructionPtr inst) {
  inst->prev = prev;
  inst->next = shared_from_this();
  if (auto p = prev.lock()) {
    p->next = inst;
  }
  prev = inst;
}

void Instruction::set_def(OperandID def_id) {
  maybe_def_id = def_id;
}

void Instruction::add_use(OperandID use_id) {
  use_id_list.insert(use_id);
}

void Instruction::remove_use(OperandID use_id) {
  use_id_list.erase(use_id);
}

void Instruction::replace_operand(
  OperandID old_id,
  OperandID new_id,
  Context& context
) {
  using namespace instruction;

  if (maybe_def_id.has_value() && maybe_def_id.value() == old_id) {
    maybe_def_id = new_id;
    context.get_operand(old_id)->remove_use(id);
    context.get_operand(new_id)->add_use(id);
  }

  use_id_list.erase(old_id);
  use_id_list.insert(new_id);
  context.get_operand(old_id)->remove_use(id);
  context.get_operand(new_id)->remove_use(id);

  std::visit(
    overloaded{
      [old_id, new_id](Binary& kind) {
        if (kind.dst_id == old_id) {
          kind.dst_id = new_id;
        }
        if (kind.lsrc_id == old_id) {
          kind.lsrc_id = new_id;
        }
        if (kind.rsrc_id == old_id) {
          kind.rsrc_id = new_id;
        }
      },
      [old_id, new_id](ICmp& kind) {
        if (kind.dst_id == old_id) {
          kind.dst_id = new_id;
        }
        if (kind.lsrc_id == old_id) {
          kind.lsrc_id = new_id;
        }
        if (kind.rsrc_id == old_id) {
          kind.rsrc_id = new_id;
        }
      },
      [old_id, new_id](FCmp& kind) {
        if (kind.dst_id == old_id) {
          kind.dst_id = new_id;
        }
        if (kind.lsrc_id == old_id) {
          kind.lsrc_id = new_id;
        }
        if (kind.rsrc_id == old_id) {
          kind.rsrc_id = new_id;
        }
      },
      [old_id, new_id](Cast& kind) {
        if (kind.dst_id == old_id) {
          kind.dst_id = new_id;
        }
        if (kind.src_id == old_id) {
          kind.src_id = new_id;
        }
      },
      [old_id, new_id](Ret& kind) {
        if (kind.maybe_value_id.has_value() &&
            kind.maybe_value_id.value() == old_id) {
          kind.maybe_value_id = new_id;
        }
      },
      [old_id, new_id](Branch& kind) {
        if (kind.cond_id == old_id) {
          kind.cond_id = new_id;
        }
      },
      [old_id, new_id](Jump& kind) {},
      [old_id, new_id](Phi& kind) {
        if (kind.dst_id == old_id) {
          kind.dst_id = new_id;
        }
        auto it = std::find_if(
          kind.src_id_list.begin(), kind.src_id_list.end(),
          [&old_id](auto& item) { return std::get<0>(item) == old_id; }
        );
        if (it != kind.src_id_list.end()) {
          kind.src_id_list.insert(std::make_tuple(new_id, std::get<1>(*it)));
          kind.src_id_list.erase(it);
        }
      },
      [&](Alloca& kind) {
        if (kind.dst_id == old_id) {
          kind.dst_id = new_id;
        }
        if (kind.maybe_size_id.has_value() &&
            kind.maybe_size_id.value() == old_id) {
          kind.maybe_size_id = new_id;
        }
        if (kind.maybe_align_id.has_value() &&
            kind.maybe_align_id.value() == old_id) {
          kind.maybe_align_id = new_id;
        }
        if (kind.maybe_addrspace_id.has_value() &&
            kind.maybe_addrspace_id.value() == old_id) {
          kind.maybe_addrspace_id = new_id;
        }
      },
      [&](Load& kind) {
        if (kind.dst_id == old_id) {
          kind.dst_id = new_id;
        }
        if (kind.ptr_id == old_id) {
          kind.ptr_id = old_id;
        }
      },
      [&](Store& kind) {
        if (kind.value_id == old_id) {
          kind.value_id = new_id;
        }
        if (kind.ptr_id == old_id) {
          kind.ptr_id = old_id;
        }
      },
      [&](Call& kind) {
        if (kind.maybe_dst_id.has_value() &&
            kind.maybe_dst_id.value() == old_id) {
          kind.maybe_dst_id = old_id;
        }
        for (auto& arg_id : kind.arg_id_list) {
          if (arg_id == old_id) {
            arg_id = new_id;
          }
        }
      },
      [&](GetElementPtr& kind) {
        if (kind.dst_id == old_id) {
          kind.dst_id = new_id;
        }
        if (kind.ptr_id == old_id) {
          kind.ptr_id = new_id;
        }
        for (auto& index_id : kind.index_id_list) {
          if (index_id == old_id) {
            index_id = new_id;
          }
        }
      },
      [](const Dummy& kind) {}
    },
    kind
  );
}

void Instruction::remove(Context& context) {
  using namespace instruction;

  std::visit(
    overloaded{
      [this, &context](const Binary& kind) {
        context.get_operand(kind.lsrc_id)->remove_use(id);
        context.get_operand(kind.rsrc_id)->remove_use(id);
      },
      [this, &context](const ICmp& kind) {
        context.get_operand(kind.lsrc_id)->remove_use(id);
        context.get_operand(kind.rsrc_id)->remove_use(id);
      },
      [this, &context](const FCmp& kind) {
        context.get_operand(kind.lsrc_id)->remove_use(id);
        context.get_operand(kind.rsrc_id)->remove_use(id);
      },
      [this, &context](const Cast& kind) {
        context.get_operand(kind.src_id)->remove_use(id);
      },
      [this, &context](const Ret& kind) {
        if (kind.maybe_value_id.has_value()) {
          context.get_operand(kind.maybe_value_id.value())->remove_use(id);
        }
      },
      [this, &context](const Branch& kind) {
        context.get_operand(kind.cond_id)->remove_use(id);
        context.get_basic_block(kind.then_block_id)->remove_use(id);
        context.get_basic_block(kind.else_block_id)->remove_use(id);

        context.get_basic_block(kind.then_block_id)
          ->remove_pred(parent_block_id);
        context.get_basic_block(kind.else_block_id)
          ->remove_pred(parent_block_id);

        context.get_basic_block(parent_block_id)
          ->remove_succ(kind.then_block_id);
        context.get_basic_block(parent_block_id)
          ->remove_succ(kind.else_block_id);
      },
      [this, &context](const Jump& kind) {
        context.get_basic_block(kind.then_block_id)->remove_use(id);

        context.get_basic_block(kind.then_block_id)
          ->remove_pred(parent_block_id);

        context.get_basic_block(parent_block_id)
          ->remove_succ(kind.then_block_id);
      },
      [this, &context](const Phi& kind) {
        for (auto [operand_id, block_id] : kind.src_id_list) {
          context.get_operand(operand_id)->remove_use(id);
          context.get_basic_block(block_id)->remove_use(id);
        }
      },
      [this, &context](const Alloca& kind) {
        if (kind.maybe_size_id.has_value()) {
          context.get_operand(kind.maybe_size_id.value())->remove_use(id);
        }
        if (kind.maybe_align_id.has_value()) {
          context.get_operand(kind.maybe_align_id.value())->remove_use(id);
        }
        if (kind.maybe_addrspace_id.has_value()) {
          context.get_operand(kind.maybe_addrspace_id.value())->remove_use(id);
        }
      },
      [this, &context](const Load& kind) {
        context.get_operand(kind.ptr_id)->remove_use(id);
        if (kind.maybe_align_id.has_value()) {
          context.get_operand(kind.maybe_align_id.value())->remove_use(id);
        }
      },
      [this, &context](const Store& kind) {
        context.get_operand(kind.value_id)->remove_use(id);
        context.get_operand(kind.ptr_id)->remove_use(id);
        if (kind.maybe_align_id.has_value()) {
          context.get_operand(kind.maybe_align_id.value())->remove_use(id);
        }
      },
      [this, &context](const Call& kind) {
        if (kind.maybe_dst_id.has_value()) {
          context.get_operand(kind.maybe_dst_id.value())->remove_use(id);
        }
        context.get_function(kind.function_name)->remove_caller(id);
        for (auto arg_id : kind.arg_id_list) {
          context.get_operand(arg_id)->remove_use(id);
        }
      },
      [this, &context](const GetElementPtr& kind) {
        context.get_operand(kind.ptr_id)->remove_use(id);
        for (auto index_id : kind.index_id_list) {
          context.get_operand(index_id)->remove_use(id);
        }
      },
      [](const Dummy& kind) {}
    },
    kind
  );

  raw_remove();
}

void Instruction::raw_remove() {
  if (auto p = prev.lock()) {
    p->next = next;
  }
  if (next) {
    next->prev = prev;
  }
}

bool Instruction::is_terminator() const {
  return is<instruction::Jump>() || is<instruction::Branch>() ||
         is<instruction::Ret>();
}

void Instruction::add_phi_operand(
  OperandID incoming_operand_id,
  BasicBlockID incoming_block_id,
  Context& context
) {
  if (!is<instruction::Phi>()) {
    return;
  }
  std::get<instruction::Phi>(kind).src_id_list.emplace(
    incoming_operand_id, incoming_block_id
  );
  context.get_operand(incoming_operand_id)->add_use(id);
  context.get_basic_block(incoming_block_id)->add_use(id);
}

std::optional<OperandID> Instruction::remove_phi_operand(
  BasicBlockID incoming_block_id,
  Context& context
) {
  if (!is<instruction::Phi>()) {
    return std::nullopt;
  }

  auto& incoming_list = std::get<instruction::Phi>(kind).src_id_list;
  auto it = std::find_if(
    incoming_list.begin(), incoming_list.end(),
    [&incoming_block_id](const auto& item) {
      return std::get<1>(item) == incoming_block_id;
    }
  );
  if (it == incoming_list.end()) {
    return std::nullopt;
  }

  auto operand_id = std::get<0>(*it);
  context.get_operand(operand_id)->remove_use(id);
  context.get_basic_block(incoming_block_id)->remove_use(id);
  incoming_list.erase(it);

  return operand_id;
}

bool Instruction::operator==(const Instruction& other) const {
  return kind == other.kind;
}

std::string Instruction::to_string(Context& context) const {
  using namespace instruction;
  std::string s;
  std::visit(
    overloaded{
      [this, &s, &context](const Binary& inst) {
        s += context.get_operand(inst.dst_id)->to_string(false) + " = ";

        switch (inst.op) {
          case BinaryOp::IAdd:
            s += "add ";
            break;
          case BinaryOp::FAdd:
            s += "fadd ";
            break;
          case BinaryOp::ISub:
            s += "sub ";
            break;
          case BinaryOp::FSub:
            s += "fsub ";
            break;
          case BinaryOp::IMul:
            s += "mul ";
            break;
          case BinaryOp::FMul:
            s += "fmul ";
            break;
          case BinaryOp::IDiv:
            s += "sdiv ";
            break;
          case BinaryOp::FDiv:
            s += "fdiv ";
            break;
          case BinaryOp::Mod:
            s += "srem ";
            break;
          case BinaryOp::Sll:
            s += "shl ";
            break;
          case BinaryOp::Srl:
            s += "lshr ";
            break;
          case BinaryOp::Sra:
            s += "ashr ";
            break;
        }
        s += context.get_operand(inst.lsrc_id)->to_string(true) + ", " +
             context.get_operand(inst.rsrc_id)->to_string(false);
      },
      [this, &s, &context](const ICmp& inst) {
        s += context.get_operand(inst.dst_id)->to_string(false) + " = icmp ";

        switch (inst.op) {
          case ICmpOp::Eq:
            s += "eq ";
            break;
          case ICmpOp::Ne:
            s += "ne ";
            break;
          case ICmpOp::Lt:
            s += "slt ";
            break;
          case ICmpOp::Le:
            s += "sle ";
            break;
        }
        s += context.get_operand(inst.lsrc_id)->to_string(true) + ", " +
             context.get_operand(inst.rsrc_id)->to_string(false);
      },
      [this, &s, &context](const FCmp& inst) {
        s += context.get_operand(inst.dst_id)->to_string(false) + " = fcmp ";

        switch (inst.op) {
          case FCmpOp::Eq:
            s += "oeq ";
            break;
          case FCmpOp::Ne:
            s += "one ";
            break;
          case FCmpOp::Lt:
            s += "olt ";
            break;
          case FCmpOp::Le:
            s += "ole ";
            break;
        }
        s += context.get_operand(inst.lsrc_id)->to_string(true) + ", " +
             context.get_operand(inst.rsrc_id)->to_string(false);
      },
      [this, &s, &context](const Cast& inst) {
        s += context.get_operand(inst.dst_id)->to_string(false) + " = ";

        switch (inst.op) {
          case CastOp::F2I:
            s += "fptosi ";
            break;
          case CastOp::I2F:
            s += "sitofp ";
            break;
          case CastOp::Ext:
            s += "zext ";
            break;
          case CastOp::Bitcast:
            s += "bitcast ";
            break;
        }
        s += context.get_operand(inst.src_id)->to_string(true) + " to " +
             context.get_operand(inst.dst_id)->type->to_string();
      },
      [this, &s, &context](const Ret& inst) {
        s += "ret ";
        if (inst.maybe_value_id.has_value()) {
          s +=
            context.get_operand(inst.maybe_value_id.value())->to_string(true);
        } else {
          s += "void";
        }
      },
      [this, &s, &context](const Branch& inst) {
        s += "br " + context.get_operand(inst.cond_id)->to_string(true) +
             ", label %" +
             context.get_basic_block(inst.then_block_id)->get_label() +
             ", label %" +
             context.get_basic_block(inst.else_block_id)->get_label();
      },
      [this, &s, &context](const Jump& inst) {
        s += "br label %" +
             context.get_basic_block(inst.then_block_id)->get_label();
      },
      [this, &s, &context](const Phi& inst) {
        s += context.get_operand(inst.dst_id)->to_string(false) + " = phi " +
             context.get_operand(inst.dst_id)->type->to_string();
        for (auto& item : inst.src_id_list) {
          s += "[" + context.get_operand(std::get<0>(item))->to_string(false) +
               ", %" + context.get_basic_block(std::get<1>(item))->get_label() +
               "], ";
        }
        if (!inst.src_id_list.empty()) {
          s.pop_back();
          s.pop_back();
        }
      },
      [this, &s, &context](const Alloca& inst) {
        s += context.get_operand(inst.dst_id)->to_string(false) + " = alloca " +
             inst.allocated_type->to_string();

        if (inst.maybe_size_id.has_value()) {
          s += ", " +
               context.get_operand(inst.maybe_size_id.value())->to_string(true);
        }
        if (inst.maybe_align_id.has_value()) {
          s +=
            ", align" +
            context.get_operand(inst.maybe_align_id.value())->to_string(false);
        }
        if (inst.maybe_addrspace_id.has_value()) {
          s +=
            ", addrspace(" +
            context.get_operand(inst.maybe_align_id.value())->to_string(false) +
            ")";
        }
      },
      [this, &s, &context](const Load& inst) {
        s += context.get_operand(inst.dst_id)->to_string(false) + " = load " +
             context.get_operand(inst.dst_id)->type->to_string() + ", ptr " +
             context.get_operand(inst.ptr_id)->to_string(false);

        if (inst.maybe_align_id.has_value()) {
          s +=
            ", align" +
            context.get_operand(inst.maybe_align_id.value())->to_string(false);
        }
      },
      [this, &s, &context](const Store& inst) {
        s += "store " + context.get_operand(inst.value_id)->to_string(true) +
             ", ptr " + context.get_operand(inst.ptr_id)->to_string(false);

        if (inst.maybe_align_id.has_value()) {
          s +=
            ", align" +
            context.get_operand(inst.maybe_align_id.value())->to_string(false);
        }
      },
      [this, &s, &context](const Call& inst) {
        if (inst.maybe_dst_id.has_value()) {
          s +=
            context.get_operand(inst.maybe_dst_id.value())->to_string(false) +
            " = ";
        }
        s +=
          "call " +
          context.get_function(inst.function_name)->return_type->to_string() +
          " @" + inst.function_name + "(";
        for (size_t i = 0; i < inst.arg_id_list.size(); ++i) {
          s += context.get_operand(inst.arg_id_list[i])->to_string(true) + ", ";
        }
        if (!inst.arg_id_list.empty()) {
          s.pop_back();
          s.pop_back();
        }
        s += ")";
      },
      [this, &s, &context](const GetElementPtr& inst) {
        s += context.get_operand(inst.dst_id)->to_string(false) +
             " = getelementptr " + inst.basis_type->to_string() + ", ptr " +
             context.get_operand(inst.ptr_id)->to_string(false) + ", ";
        for (auto i : inst.index_id_list) {
          s += context.get_operand(i)->to_string(true) + ", ";
        }
        if (!inst.index_id_list.empty()) {
          s.pop_back();
          s.pop_back();
        }
      },
      [this, &s, &context](const Dummy& inst) {}
    },
    kind
  );
  return s + "; id: " + std::to_string(id);
}

InstructionPtr create_instruction(
  InstructionID id,
  InstructionKind kind,
  BasicBlockID parent_block_id
) {
  return std::make_shared<Instruction>(id, kind, parent_block_id);
}

InstructionPtr create_dummy_instruction() {
  return create_instruction(
    std::numeric_limits<OperandID>::max(), instruction::Dummy{},
    std::numeric_limits<BasicBlockID>::max()
  );
}

}  // namespace sed::ir