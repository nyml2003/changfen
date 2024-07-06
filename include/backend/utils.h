#ifndef SED_BACKEND_UTILS
#define SED_BACKEND_UTILS

#include "index.h"
namespace sed::backend
{
bool check_itype_immediate(int32_t value);

bool check_utype_immediate(uint32_t value);
} // namespace sed::backend

#endif // SED_BACKEND_UTILS
