#include "ir/type.h"

namespace sed::ir {
Type::Type(TypeKind kind) : kind(kind) {}

std::string Type::to_string() const {
  using namespace type;
  std::string s;
  std::visit(
    overloaded{
      [&s](const Void& a) { s += "void"; },
      [&s](const Int& a) { s += "i" + std::to_string(a.size); },
      [&s](const Float& a) { s += "float"; },
      [&s](const Array& a) {
        s += "[" + std::to_string(a.len) + " x " + a.type->to_string() + "]";
      },
      [&s](const Pointer& a) { s += "ptr"; }
    },
    kind
  );
  return s;
}

size_t Type::size() const {
  using namespace type;
  return std::visit(
    overloaded{
      [](const Void& a) -> size_t { return 0; },
      [](const Int& a) -> size_t { return a.size; },
      [](const Float& a) -> size_t { return 32; },
      [](const Array& a) -> size_t { return a.len * a.type->size(); },
      [](const Pointer& a) -> size_t { return 64; }
    },
    kind
  );
}

bool Type::operator==(Type r) const {
  using namespace type;
  return std::visit(
    overloaded{
      [](const Void& l, const Void& r) { return true; },
      [](const Int& l, const Int& r) { return l.size == r.size; },
      [](const Float& l, const Float& r) { return true; },
      [](const Array& l, const Array& r) {
        return l.len == r.len && *l.type == *r.type;
      },
      [](const Pointer& l, const Pointer& r) { return true; },
      [](const auto& l, const auto& r) { return false; }
    },
    kind, r.kind
  );
}

bool Type::operator!=(Type r) const {
  return !(*this == r);
}

size_t size(TypePtr type) {
  return type->size();
}

bool operator==(TypePtr l, TypePtr r) {
  return *l == *r;
}
bool operator!=(TypePtr l, TypePtr r) {
  return *l != *r;
}

}  // namespace sed::ir
