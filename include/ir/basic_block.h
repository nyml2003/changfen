#ifndef SED_IR_BASIC_BLOCK_H
#define SED_IR_BASIC_BLOCK_H

#include "index.h"

namespace sed::ir
{
struct BasicBlock : std::enable_shared_from_this<BasicBlock>
{
    BasicBlockID id;
    std::string parent_function_name;
    InstructionPtr head_inst;
    InstructionPtr tail_inst;

    std::set<InstructionID> use_id_list;
    std::set<BasicBlockID> pred_list;
    std::set<BasicBlockID> succ_list;

    BasicBlockPtr next;
    BasicBlockPrevPtr prev;

    BasicBlock(BasicBlockID id, std::string parent_function_name);

    void append_instruction(InstructionPtr inst);
    void prepend_instruction(InstructionPtr inst);

    void add_use(InstructionID use_id);
    void remove_use(InstructionID use_id);

    void split(InstructionPtr inst, Builder &builder);

    void add_pred(BasicBlockID pred_id);
    void add_succ(BasicBlockID succ_id);
    void remove_pred(BasicBlockID pred_id);
    void remove_succ(BasicBlockID succ_id);

    void insert_next(BasicBlockPtr block);
    void insert_prev(BasicBlockPtr block);
    void remove(Context &context);

    bool has_terminator() const;
    bool has_use() const;

    std::set<BasicBlockID> get_succ() const;

    std::string get_label() const;
    std::string to_string(Context &context) const;
};

BasicBlockPtr create_basic_block(BasicBlockID id, std::string parent_function_name);
BasicBlockPtr create_dummy_basic_block();

} // namespace sed::ir

#endif // SED_IR_BASIC_BLOCK_H