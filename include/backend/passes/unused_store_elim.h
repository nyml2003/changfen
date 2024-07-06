#ifndef SED_BACKEND_OPTIMIZE_UNUSED_STORE_ELIM_H
#define SED_BACKEND_OPTIMIZE_UNUSED_STORE_ELIM_H

#include "backend/builder.h"
#include "index.h"

namespace sed
{
namespace backend
{

void unused_store_elim(Builder &builder);

void unused_store_elim_function(FunctionPtr function, Builder &builder);

} // namespace backend
} // namespace sed

#endif
