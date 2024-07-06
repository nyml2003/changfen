#include "utils.h"

namespace sed::backend {

bool check_itype_immediate(int32_t value) {
  return (value >= -0x800 && value <= 0x7ff);
}

bool check_utype_immediate(uint32_t value) {
  return ((value & 0xfff) == 0);
}

}  // namespace sed::backend
