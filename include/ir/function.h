#ifndef SED_IR_FUNCTION_H
#define SED_IR_FUNCTION_H

#include "index.h"

namespace sed::ir
{
struct Function
{
    std::string name;
    TypePtr return_type;
    std::vector<OperandID> param_id_list;
    BasicBlockPtr head_block;
    BasicBlockPtr tail_block;
    std::set<InstructionID> caller_id_list;

    bool is_declare;

    std::optional<OperandID> maybe_return_id;
    std::optional<BasicBlockPtr> maybe_return_block;

    Function(std::string name, TypePtr return_type, std::vector<OperandID> param_id_list, bool is_declare);

    void append_basic_block(BasicBlockPtr block);
    void prepend_basic_block(BasicBlockPtr block);

    void add_terminator(Builder &builder);
    void remove_caller(InstructionID caller_id);
    void remove_unused_block(Context &context);

    std::string to_string(Context &context) const;
};
} // namespace sed::ir

#endif // SED_IR_FUNCTION_H