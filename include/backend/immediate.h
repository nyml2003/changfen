#ifndef SED_BACKEND_IMMEDIATE_H_
#define SED_BACKEND_IMMEDIATE_H_

#include "index.h"

namespace sed
{
namespace backend
{

/// Immediate in the assembly code.
/// Value check is not performed here. The value shall be converted into the
/// right form in the construction of the assembly code.
class Immediate
{
  public:
    ImmediateValue value;
    Immediate(ImmediateValue value) : value(value)
    {
    }
    std::string to_string(int width = 0) const;

    bool is_zero() const;

    __int128_t get_value() const;
};

} // namespace backend

} // namespace sed

#endif