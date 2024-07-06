#include "backend/immediate.h"

namespace sed {
namespace backend {

std::string Immediate::to_string(int width) const {
  return std::visit(
    overloaded{
      [](const int32_t& imm) { return std::to_string(imm); },
      [](const int64_t& imm) { return std::to_string(imm); },
      [&](const uint32_t& imm) {
        std::stringstream ss;
        width = width == 0 ? 8 : width;
        ss << "0x" << std::setfill('0') << std::setw(width) << std::hex << imm;
        return ss.str();
      },
      [&](const uint64_t& imm) {
        std::stringstream ss;
        width = width == 0 ? 8 : width;
        ss << "0x" << std::setfill('0') << std::setw(width) << std::hex << imm;
        return ss.str();
      },
    },
    value
  );
}

bool Immediate::is_zero() const {
  return std::visit(
    overloaded{
      [](int32_t v) { return v == 0; },
      [](int64_t v) { return v == 0; },
      [](uint32_t v) { return v == 0; },
      [](uint64_t v) { return v == 0; },
    },
    value
  );
}

__int128_t Immediate::get_value() const {
  return std::visit(
    overloaded{
      [](int32_t v) { return (__int128_t)v; },
      [](int64_t v) { return (__int128_t)v; },
      [](uint32_t v) { return (__int128_t)v; },
      [](uint64_t v) { return (__int128_t)v; },
    },
    value
  );
}

}  // namespace backend
}  // namespace sed