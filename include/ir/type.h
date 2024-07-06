#ifndef SED_IR_TYPE_H
#define SED_IR_TYPE_H

#include "index.h"

namespace sed::ir
{
namespace type
{
struct Void
{
};
struct Int
{
    int size;
};
struct Float
{
};
struct Array
{
    size_t len;
    TypePtr type;
};
struct Pointer
{
};

} // namespace type

struct Type
{
    TypeKind kind;

    Type(TypeKind kind);
    size_t size() const;

    template <typename T> inline bool is()
    {
        return std::holds_alternative<T>(kind);
    }

    template <typename T> std::optional<T> as()
    {
        if (std::holds_alternative<T>(kind))
        {
            return std::get<T>(kind);
        }
        else
        {
            return std::nullopt;
        }
    }

    bool operator==(const Type r) const;
    bool operator!=(const Type r) const;

    std::string to_string() const;
};

size_t size(TypePtr type);

bool operator==(TypePtr l, TypePtr r);
bool operator!=(TypePtr l, TypePtr r);

} // namespace sed::ir

#endif // SED_IR_TYPE_H