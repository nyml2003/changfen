#ifndef SED_IR_CONTEXT_H
#define SED_IR_CONTEXT_H

#include "index.h"

namespace sed::ir
{

struct Context
{
    std::map<OperandID, OperandPtr> operand_table;
    std::map<InstructionID, InstructionPtr> inst_table;
    std::map<BasicBlockID, BasicBlockPtr> block_table;

    std::map<std::string, FunctionPtr> function_table;
    std::set<OperandID> global_list;

    OperandID next_operand_id = 0;
    InstructionID next_instruction_id = 0;
    BasicBlockID next_block_id = 0;

    OperandID get_next_operand_id();
    InstructionID get_next_instruction_id();
    BasicBlockID get_next_block_id();

    void add_operand(OperandPtr operand);
    void add_block(BasicBlockPtr block);
    void add_instruction(InstructionPtr inst);
    void add_function(FunctionPtr function);

    OperandPtr get_operand(OperandID id);
    InstructionPtr get_instruction(InstructionID id);
    BasicBlockPtr get_basic_block(BasicBlockID id);
    FunctionPtr get_function(std::string name);
    std::string to_string();
};

} // namespace sed::ir

#endif // SED_IR_CONTEXT_H