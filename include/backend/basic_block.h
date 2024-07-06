#ifndef SED_BACKEND_BASICBLOCK_H
#define SED_BACKEND_BASICBLOCK_H

#include "index.h"

namespace sed
{
namespace backend
{

// TODO: 基本块应当和function，instruction结合，这里只写了function.
class BasicBlock : public std::enable_shared_from_this<BasicBlock>
{
  public:
    InstructionPtr head_instruction;
    InstructionPtr tail_instruction;

    std::vector<BasicBlockID> successor_list;
    std::vector<BasicBlockID> predecessor_list;

    BasicBlockID id;
    std::string parent_function_name;

    BasicBlockPtr next;
    BasicBlockPrevPtr prev;

    BasicBlock(BasicBlockID id, std::string parent_function_name);

    void add_instruction_front(InstructionPtr instruction);
    void add_instruction_back(InstructionPtr instruction);

    void insert_next(BasicBlockPtr basic_block);
    void insert_prev(BasicBlockPtr basic_block);

    std::string get_label();

    void add_successor(BasicBlockID successor_id);
    void add_predecessor(BasicBlockID predecessor_id);
    void remove_successor(BasicBlockID successor_id);
    void remove_predecessor(BasicBlockID predecessor_id);

    std::string to_string(Context &context);
};

BasicBlockPtr create_basic_block(BasicBlockID id, std::string parent_function_name);

BasicBlockPtr create_dummy_basic_block();

}; // namespace backend
}; // namespace sed

#endif