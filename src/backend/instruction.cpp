#include "backend/instruction.h"
#include "backend/basic_block.h"
#include "backend/operand.h"

namespace sed {
namespace backend {

Instruction::Instruction(
  InstructionID id,
  InstructionKind kind,
  BasicBlockID parent_block_id
)
  : id(id), kind(kind), parent_block_id(parent_block_id), next(nullptr) {}

void Instruction::add_def(OperandID def_id) {
  def_id_list.push_back(def_id);
}

void Instruction::add_use(OperandID use_id) {
  use_id_list.push_back(use_id);
}

void Instruction::insert_next(InstructionPtr instruction) {
  instruction->next = this->next;
  instruction->prev = this->shared_from_this();

  if (this->next) {
    this->next->prev = instruction;
  }
  this->next = instruction;
}

void Instruction::insert_prev(InstructionPtr instruction) {
  instruction->next = this->shared_from_this();
  instruction->prev = this->prev;

  if (auto prev = this->prev.lock()) {
    prev->next = instruction;
  }
  this->prev = instruction;
}

std::optional<BasicBlockID> Instruction::get_basic_block_id_if_branch() const {
  if (auto branch = std::get_if<instruction::Branch>(&kind)) {
    return branch->block_id;
  } else if (auto jump = std::get_if<instruction::Jump>(&kind)) {
    return jump->block_id;
  }
  return std::nullopt;
}

InstructionPtr create_instruction(
  InstructionID id,
  InstructionKind kind,
  BasicBlockID parent_block_id
) {
  return std::make_shared<Instruction>(id, kind, parent_block_id);
};

InstructionPtr create_dummy_instruction() {
  return create_instruction(
    std::numeric_limits<InstructionID>::max(), instruction::Dummy{},
    std::numeric_limits<BasicBlockID>::max()
  );
};

void Instruction::replace_operand(
  OperandID old_operand_id,
  OperandID new_operand_id,
  Context& context
) {
  using namespace instruction;
  if (old_operand_id == new_operand_id)
    return;

  auto old_operand = context.get_operand(old_operand_id);
  auto new_operand = context.get_operand(new_operand_id);

  auto def_iter =
    std::find(def_id_list.begin(), def_id_list.end(), old_operand_id);
  if (def_iter != def_id_list.end()) {
    this->def_id_list.erase(def_iter);
    this->def_id_list.push_back(new_operand_id);
    old_operand->remove_def(this->id);
    new_operand->add_def(this->id);
  }

  auto use_iter =
    std::find(use_id_list.begin(), use_id_list.end(), old_operand_id);
  if (use_iter != use_id_list.end()) {
    this->use_id_list.erase(use_iter);
    this->use_id_list.push_back(new_operand_id);
    old_operand->remove_use(this->id);
    new_operand->add_use(this->id);
  }

  std::visit(
    overloaded{
      [&](Load& load) {
        if (load.rd_id == old_operand_id)
          load.rd_id = new_operand_id;
        if (load.rs_id == old_operand_id)
          load.rs_id = new_operand_id;
        if (load.imm_id == old_operand_id)
          load.imm_id = new_operand_id;
      },
      [&](PseudoLoad& pseudo_load) {
        if (pseudo_load.rd_id == old_operand_id)
          pseudo_load.rd_id = new_operand_id;
        if (pseudo_load.symbol_id == old_operand_id)
          pseudo_load.symbol_id = new_operand_id;
      },
      [&](Store& store) {
        if (store.rs1_id == old_operand_id)
          store.rs1_id = new_operand_id;
        if (store.rs2_id == old_operand_id)
          store.rs2_id = new_operand_id;
        if (store.imm_id == old_operand_id)
          store.imm_id = new_operand_id;
      },
      [&](PseudoStore& pseudo_store) {
        if (pseudo_store.rd_id == old_operand_id)
          pseudo_store.rd_id = new_operand_id;
        if (pseudo_store.rt_id == old_operand_id)
          pseudo_store.rt_id = new_operand_id;
        if (pseudo_store.symbol_id == old_operand_id)
          pseudo_store.symbol_id = new_operand_id;
      },
      [&](Binary& binary) {
        if (binary.rs1_id == old_operand_id)
          binary.rs1_id = new_operand_id;
        if (binary.rs2_id == old_operand_id)
          binary.rs2_id = new_operand_id;
        if (binary.rd_id == old_operand_id)
          binary.rd_id = new_operand_id;
      },
      [&](BinaryImmediate& binary_immediate) {
        if (binary_immediate.rs_id == old_operand_id)
          binary_immediate.rs_id = new_operand_id;
        if (binary_immediate.rd_id == old_operand_id)
          binary_immediate.rd_id = new_operand_id;
        if (binary_immediate.imm_id == old_operand_id)
          binary_immediate.imm_id = new_operand_id;
      },
      [&](FloatLoad& load) {
        if (load.rd_id == old_operand_id)
          load.rd_id = new_operand_id;
        if (load.rs_id == old_operand_id)
          load.rs_id = new_operand_id;
        if (load.imm_id == old_operand_id)
          load.imm_id = new_operand_id;
      },
      [&](FloatPseudoLoad& pseudo_load) {
        if (pseudo_load.rd_id == old_operand_id)
          pseudo_load.rd_id = new_operand_id;
        if (pseudo_load.symbol_id == old_operand_id)
          pseudo_load.symbol_id = new_operand_id;
        if (pseudo_load.rt_id == old_operand_id)
          pseudo_load.rt_id = new_operand_id;
      },
      [&](FloatStore& store) {
        if (store.rs1_id == old_operand_id)
          store.rs1_id = new_operand_id;
        if (store.rs2_id == old_operand_id)
          store.rs2_id = new_operand_id;
        if (store.imm_id == old_operand_id)
          store.imm_id = new_operand_id;
      },
      [&](FloatPseudoStore& pseudo_store) {
        if (pseudo_store.rd_id == old_operand_id)
          pseudo_store.rd_id = new_operand_id;
        if (pseudo_store.rt_id == old_operand_id)
          pseudo_store.rt_id = new_operand_id;
        if (pseudo_store.symbol_id == old_operand_id)
          pseudo_store.symbol_id = new_operand_id;
      },
      [&](FloatMove& move) {
        if (move.rs_id == old_operand_id)
          move.rs_id = new_operand_id;
        if (move.rd_id == old_operand_id)
          move.rd_id = new_operand_id;
      },
      [&](FloatConvert& convert) {
        if (convert.rs_id == old_operand_id)
          convert.rs_id = new_operand_id;
        if (convert.rd_id == old_operand_id)
          convert.rd_id = new_operand_id;
      },
      [&](FloatBinary& binary) {
        if (binary.rs1_id == old_operand_id)
          binary.rs1_id = new_operand_id;
        if (binary.rs2_id == old_operand_id)
          binary.rs2_id = new_operand_id;
        if (binary.rd_id == old_operand_id)
          binary.rd_id = new_operand_id;
      },
      [&](FloatMultiplyAdd& multiply_add) {
        if (multiply_add.rs1_id == old_operand_id)
          multiply_add.rs1_id = new_operand_id;
        if (multiply_add.rs2_id == old_operand_id)
          multiply_add.rs2_id = new_operand_id;
        if (multiply_add.rs3_id == old_operand_id)
          multiply_add.rs3_id = new_operand_id;
        if (multiply_add.rd_id == old_operand_id)
          multiply_add.rd_id = new_operand_id;
      },
      [&](FloatUnary& unary) {
        if (unary.rs_id == old_operand_id)
          unary.rs_id = new_operand_id;
        if (unary.rd_id == old_operand_id)
          unary.rd_id = new_operand_id;
      },
      [&](Lui& lui) {
        if (lui.rd_id == old_operand_id)
          lui.rd_id = new_operand_id;
        if (lui.imm_id == old_operand_id)
          lui.imm_id = new_operand_id;
      },
      [&](Li& li) {
        if (li.rd_id == old_operand_id)
          li.rd_id = new_operand_id;
        if (li.imm_id == old_operand_id)
          li.imm_id = new_operand_id;
      },
      [&](Branch& branch) {
        if (branch.rs1_id == old_operand_id)
          branch.rs1_id = new_operand_id;
        if (branch.rs2_id == old_operand_id)
          branch.rs2_id = new_operand_id;
      },
      [&](Phi& phi) {
        if (phi.rd_id == old_operand_id)
          phi.rd_id = new_operand_id;
        for (auto& [id, _] : phi.incoming_list) {
          if (id == old_operand_id)
            id = new_operand_id;
        }
      },
      [&](auto& instruction) {}
    },
    this->kind
  );
}

void Instruction::replace_def_operand(
  OperandID old_operand_id,
  OperandID new_operand_id,
  Context& context
) {
  using namespace instruction;
  if (old_operand_id == new_operand_id)
    return;

  auto old_operand = context.get_operand(old_operand_id);
  auto new_operand = context.get_operand(new_operand_id);

  auto def_iter = std::find(
    this->def_id_list.begin(), this->def_id_list.end(), old_operand_id
  );
  if (def_iter != this->def_id_list.end()) {
    this->def_id_list.erase(def_iter);
    this->def_id_list.push_back(new_operand_id);
    old_operand->remove_def(this->id);
    new_operand->add_def(this->id);
  } else {
    return;
  }

  std::visit(
    overloaded{
      [&](Load& load) {
        if (load.rd_id == old_operand_id)
          load.rd_id = new_operand_id;
      },
      [&](PseudoLoad& pseudo_load) {
        if (pseudo_load.rd_id == old_operand_id)
          pseudo_load.rd_id = new_operand_id;
      },
      [&](Store& store) {},  // Do nothing
      [&](PseudoStore& pseudo_store) {
        if (pseudo_store.rt_id == old_operand_id)
          pseudo_store.rt_id = new_operand_id;
      },
      [&](Binary& binary) {
        if (binary.rd_id == old_operand_id)
          binary.rd_id = new_operand_id;
      },
      [&](BinaryImmediate& binary_immediate) {
        if (binary_immediate.rd_id == old_operand_id)
          binary_immediate.rd_id = new_operand_id;
      },
      [&](FloatLoad& load) {
        if (load.rd_id == old_operand_id)
          load.rd_id = new_operand_id;
      },
      [&](FloatPseudoLoad& pseudo_load) {
        if (pseudo_load.rd_id == old_operand_id)
          pseudo_load.rd_id = new_operand_id;
        if (pseudo_load.rt_id == old_operand_id)
          pseudo_load.rt_id = new_operand_id;
      },
      [&](FloatStore& store) {},  // Do nothing.
      [&](FloatPseudoStore& pseudo_store) {
        if (pseudo_store.rt_id == old_operand_id)
          pseudo_store.rt_id = new_operand_id;
      },
      [&](FloatMove& move) {
        if (move.rd_id == old_operand_id)
          move.rd_id = new_operand_id;
      },
      [&](FloatConvert& convert) {
        if (convert.rd_id == old_operand_id)
          convert.rd_id = new_operand_id;
      },
      [&](FloatBinary& binary) {
        if (binary.rd_id == old_operand_id)
          binary.rd_id = new_operand_id;
      },
      [&](FloatMultiplyAdd& multiply_add) {
        if (multiply_add.rd_id == old_operand_id)
          multiply_add.rd_id = new_operand_id;
      },
      [&](FloatUnary& unary) {
        if (unary.rd_id == old_operand_id)
          unary.rd_id = new_operand_id;
      },
      [&](Lui& lui) {
        if (lui.rd_id == old_operand_id)
          lui.rd_id = new_operand_id;
      },
      [&](Li& li) {
        if (li.rd_id == old_operand_id)
          li.rd_id = new_operand_id;
      },
      [&](Branch& branch) {},  // Do nothing.
      [&](Phi& phi) {
        if (phi.rd_id == old_operand_id)
          phi.rd_id = new_operand_id;
      },
      [&](auto& instruction) {}
    },
    this->kind
  );
}

void Instruction::replace_use_operand(
  OperandID old_operand_id,
  OperandID new_operand_id,
  Context& context
) {
  using namespace instruction;

  if (old_operand_id == new_operand_id)
    return;

  auto old_operand = context.get_operand(old_operand_id);
  auto new_operand = context.get_operand(new_operand_id);

  auto use_iter = std::find(
    this->use_id_list.begin(), this->use_id_list.end(), old_operand_id
  );
  if (use_iter != this->use_id_list.end()) {
    this->use_id_list.erase(use_iter);
    this->use_id_list.push_back(new_operand_id);
    old_operand->remove_use(this->id);
    new_operand->add_use(this->id);
  } else {
    return;
  }

  std::visit(
    overloaded{
      [&](Load& load) {
        if (load.rs_id == old_operand_id)
          load.rs_id = new_operand_id;
        if (load.imm_id == old_operand_id)
          load.imm_id = new_operand_id;
      },
      [&](PseudoLoad& pseudo_load) {
        if (pseudo_load.symbol_id == old_operand_id)
          pseudo_load.symbol_id = new_operand_id;
      },
      [&](Store& store) {
        if (store.rs1_id == old_operand_id)
          store.rs1_id = new_operand_id;
        if (store.rs2_id == old_operand_id)
          store.rs2_id = new_operand_id;
        if (store.imm_id == old_operand_id)
          store.imm_id = new_operand_id;
      },
      [&](PseudoStore& pseudo_store) {
        if (pseudo_store.rd_id == old_operand_id)
          pseudo_store.rd_id = new_operand_id;
        if (pseudo_store.rt_id == old_operand_id)
          pseudo_store.rt_id = new_operand_id;
        if (pseudo_store.symbol_id == old_operand_id)
          pseudo_store.symbol_id = new_operand_id;
      },
      [&](Binary& binary) {
        if (binary.rs1_id == old_operand_id)
          binary.rs1_id = new_operand_id;
        if (binary.rs2_id == old_operand_id)
          binary.rs2_id = new_operand_id;
      },
      [&](BinaryImmediate& binary_immediate) {
        if (binary_immediate.rs_id == old_operand_id)
          binary_immediate.rs_id = new_operand_id;
        if (binary_immediate.imm_id == old_operand_id)
          binary_immediate.imm_id = new_operand_id;
      },
      [&](FloatLoad& load) {
        if (load.rs_id == old_operand_id)
          load.rs_id = new_operand_id;
        if (load.imm_id == old_operand_id)
          load.imm_id = new_operand_id;
      },
      [&](FloatPseudoLoad& pseudo_load) {
        if (pseudo_load.symbol_id == old_operand_id)
          pseudo_load.symbol_id = new_operand_id;
        if (pseudo_load.rt_id == old_operand_id)
          pseudo_load.rt_id = new_operand_id;
      },
      [&](FloatStore& store) {
        if (store.rs1_id == old_operand_id)
          store.rs1_id = new_operand_id;
        if (store.rs2_id == old_operand_id)
          store.rs2_id = new_operand_id;
        if (store.imm_id == old_operand_id)
          store.imm_id = new_operand_id;
      },
      [&](FloatPseudoStore& pseudo_store) {
        if (pseudo_store.rd_id == old_operand_id)
          pseudo_store.rd_id = new_operand_id;
        if (pseudo_store.rt_id == old_operand_id)
          pseudo_store.rt_id = new_operand_id;
        if (pseudo_store.symbol_id == old_operand_id)
          pseudo_store.symbol_id = new_operand_id;
      },
      [&](FloatMove& move) {
        if (move.rs_id == old_operand_id)
          move.rs_id = new_operand_id;
      },
      [&](FloatConvert& convert) {
        if (convert.rs_id == old_operand_id)
          convert.rs_id = new_operand_id;
      },
      [&](FloatBinary& binary) {
        if (binary.rs1_id == old_operand_id)
          binary.rs1_id = new_operand_id;
        if (binary.rs2_id == old_operand_id)
          binary.rs2_id = new_operand_id;
      },
      [&](FloatMultiplyAdd& multiply_add) {
        if (multiply_add.rs1_id == old_operand_id)
          multiply_add.rs1_id = new_operand_id;
        if (multiply_add.rs2_id == old_operand_id)
          multiply_add.rs2_id = new_operand_id;
        if (multiply_add.rs3_id == old_operand_id)
          multiply_add.rs3_id = new_operand_id;
      },
      [&](FloatUnary& unary) {
        if (unary.rs_id == old_operand_id)
          unary.rs_id = new_operand_id;
      },
      [&](Lui& lui) {
        if (lui.imm_id == old_operand_id)
          lui.imm_id = new_operand_id;
      },
      [&](Li& li) {
        if (li.imm_id == old_operand_id)
          li.imm_id = new_operand_id;
      },
      [&](Branch& branch) {
        if (branch.rs1_id == old_operand_id)
          branch.rs1_id = new_operand_id;
        if (branch.rs2_id == old_operand_id)
          branch.rs2_id = new_operand_id;
      },
      [&](Phi& phi) {
        for (auto& [id, _] : phi.incoming_list) {
          if (id == old_operand_id)
            id = new_operand_id;
        }
      },
      [&](auto& instruction) {}
    },
    this->kind
  );
}

void Instruction::remove(Context& context) {
  for (auto def_id : this->def_id_list) {
    auto def = context.get_operand(def_id);
    def->remove_def(this->id);
  }

  for (auto use_id : this->use_id_list) {
    auto use = context.get_operand(use_id);
    use->remove_use(this->id);
  }

  if (auto prev = this->prev.lock()) {
    prev->next = this->next;
  }

  if (this->next) {
    this->next->prev = this->prev;
  }
}

void Instruction::raw_remove() {
  if (auto prev = this->prev.lock()) {
    prev->next = this->next;
  }
  if (this->next) {
    this->next->prev = this->prev;
  }
}

std::string Instruction::to_string(Context& context) {
  using namespace instruction;

  return std::visit(
    overloaded{
      [&](const Load& load) {
        std::stringstream ss;
        auto rd = context.get_operand(load.rd_id);
        auto rs = context.get_operand(load.rs_id);
        auto imm = context.get_operand(load.imm_id);
        switch (load.op) {
          case instruction::Load::LB:
            ss << "lb";
            break;
          case instruction::Load::LBU:
            ss << "lbu";
            break;
          case instruction::Load::LH:
            ss << "lh";
            break;
          case instruction::Load::LHU:
            ss << "lhu";
            break;
          case instruction::Load::LW:
            ss << "lw";
            break;
          case instruction::Load::LWU:
            ss << "lwu";
            break;
          case instruction::Load::LD:
            ss << "ld";
            break;
        }
        ss << " " << rd->to_string() << ", " << imm->to_string() << "("
           << rs->to_string() << ")";
        return ss.str();
      },
      [&](const PseudoLoad& pseudo_load) {
        std::stringstream ss;
        auto rd = context.get_operand(pseudo_load.rd_id);
        auto symbol = context.get_operand(pseudo_load.symbol_id);

        switch (pseudo_load.op) {
          case instruction::PseudoLoad::LA:
            ss << "la";
            break;
          case instruction::PseudoLoad::LW:
            ss << "lw";
            break;
        }

        ss << " " << rd->to_string() << ", " << symbol->to_string();

        return ss.str();
      },
      [&](const Store& store) {
        std::stringstream ss;
        auto rs1 = context.get_operand(store.rs1_id);
        auto rs2 = context.get_operand(store.rs2_id);
        auto imm = context.get_operand(store.imm_id);
        switch (store.op) {
          case instruction::Store::SB:
            ss << "sb";
            break;
          case instruction::Store::SH:
            ss << "sh";
            break;
          case instruction::Store::SW:
            ss << "sw";
            break;
          case instruction::Store::SD:
            ss << "sd";
            break;
            break;
        }
        ss << " " << rs2->to_string() << ", " << imm->to_string() << "("
           << rs1->to_string() << ")";
        return ss.str();
      },
      [&](const PseudoStore& pseudo_store) {
        std::stringstream ss;
        auto rd = context.get_operand(pseudo_store.rd_id);
        auto symbol = context.get_operand(pseudo_store.symbol_id);
        auto rt = context.get_operand(pseudo_store.rt_id);
        switch (pseudo_store.op) {
          case instruction::PseudoStore::SW:
            ss << "sw";
            break;
        }

        ss << " " << rd->to_string() << ", " << symbol->to_string() << ", "
           << rt->to_string();
        return ss.str();
      },
      [&](const Binary& binary) {
        std::stringstream ss;
        auto rd = context.get_operand(binary.rd_id);
        auto rs1 = context.get_operand(binary.rs1_id);
        auto rs2 = context.get_operand(binary.rs2_id);
        switch (binary.op) {
          case instruction::Binary::ADD:
            ss << "add";
            break;
          case instruction::Binary::ADDW:
            ss << "addw";
            break;
          case instruction::Binary::SUB:
            ss << "sub";
            break;
          case instruction::Binary::SUBW:
            ss << "subw";
            break;
          case instruction::Binary::AND:
            ss << "and";
            break;
          case instruction::Binary::OR:
            ss << "or";
            break;
          case instruction::Binary::XOR:
            ss << "xor";
            break;
          case instruction::Binary::SLL:
            ss << "sll";
            break;
          case instruction::Binary::SLLW:
            ss << "sllw";
            break;
          case instruction::Binary::SRL:
            ss << "srl";
            break;
          case instruction::Binary::SRLW:
            ss << "srlw";
            break;
          case instruction::Binary::SRA:
            ss << "sra";
            break;
          case instruction::Binary::SRAW:
            ss << "sraw";
            break;
          case instruction::Binary::SLT:
            ss << "slt";
            break;
          case instruction::Binary::SLTU:
            ss << "sltu";
            break;
          case instruction::Binary::MUL:
            ss << "mul";
            break;
          case instruction::Binary::MULW:
            ss << "mulw";
            break;
          case instruction::Binary::MULH:
            ss << "mulh";
            break;
          case instruction::Binary::MULHSU:
            ss << "mulhsu";
            break;
          case instruction::Binary::MULHU:
            ss << "mulhu";
            break;
          case instruction::Binary::DIV:
            ss << "div";
            break;
          case instruction::Binary::DIVU:
            ss << "divu";
            break;
          case instruction::Binary::DIVW:
            ss << "divw";
            break;
          case instruction::Binary::REM:
            ss << "rem";
            break;
          case instruction::Binary::REMU:
            ss << "remu";
            break;
          case instruction::Binary::REMW:
            ss << "remw";
            break;
          case instruction::Binary::REMUW:
            ss << "remuw";
            break;
        }
        ss << " " << rd->to_string() << ", " << rs1->to_string() << ", "
           << rs2->to_string();
        return ss.str();
      },
      [&](const BinaryImmediate& binaryimmdiate) {
        std::stringstream ss;

        auto rd = context.get_operand(binaryimmdiate.rd_id);
        auto rs = context.get_operand(binaryimmdiate.rs_id);
        auto imm = context.get_operand(binaryimmdiate.imm_id);
        switch (binaryimmdiate.op) {
          case instruction::BinaryImmediate::ADDI:
            ss << "addi";
            break;
          case instruction::BinaryImmediate::ADDIW:
            ss << "addiw";
            break;
          case instruction::BinaryImmediate::SLLI:
            ss << "slli";
            break;
          case instruction::BinaryImmediate::SLLIW:
            ss << "slliw";
            break;
          case instruction::BinaryImmediate::SRLI:
            ss << "srli";
            break;
          case instruction::BinaryImmediate::SRLIW:
            ss << "srliw";
            break;
          case instruction::BinaryImmediate::SRAI:
            ss << "srai";
            break;
          case instruction::BinaryImmediate::SRAIW:
            ss << "sraiw";
            break;
          case instruction::BinaryImmediate::ANDI:
            ss << "andi";
            break;
          case instruction::BinaryImmediate::ORI:
            ss << "ori";
            break;
          case instruction::BinaryImmediate::XORI:
            ss << "xori";
            break;
          case instruction::BinaryImmediate::SLTI:
            ss << "slti";
            break;
          case instruction::BinaryImmediate::SLTIU:
            ss << "sltiu";
            break;
        }
        ss << " " << rd->to_string() << ", " << rs->to_string() << ", "
           << imm->to_string(3);
        return ss.str();
      },
      [&](const FloatLoad& floatload) {
        std::stringstream ss;

        auto rd = context.get_operand(floatload.rd_id);
        auto rs = context.get_operand(floatload.rs_id);
        auto imm = context.get_operand(floatload.imm_id);
        switch (floatload.op) {
          case instruction::FloatLoad::FLW:
            ss << "flw";
            break;
          case instruction::FloatLoad::FLD:
            ss << "fld";
            break;
        }

        ss << " " << rd->to_string() << ", " << imm->to_string() << "("
           << rs->to_string() << ")";
        return ss.str();
      },
      [&context](const FloatPseudoLoad& instruction) {
        std::stringstream ss;

        auto rd = context.get_operand(instruction.rd_id);
        auto symbol = context.get_operand(instruction.symbol_id);
        auto rt = context.get_operand(instruction.rt_id);

        switch (instruction.op) {
          case FloatPseudoLoad::FLW:
            ss << "flw";
            break;
        }

        ss << " " << rd->to_string() << ", " << symbol->to_string() << ", "
           << rt->to_string();

        return ss.str();
      },
      [&](const FloatStore& floatstore) {
        std::stringstream ss;

        auto rs1 = context.get_operand(floatstore.rs1_id);
        auto rs2 = context.get_operand(floatstore.rs2_id);
        auto imm = context.get_operand(floatstore.imm_id);
        switch (floatstore.op) {
          case instruction::FloatStore::FSW:
            ss << "fsw";
            break;
          case instruction::FloatStore::FSD:
            ss << "fsd";
            break;
        }
        ss << " " << rs2->to_string() << ", " << imm->to_string() << "("
           << rs1->to_string() << ")";
        return ss.str();
      },
      [&context](const FloatPseudoStore& instruction) {
        std::stringstream ss;

        auto rd = context.get_operand(instruction.rd_id);
        auto symbol = context.get_operand(instruction.symbol_id);
        auto rt = context.get_operand(instruction.rt_id);

        switch (instruction.op) {
          case FloatPseudoStore::FSW:
            ss << "fsw";
            break;
        }

        ss << " " << rd->to_string() << ", " << symbol->to_string() << ", "
           << rt->to_string();

        return ss.str();
      },
      [&](const FloatMove& floatmove) {
        std::stringstream ss;

        auto rd = context.get_operand(floatmove.rd_id);
        auto rs = context.get_operand(floatmove.rs_id);

        ss << "fmv";

        switch (floatmove.dest_mode) {
          case instruction::FloatMove::H:
            ss << ".h";
            break;
          case instruction::FloatMove::S:
            ss << ".w";
            break;
          case instruction::FloatMove::D:
            ss << ".d";
            break;
          case instruction::FloatMove::X:
            ss << ".x";
            break;
        }

        switch (floatmove.source_mode) {
          case instruction::FloatMove::H:
            ss << ".h";
            break;
          case instruction::FloatMove::S:
            ss << ".w";
            break;
          case instruction::FloatMove::D:
            ss << ".d";
            break;
          case instruction::FloatMove::X:
            ss << ".x";
            break;
        }

        ss << " " << rd->to_string() << ", " << rs->to_string();
        return ss.str();
      },
      [&](const FloatConvert& floatconvert) {
        std::stringstream ss;

        auto rd = context.get_operand(floatconvert.rd_id);
        auto rs = context.get_operand(floatconvert.rs_id);

        ss << "fcvt";

        switch (floatconvert.dest_mode) {
          case instruction::FloatConvert::H:
            ss << ".h";
            break;
          case instruction::FloatConvert::S:
            ss << ".s";
            break;
          case instruction::FloatConvert::W:
            ss << ".w";
            break;
          case instruction::FloatConvert::D:
            ss << ".d";
            break;
          case instruction::FloatConvert::WU:
            ss << ".wu";
            break;
          case instruction::FloatConvert::L:
            ss << ".l";
            break;
          case instruction::FloatConvert::LU:
            ss << ".lu";
            break;
        }

        switch (floatconvert.source_mode) {
          case instruction::FloatConvert::H:
            ss << ".h";
            break;
          case instruction::FloatConvert::S:
            ss << ".s";
            break;
          case instruction::FloatConvert::W:
            ss << ".w";
            break;
          case instruction::FloatConvert::D:
            ss << ".d";
            break;
          case instruction::FloatConvert::WU:
            ss << ".wu";
            break;
          case instruction::FloatConvert::L:
            ss << ".l";
            break;
          case instruction::FloatConvert::LU:
            ss << ".lu";
            break;
        }

        ss << " " << rd->to_string() << ", " << rs->to_string();

        if (floatconvert.dest_mode == instruction::FloatConvert::WU ||
            floatconvert.dest_mode == instruction::FloatConvert::W ||
            floatconvert.dest_mode == instruction::FloatConvert::LU ||
            floatconvert.dest_mode == instruction::FloatConvert::L) {
          ss << ", rtz";
        }
        // 舍入方式似乎只有float to int？

        return ss.str();
      },
      [&](const FloatBinary& floatbinary) {
        std::stringstream ss;

        auto rd = context.get_operand(floatbinary.rd_id);
        auto rs1 = context.get_operand(floatbinary.rs1_id);
        auto rs2 = context.get_operand(floatbinary.rs2_id);
        switch (floatbinary.op) {
          case instruction::FloatBinary::FADD:
            ss << "fadd";
            break;
          case instruction::FloatBinary::FSUB:
            ss << "fsub";
            break;
          case instruction::FloatBinary::FMUL:
            ss << "fmul";
            break;
          case instruction::FloatBinary::FDIV:
            ss << "fdiv";
            break;
          case instruction::FloatBinary::FMIN:
            ss << "fmin";
            break;
          case instruction::FloatBinary::FMAX:
            ss << "fmax";
            break;
          case instruction::FloatBinary::FEQ:
            ss << "feq";
            break;
          case instruction::FloatBinary::FLT:
            ss << "flt";
            break;
          case instruction::FloatBinary::FLE:
            ss << "fle";
            break;
          case instruction::FloatBinary::FSGNJ:
            ss << "fsgnj";
            break;
          case instruction::FloatBinary::FSGNJN:
            ss << "fsgnjn";
            break;
          case instruction::FloatBinary::FSGNJX:
            ss << "fsgnjx";
            break;
        }

        switch (floatbinary.mode) {
          case instruction::FloatBinary::SINGLE:
            ss << ".s";
            break;
          case instruction::FloatBinary::DOUBLE:
            ss << ".d";
            break;
        }

        ss << " " << rd->to_string() << ", " << rs1->to_string() << ", "
           << rs2->to_string();

        return ss.str();
      },
      [&](const FloatMultiplyAdd& floatmultiplyadd) {
        std::stringstream ss;

        auto rd = context.get_operand(floatmultiplyadd.rd_id);
        auto rs1 = context.get_operand(floatmultiplyadd.rs1_id);
        auto rs2 = context.get_operand(floatmultiplyadd.rs2_id);
        auto rs3 = context.get_operand(floatmultiplyadd.rs3_id);

        switch (floatmultiplyadd.op) {
          case instruction::FloatMultiplyAdd::FMADD:
            ss << "fmadd";
            break;
          case instruction::FloatMultiplyAdd::FMSUB:
            ss << "fmsub";
            break;
          case instruction::FloatMultiplyAdd::FNMADD:
            ss << "fnmadd";
            break;
          case instruction::FloatMultiplyAdd::FNMSUB:
            ss << "fnmsub";
            break;
        }

        switch (floatmultiplyadd.mode) {
          case instruction::FloatMultiplyAdd::SINGLE:
            ss << ".s";
            break;
          case instruction::FloatMultiplyAdd::DOUBLE:
            ss << ".d";
            break;
        }

        ss << " " << rd->to_string() << ", " << rs1->to_string() << ", "
           << rs2->to_string() << ", " << rs3->to_string();

        return ss.str();
      },
      [&](const FloatUnary& floatunary) {
        std::stringstream ss;

        auto rd = context.get_operand(floatunary.rd_id);
        auto rs1 = context.get_operand(floatunary.rs_id);

        switch (floatunary.op) {
          case instruction::FloatUnary::FCLASS:
            ss << "fclass";
            break;
          case instruction::FloatUnary::FSQRT:
            ss << "fsqrt";
            break;
        }

        switch (floatunary.mode) {
          case instruction::FloatUnary::SINGLE:
            ss << ".s";
            break;
          case instruction::FloatUnary::DOUBLE:
            ss << ".d";
            break;
        }

        ss << " " << rd->to_string() << ", " << rs1->to_string();

        return ss.str();
      },
      [&](const Lui& lui) {
        std::stringstream ss;

        auto rd = context.get_operand(lui.rd_id);
        auto imm = context.get_operand(lui.imm_id);
        ss << "lui " << rd->to_string() << ", " << imm->to_string(5);
        return ss.str();
      },
      [&](const Li& li) {
        std::stringstream ss;
        auto rd = context.get_operand(li.rd_id);
        auto imm = context.get_operand(li.imm_id);
        ss << "li " << rd->to_string() << ", " << imm->to_string();
        return ss.str();
      },
      [](const Ret& ret) -> std::string { return "ret"; },
      [](const Call& call) { return "call " + call.function_name; },
      [&](const Branch& branch) {
        std::stringstream ss;

        auto rs1 = context.get_operand(branch.rs1_id);
        auto rs2 = context.get_operand(branch.rs2_id);
        switch (branch.op) {
          case instruction::Branch::BEQ:
            ss << "beq";
            break;
          case instruction::Branch::BNE:
            ss << "bne";
            break;
          case instruction::Branch::BLT:
            ss << "blt";
            break;
          case instruction::Branch::BGE:
            ss << "bge";
            break;
          case instruction::Branch::BLTU:
            ss << "bltu";
            break;
          case instruction::Branch::BGEU:
            ss << "bgeu";
            break;
        }
        ss << " " << rs1->to_string() << ", " << rs2->to_string() << ", "
           << context.get_basic_block(branch.block_id)->get_label();

        return ss.str();
      },
      [&](const Jump& jump) {
        return "j " + context.get_basic_block(jump.block_id)->get_label();
      },
      [&](const Phi& phi) {
        std::stringstream ss;

        auto rd = context.get_operand(phi.rd_id);

        ss << "# phi " << rd->to_string() << " : ";

        for (auto& [operand_id, block_id] : phi.incoming_list) {
          ss << "[" << context.get_operand(operand_id)->to_string() << ", "
             << context.get_basic_block(block_id)->get_label() << "] ";
        }

        return ss.str();
      },
      [](const auto& instruction) -> std::string {
        return "Unknown Instruction";
      }
    },
    kind
  );
};

bool Instruction::is_phi() const {
  return std::holds_alternative<instruction::Phi>(this->kind);
}

bool Instruction::is_branch_or_jmp() const {
  return std::holds_alternative<instruction::Jump>(this->kind) ||
         std::holds_alternative<instruction::Branch>(this->kind);
}

bool Instruction::is_load() const {
  return std::holds_alternative<instruction::Load>(this->kind);
}
bool Instruction::is_store() const {
  return std::holds_alternative<instruction::Store>(this->kind);
}
bool Instruction::is_binary() const {
  return std::holds_alternative<instruction::Binary>(this->kind);
}

bool Instruction::is_binary_imm() const {
  return std::holds_alternative<instruction::BinaryImmediate>(this->kind);
}

bool Instruction::is_float_load() const {
  return std::holds_alternative<instruction::FloatLoad>(this->kind);
}

bool Instruction::is_float_store() const {
  return std::holds_alternative<instruction::FloatStore>(this->kind);
}

bool Instruction::is_li() const {
  return std::holds_alternative<instruction::Li>(this->kind);
}

bool Instruction::is_lui() const {
  return std::holds_alternative<instruction::Lui>(this->kind);
}
}  // namespace backend
}  // namespace sed