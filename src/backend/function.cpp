#include "backend/function.h"
#include "backend/basic_block.h"
#include "backend/context.h"
#include "index.h"

namespace sed
{
namespace backend
{

Function::Function(std::string name) : name(name), stack_frame_size(0)
{
    this->head_basic_block = create_dummy_basic_block();
    this->tail_basic_block = create_dummy_basic_block();

    this->head_basic_block->insert_next(tail_basic_block);
}

void Function::add_basic_block(BasicBlockPtr basic_block)
{
    this->tail_basic_block->insert_prev(basic_block);
}

// Function" has no member "add_saved_register ??? THIS shouldn't happen don't
// know why ,because it has been delcared in function.h
void Function::add_saved_register(Register reg)
{
    if (!REG_CALLEE_SAVED.count(reg))
    {
        return;
    }
    this->saved_register_set.insert(reg);
}

std::string Function::to_string(Context &context)
{
    std::string result = "\t.globl " + this->name + "\n";
    // don't know if this is useful in sed,or if it is left out,when you merge,you
    // will know if it is
    result += "\t.align 1\n";
    result += "\t.type " + this->name + ", @function\n";

    result += this->name + ":\n";

    auto now_basic_block = this->head_basic_block->next;

    while (now_basic_block != this->tail_basic_block)
    {
        result += now_basic_block->to_string(context);
        now_basic_block = now_basic_block->next;
    }

    return result;
}

} // namespace backend
} // namespace sed