#include "frontend/type.h"
#include "frontend/ast.h"
#include <iostream>
#include <sstream>
#include <utility>
#include <variant>

namespace sed::frontend
{

std::vector<size_t> valueType::Array::getDimensions() const
{
    std::vector<size_t> dimensions = {};
    dimensions.push_back(capacity);
    if (elementType->isArray())
    {
        auto array = std::get<valueType::Array>(elementType->kind);
        auto subDimensions = array.getDimensions();
        dimensions.insert(dimensions.end(), subDimensions.begin(), subDimensions.end());
    }
    return dimensions;
}

ValueType::ValueType(ValueTypeKind kind) : kind(kind)
{
}

std::string ValueType::to_string() const
{
    std::stringstream ss;
    return std::visit(overloaded{[&ss](const valueType::Integer32 &) {
                                     ss << "i32";
                                     return ss.str();
                                 },
                                 [&ss](const valueType::Float32 &) {
                                     ss << "f32";
                                     return ss.str();
                                 },
                                 [&ss](const valueType::Boolean &) {
                                     ss << "bool";
                                     return ss.str();
                                 },
                                 [&ss](const valueType::Void &) {
                                     ss << "void";
                                     return ss.str();
                                 },
                                 [&ss](const valueType::Array &array) {
                                     ss << array.elementType->to_string() << "[" << array.capacity << "]";
                                     return ss.str();
                                 },
                                 [&ss](const valueType::Pointer &pointer) {
                                     ss << pointer.valueType->to_string() << "*";
                                     return ss.str();
                                 },
                                 [&ss](const valueType::Function &function) {
                                     ss << function.returnType->to_string() << "(";
                                     for (size_t i = 0; i < function.parameterTypes.size(); i++)
                                     {
                                         ss << function.parameterTypes[i]->to_string();
                                         if (i != function.parameterTypes.size() - 1)
                                         {
                                             ss << ", ";
                                         }
                                     }
                                     ss << ")";
                                     return ss.str();
                                 }},
                      kind);
}

ValueTypePtr ValueType::getTrivialType() const
{
    return std::visit(overloaded{
                          [](const valueType::Integer32 &) { return createInteger32(); },
                          [](const valueType::Float32 &) { return createFloat32(); },
                          [](const valueType::Boolean &) { return createBoolean(); },
                          [](const valueType::Void &) { return createVoid(); },
                          [](const valueType::Array &array) { return array.elementType->getTrivialType(); },
                          [](const valueType::Pointer &pointer) { return pointer.valueType->getTrivialType(); },
                          [](const valueType::Function &function) {
                              return createFunction(function.returnType, function.parameterTypes);
                          },
                          [](auto &) { return nullptr; },
                      },
                      kind);
}

bool ValueType::isInteger32() const
{
    return std::holds_alternative<valueType::Integer32>(kind);
}

bool ValueType::isFloat32() const
{
    return std::holds_alternative<valueType::Float32>(kind);
}

bool ValueType::isBoolean() const
{
    return std::holds_alternative<valueType::Boolean>(kind);
}

bool ValueType::isArray() const
{
    return std::holds_alternative<valueType::Array>(kind);
}

bool ValueType::isVoid() const
{
    return std::holds_alternative<valueType::Void>(kind);
}

bool ValueType::isPointer() const
{
    return std::holds_alternative<valueType::Pointer>(kind);
}

bool ValueType::isFunction() const
{
    return std::holds_alternative<valueType::Function>(kind);
}

size_t ValueType::size() const
{
    return std::visit(
        overloaded{
            [](const valueType::Integer32 &) -> size_t { return 32; },
            [](const valueType::Float32 &) -> size_t { return 32; },
            [](const valueType::Boolean &) -> size_t { return 1; },
            [](const valueType::Array &array) -> size_t { return array.capacity * array.elementType->size(); },
            [](const valueType::Void &) -> size_t { return 0; },
            [](const valueType::Pointer &) -> size_t { return 64; },
            [](auto &) -> size_t {
                throw std::runtime_error("Unknown type");
                return 0;
            },
        },
        kind);
}

ValueTypePtr createInteger32()
{
    return std::make_shared<ValueType>(valueType::Integer32{});
}

ValueTypePtr createFloat32()
{
    return std::make_shared<ValueType>(valueType::Float32{});
}

ValueTypePtr createBoolean()
{
    return std::make_shared<ValueType>(valueType::Boolean{});
}

ValueTypePtr createArray(ValueTypePtr elementType, std::optional<std::size_t> size)
{
    if (!size.has_value())
    {
        return createPointer(elementType);
    }
    else
    {
        return std::make_shared<ValueType>(valueType::Array{
            elementType->getTrivialType(),
            elementType,
            size.value(),
        });
    }
}

std::optional<ValueTypePtr> createArrayFromExpression(ValueTypePtr elementType, std::optional<ast::ExpressionPtr> size)
{
    if (!size.has_value())
    {
        return std::make_optional(createPointer(elementType));
    }
    auto sizeValue = size.value();
    if (!sizeValue->isCompileTimeConstant())
    {
        return std::nullopt;
    }
    auto constant = sizeValue->getCompileTimeConstantValue();
    if (!constant.has_value())
    {
        return std::nullopt;
    }
    auto constantValue = createCompileTimeConstantValueFromCast(createInteger32(), constant.value());
    int arraySize = std::get<int>(constantValue->kind);
    return std::make_optional(createArray(elementType, arraySize));
}

ValueTypePtr createVoid()
{
    return std::make_shared<ValueType>(valueType::Void{});
}

ValueTypePtr createPointer(ValueTypePtr valueType)
{
    return std::make_shared<ValueType>(valueType::Pointer{
        valueType->getTrivialType(),
        valueType,
    });
}

ValueTypePtr createFunction(ValueTypePtr returnType, std::vector<ValueTypePtr> parameterTypes)
{
    return std::make_shared<ValueType>(valueType::Function{
        returnType,
        parameterTypes,
    });
}

bool operator==(ValueTypePtr lhs, ValueTypePtr rhs)
{
    if (lhs->kind.index() != rhs->kind.index())
    {
        return false;
    }
    return std::visit(
        overloaded{
            [](valueType::Integer32 &, valueType::Integer32 &) { return true; },
            [](valueType::Float32 &, valueType::Float32 &) { return true; },
            [](valueType::Boolean &, valueType::Boolean &) { return true; },
            [](valueType::Array &lhs, valueType::Array &rhs) {
                return lhs.capacity == rhs.capacity && lhs.elementType == rhs.elementType;
            },
            [](valueType::Void &, valueType::Void &) { return true; },
            [](valueType::Pointer &lhs, valueType::Pointer &rhs) { return lhs.valueType == rhs.valueType; },
            [](auto &lhs, auto &rhs) { return false; },
        },
        lhs->kind, rhs->kind);
}

bool operator!=(ValueTypePtr lhs, ValueTypePtr rhs)
{
    return !(lhs == rhs);
}

} // namespace sed::frontend
