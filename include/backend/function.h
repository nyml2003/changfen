#ifndef SED_BACKEND_FUNCTION_H_
#define SED_BACKEND_FUNCTION_H_

#include "backend/register.h"
#include "index.h"
#include <cstddef>

namespace sed
{
namespace backend
{

class Function
{
  public:
    BasicBlockPtr head_basic_block;
    BasicBlockPtr tail_basic_block;
    std::string name;

    std::size_t stack_frame_size;
    std::size_t align_frame_size;

    std::set<Register> saved_register_set;

    Function(std::string name);

    void add_basic_block(BasicBlockPtr basic_block);

    void add_saved_register(Register reg);

    std::string to_string(Context &context);
};

} // namespace backend
} // namespace sed

#endif