#ifndef SED_frontend_TYPE_H
#define SED_frontend_TYPE_H
#include "index.h"
#include <optional>
#include <string>

namespace sed::frontend
{
namespace valueType
{
struct Integer32
{
};

struct Boolean
{
};

struct Float32
{
};

struct Array
{
    ValueTypePtr trivialType;
    ValueTypePtr elementType;
    std::size_t capacity;
    std::vector<size_t> getDimensions() const;
};

struct Void
{
};

struct Pointer
{
    ValueTypePtr trivialType;
    ValueTypePtr valueType;
};

struct Function
{
    ValueTypePtr returnType;
    std::vector<ValueTypePtr> parameterTypes;
};
} // namespace valueType
struct ValueType
{
    ValueTypeKind kind;
    explicit ValueType(ValueTypeKind kind);

    [[nodiscard]] std::string to_string() const;
    ValueTypePtr getTrivialType() const;
    bool isInteger32() const;
    bool isFloat32() const;
    bool isBoolean() const;
    bool isArray() const;
    bool isVoid() const;
    bool isPointer() const;
    bool isFunction() const;
    // the byte size of the type
    size_t size() const;
};

ValueTypePtr createInteger32();

ValueTypePtr createFloat32();

ValueTypePtr createBoolean();

ValueTypePtr createArray(ValueTypePtr elementType, std::optional<std::size_t> size = std::nullopt);

std::optional<ValueTypePtr> createArrayFromExpression(ValueTypePtr elementType, std::optional<ast::ExpressionPtr> size);

ValueTypePtr createVoid();

ValueTypePtr createPointer(ValueTypePtr valueType);
/**
 * @brief Create a function type
 * @param returnType the return type of the function
 * @param parameterTypes
 */
ValueTypePtr createFunction(ValueTypePtr returnType, std::vector<ValueTypePtr> parameterTypes);

bool operator==(ValueTypePtr lhs, ValueTypePtr rhs);

bool operator!=(ValueTypePtr lhs, ValueTypePtr rhs);

} // namespace sed::frontend

#endif // SED_frontend_TYPE_H