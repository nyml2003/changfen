#include "backend/context.h"
#include "backend/basic_block.h"
#include "backend/function.h"
#include "backend/global.h"
#include "backend/instruction.h"
#include "backend/operand.h"
#include "index.h"

namespace sed
{
namespace backend
{

// Context Get methods
OperandPtr Context::get_operand(OperandID id)
{
    if (operand_table.count(id))
    {
        return operand_table.at(id);
    }
    else
    {
        throw std::runtime_error("Context::get_operand:Operand not found");
    }
}

InstructionPtr Context::get_instruction(InstructionID id)
{
    if (instruction_table.count(id))
    {
        return instruction_table.at(id);
    }
    else
    {
        throw std::runtime_error("Context::get_instruction: Instruction not found");
    }
}

BasicBlockPtr Context::get_basic_block(BasicBlockID id)
{
    if (basic_block_table.count(id))
    {
        return basic_block_table.at(id);
    }
    else
    {
        throw std::runtime_error("Context::get_basicblock: BasicBlock not found");
    }
}

FunctionPtr Context::get_function(std::string name)
{
    if (function_table.count(name))
    {
        return function_table.at(name);
    }
    else
    {
        throw std::runtime_error("Context::get_function: Function not found");
    }
}

// Context insert methods.
void Context::register_operand(OperandPtr operand)
{
    if (operand_table.count(operand->id))
    {
        throw std::runtime_error("Context::insert_operand: Operand already exists");
    }
    operand_table[operand->id] = operand;

    if (std::holds_alternative<Global>(operand->kind) && operand->modifier == Modifier::NONE)
    {
        global_list.push_back(operand->id);
    }
}

void Context::register_instruction(InstructionPtr instruction)
{
    if (instruction_table.count(instruction->id))
    {
        throw std::runtime_error("Context::insert_instruction: Instruction already exists");
    }
    instruction_table[instruction->id] = instruction;
}

void Context::register_basic_block(BasicBlockPtr basicblock)
{
    if (basic_block_table.count(basicblock->id))
    {
        throw std::runtime_error("Context::insert_basicblock: BasicBlock already exists");
    }
    basic_block_table[basicblock->id] = basicblock;
}

void Context::register_function(FunctionPtr function)
{
    if (function_table.count(function->name))
    {
        throw std::runtime_error("Context::insert_function: Function already exists");
    }
    function_table[function->name] = function;
}

std::string Context::to_string()
{
    std::string result = "\t.option pic\n";
    result += "\t.text\n"
              "\t.globl  __builtin_fill_zero\n"
              "\t.align  1\n"
              "\t.type   __builtin_fill_zero, @function\n"
              "__builtin_fill_zero:\n"
              "\taddiw   a5,a1,-1\n"
              "\tslli    a5,a5,32\n"
              "\tsrli    a5,a5,30\n"
              "\taddi    a4,a0,4\n"
              "\tadd     a5,a5,a4\n"
              ".__builtin_label_0:\n"
              "\tsw      zero,0(a0)\n"
              "\taddi    a0,a0,4\n"
              "\tbne     a0,a5,.__builtin_label_0\n"
              "\tret\n";

    for (auto &[_, function] : this->function_table)
    {
        result += function->to_string(*this);
    }

    for (auto operand_id : this->global_list)
    {
        auto &global = std::get<Global>(this->operand_table.at(operand_id)->kind);
        result += global.to_string() + "\n";
    }

    return result;
}

} // namespace backend
} // namespace sed