#ifndef SED_IR_CODEGEN_H_
#define SED_IR_CODEGEN_H_

#include "backend/builder.h"
#include "backend/context.h"
#include "backend/utils.h"
#include "index.h"
#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace sed
{

using IrOperandID = ir::OperandID;
using IrBasicBlockID = ir::BasicBlockID;
using IrContext = ir::Context;
using IrInstructionPtr = ir::InstructionPtr;
using IrBasicBlockPtr = ir::BasicBlockPtr;
using IrFunctionPtr = ir::FunctionPtr;

using AsmOperandID = backend::OperandID;
using AsmOperandPtr = backend::OperandPtr;
using AsmBuilder = backend::Builder;
using AsmContext = backend::Context;
using AsmBasicBlockID = backend::BasicBlockID;
using AsmInstructionPtr = backend::InstructionPtr;
using AsmBasicBlockPtr = backend::BasicBlockPtr;
using AsmFunctionPtr = backend::FunctionPtr;

struct CodegenContext
{
    std::map<IrOperandID, AsmOperandID> operand_map;
    std::map<IrBasicBlockID, AsmBasicBlockID> basic_block_map;

    std::vector<std::tuple<IrInstructionPtr, AsmInstructionPtr>> phi_list;

    CodegenContext() = default;

    inline AsmOperandID get_asm_operand_id(IrOperandID ir_operand_id);
};

void codegen(IrContext &ir_context, AsmBuilder &builder, CodegenContext &codegen_context);

void codegen_function(IrFunctionPtr ir_function, IrContext &ir_context, AsmBuilder &builder,
                      CodegenContext &codegen_context);

void codegen_rest(AsmBuilder &builder);

void codegen_function_prolouge(AsmBuilder &builder);

void codegen_function_epilouge(AsmBuilder &builder);

void codegen_basic_block(IrBasicBlockPtr ir_basic_block, IrContext &ir_context, AsmBuilder &builder,
                         CodegenContext &codegen_context);

void codegen_instruction(IrInstructionPtr ir_inst, IrContext &ir_context, AsmBuilder &builder,
                         CodegenContext &codegen_context);

void codegen_phi_incoming_list(IrInstructionPtr ir_inst, AsmInstructionPtr asm_inst, IrContext &ir_context,
                               AsmBuilder &builder, CodegenContext &codegen_context);

// try_keep_imm: keep imm without mv to a reg if check_itype and int.
// fmv_imm: mv imm to a float register when it's a float value.
AsmOperandID codegen_operand(IrOperandID ir_operand_id, IrContext &ir_context, AsmBuilder &builder,
                             CodegenContext &codegen_context, bool try_keep_imm = false, bool fmv_imm = false,
                             bool in_phi = false);

inline AsmOperandID codegen_vreg(IrOperandID ir_local_id, IrContext &ir_context, AsmBuilder &builder,
                                 CodegenContext &codegen_context);

std::tuple<int, AsmOperandID> codegen_ptr2reg(AsmOperandID asm_ptr_id, AsmBuilder &builder, bool only_reg = false);
/*
/// Generate corresponding asm operan for ir operand.
/// try_keep_imm: if the immediate is an i-type immediate, keep it as
immediate,
///   otherwise load it into a register.
/// use_fmv: fmv floating-point bits from general register to a float
register.
/// phi: used in phi instruction.
AsmOperandID codegen_operand(
  IrOperandID ir_operand,
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context,
  bool try_keep_imm = false,
  bool use_fmv = false,
  bool phi = false
);
*/

/// Perform register allocation.
void asm_register_allocation(AsmBuilder &builder);

/// Perform instruction scheduling.
void asm_instruction_scheduling(AsmContext &context);

} // namespace sed

#endif