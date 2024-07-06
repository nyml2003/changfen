#ifndef SED_BACKEND_GLOBAL_H_
#define SED_BACKEND_GLOBAL_H_

#include "index.h"

namespace sed
{
namespace backend
{

class Global
{
  public:
    /// The value of the global variable.
    /// list of data in words or the size of zero
    std::string name;
    GlobalValue value;

    Global(std::string name, GlobalValue value) : name(name), value(value){};

    std::size_t get_size() const;

    std::string to_string() const;
};

} // namespace backend

} // namespace sed

#endif // GLOBAL_H
