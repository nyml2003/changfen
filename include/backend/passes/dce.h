#ifndef SED_BACKEND_OPTIMIZE_DCE_H
#define SED_BACKEND_OPTIMIZE_DCE_H

#include "backend/builder.h"
#include "index.h"

namespace sed
{
namespace backend
{

void dce(Builder &builder);

} // namespace backend
} // namespace sed

#endif