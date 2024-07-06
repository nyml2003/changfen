#include "backend/global.h"

namespace sed {
namespace backend {

std::size_t Global::get_size() const {
  // 初始化的array.
  return std::visit(
    overloaded{
      [](const std::vector<uint32_t>& v) -> size_t { return 4 * v.size(); },
      [](uint64_t v) -> size_t { return v; },
    },
    value
  );
}

std::string Global::to_string() const {
  std::string result;

  std::visit(
    overloaded{
      [&result, this](const std::vector<uint32_t>& value) {
        result += "\t.data\n";
        result += "\t.globl " + name + "\n";
        //?? newly added,咱是没align吗，不懂
        // result += "\t.align 2\n";

        result += name + ":\n";
        for (auto v : value) {
          result += "\t.word " + std::to_string(v) + "\n";
        }
      },
      [&result, this](const uint64_t& value) {
        result += "\t.bss\n";
        result += "\t.globl " + name + "\n";
        //?? newly added
        // result += "\t.align 2\n";
        result += name + ":\n";
        result += "\t.zero " + std::to_string(value) + "\n";
      },
    },
    value
  );
  result += "\t.size " + name + ", " + std::to_string(get_size()) + "\n";

  return result;
}

}  // namespace backend

}  // namespace sed