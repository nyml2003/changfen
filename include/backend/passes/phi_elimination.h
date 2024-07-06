#ifndef SED_BACKEND_OPTIMIZE_PHI_ELIMINATION_H
#define SED_BACKEND_OPTIMIZE_PHI_ELIMINATION_H

#include "backend/builder.h"
#include "index.h"

namespace sed
{
namespace backend
{

void phi_elimination(Builder &builder);

void phi_elimination_function(FunctionPtr now_function, Builder &builder);

} // namespace backend
} // namespace sed

#endif