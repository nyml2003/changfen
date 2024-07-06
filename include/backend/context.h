#ifndef SED_BACKEND_CONTEXT_H_
#define SED_BACKEND_CONTEXT_H_

#include "backend/global.h"
#include "index.h"

namespace sed
{
namespace backend
{

class Context
{
    OperandID now_operand_id = 0;
    InstructionID now_instruction_id = 0;
    BasicBlockID now_basic_block_id = 0;
    VirtualRegisterID now_virtual_register_id = 0;

  public:
    std::map<OperandID, OperandPtr> operand_table;
    std::map<InstructionID, InstructionPtr> instruction_table;
    std::map<BasicBlockID, BasicBlockPtr> basic_block_table;
    std::map<std::string, FunctionPtr> function_table;

    std::vector<OperandID> global_list;

    Context() = default;
    // 于是我们就知道必须1开始了.
    OperandID create_operand_id()
    {
        return ++now_operand_id;
    };
    InstructionID create_instruction_id()
    {
        return ++now_instruction_id;
    };
    BasicBlockID create_basic_block_id()
    {
        return ++now_basic_block_id;
    };
    VirtualRegisterID create_virtual_register_id()
    {
        return ++now_virtual_register_id;
    };

    OperandPtr get_operand(OperandID id);
    InstructionPtr get_instruction(InstructionID id);
    BasicBlockPtr get_basic_block(BasicBlockID id);
    FunctionPtr get_function(std::string name);

    void register_operand(OperandPtr operand);
    void register_instruction(InstructionPtr instruction);
    void register_basic_block(BasicBlockPtr basicblock);
    void register_function(FunctionPtr function);

    std::string to_string();
};

} // namespace backend
} // namespace sed

#endif