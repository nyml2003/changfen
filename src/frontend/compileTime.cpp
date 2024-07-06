//
// Created by DELL on 2024/6/9.
//
#include "frontend/compileTime.h"

#include "frontend/ast.h"
#include "frontend/type.h"
#include <stdexcept>

namespace sed::frontend
{
std::string CompileTimeConstantValue::to_string() const
{
    return std::visit(overloaded{[](int value) {
                                     auto result = std::to_string(value);
                                     return result;
                                 },
                                 [](float value) {
                                     auto result = std::to_string(value);
                                     return result;
                                 },
                                 [](bool value) {
                                     auto result = value ? "true" : "false";
                                     return std::string{result};
                                 },
                                 [](std::vector<CompileTimeConstantValuePtr> &elements) {
                                     std::string result = "{";
                                     for (const auto &value : elements)
                                     {
                                         result += value->to_string() + ", ";
                                     }
                                     result.pop_back();
                                     result.pop_back();
                                     result += "}";
                                     return result;
                                 },
                                 [](ZeroInitializer &) { return std::string("ZeroInitializer"); },
                                 [](auto &value) { return std::string("Unknown"); }},
                      kind);
}

CompileTimeConstantValuePtr createCompileTimeConstantValue(CompileTimeConstantValueKind kind, ValueTypePtr valueType)
{
    return std::make_shared<CompileTimeConstantValue>(CompileTimeConstantValue{kind, valueType});
}

CompileTimeConstantValuePtr createZeroCompileTimeConstantValue(ValueTypePtr valueType)
{
    return std::visit(
        overloaded{
            [&valueType](valueType::Integer32 &) { return createCompileTimeConstantValue((int)0, valueType); },
            [&valueType](valueType::Float32 &) { return createCompileTimeConstantValue((float)0, valueType); },
            [&valueType](valueType::Array &) { return createCompileTimeConstantValue(ZeroInitializer{}, valueType); },
            [&valueType](auto &value) { return createCompileTimeConstantValue(0, valueType); }},
        valueType->kind);
}

CompileTimeConstantValuePtr createCompileTimeConstantValueFromBinary(BinaryOperator op, CompileTimeConstantValuePtr lhs,
                                                                     CompileTimeConstantValuePtr rhs)
{
    return std::visit(
        overloaded{
            [&op, &lhs, &rhs](int, int) {
                switch (op)
                {
                case BinaryOperator::Add:
                    return createCompileTimeConstantValue(std::get<int>(lhs->kind) + std::get<int>(rhs->kind),
                                                          createInteger32());
                case BinaryOperator::Sub:
                    return createCompileTimeConstantValue(std::get<int>(lhs->kind) - std::get<int>(rhs->kind),
                                                          createInteger32());
                case BinaryOperator::Mul:
                    return createCompileTimeConstantValue(std::get<int>(lhs->kind) * std::get<int>(rhs->kind),
                                                          createInteger32());
                case BinaryOperator::Div:
                    return createCompileTimeConstantValue(std::get<int>(lhs->kind) / std::get<int>(rhs->kind),
                                                          createInteger32());
                case BinaryOperator::Mod:
                    return createCompileTimeConstantValue(std::get<int>(lhs->kind) % std::get<int>(rhs->kind),
                                                          createInteger32());
                case BinaryOperator::Ge:
                    return createCompileTimeConstantValue(std::get<int>(lhs->kind) >= std::get<int>(rhs->kind),
                                                          createBoolean());
                case BinaryOperator::Gt:
                    return createCompileTimeConstantValue(std::get<int>(lhs->kind) > std::get<int>(rhs->kind),
                                                          createBoolean());
                case BinaryOperator::Le:
                    return createCompileTimeConstantValue(std::get<int>(lhs->kind) <= std::get<int>(rhs->kind),
                                                          createBoolean());
                case BinaryOperator::Lt:
                    return createCompileTimeConstantValue(std::get<int>(lhs->kind) < std::get<int>(rhs->kind),
                                                          createBoolean());
                case BinaryOperator::Eq:
                    return createCompileTimeConstantValue(std::get<int>(lhs->kind) == std::get<int>(rhs->kind),
                                                          createBoolean());
                case BinaryOperator::Ne:
                    return createCompileTimeConstantValue(std::get<int>(lhs->kind) != std::get<int>(rhs->kind),
                                                          createBoolean());
                default:
                    throw std::runtime_error("Invalid binary operator");
                }
            },
            [&op, &lhs, &rhs](float, float) {
                switch (op)
                {
                case BinaryOperator::Add:
                    return createCompileTimeConstantValue(std::get<float>(lhs->kind) + std::get<float>(rhs->kind),
                                                          createFloat32());
                case BinaryOperator::Sub:
                    return createCompileTimeConstantValue(std::get<float>(lhs->kind) - std::get<float>(rhs->kind),
                                                          createFloat32());
                case BinaryOperator::Mul:
                    return createCompileTimeConstantValue(std::get<float>(lhs->kind) * std::get<float>(rhs->kind),
                                                          createFloat32());
                case BinaryOperator::Div:
                    return createCompileTimeConstantValue(std::get<float>(lhs->kind) / std::get<float>(rhs->kind),
                                                          createFloat32());
                case BinaryOperator::Ge:
                    return createCompileTimeConstantValue(std::get<float>(lhs->kind) >= std::get<float>(rhs->kind),
                                                          createBoolean());
                case BinaryOperator::Gt:
                    return createCompileTimeConstantValue(std::get<float>(lhs->kind) > std::get<float>(rhs->kind),
                                                          createBoolean());
                case BinaryOperator::Le:
                    return createCompileTimeConstantValue(std::get<float>(lhs->kind) <= std::get<float>(rhs->kind),
                                                          createBoolean());
                case BinaryOperator::Lt:
                    return createCompileTimeConstantValue(std::get<float>(lhs->kind) < std::get<float>(rhs->kind),
                                                          createBoolean());
                case BinaryOperator::Eq:
                    return createCompileTimeConstantValue(std::get<float>(lhs->kind) == std::get<float>(rhs->kind),
                                                          createBoolean());
                case BinaryOperator::Ne:
                    return createCompileTimeConstantValue(std::get<float>(lhs->kind) != std::get<float>(rhs->kind),
                                                          createBoolean());
                default:
                    throw std::runtime_error("Invalid binary operator");
                }
            },
            [&op, &lhs, &rhs](bool, bool) {
                switch (op)
                {
                case BinaryOperator::And:
                    return createCompileTimeConstantValue(std::get<bool>(lhs->kind) && std::get<bool>(rhs->kind),
                                                          createBoolean());
                case BinaryOperator::Or:
                    return createCompileTimeConstantValue(std::get<bool>(lhs->kind) || std::get<bool>(rhs->kind),
                                                          createBoolean());
                case BinaryOperator::Eq:
                    return createCompileTimeConstantValue(std::get<bool>(lhs->kind) == std::get<bool>(rhs->kind),
                                                          createBoolean());
                case BinaryOperator::Ne:
                    return createCompileTimeConstantValue(std::get<bool>(lhs->kind) != std::get<bool>(rhs->kind),
                                                          createBoolean());
                default:
                    throw std::runtime_error("Invalid binary operator");
                }
            },

            [](auto &lhs, auto &rhs) {
                throw std::runtime_error("Invalid binary operator");
                return CompileTimeConstantValuePtr{};
            }},
        lhs->kind, rhs->kind);
}

CompileTimeConstantValuePtr createCompileTimeConstantValueFromUnary(UnaryOperator op,
                                                                    CompileTimeConstantValuePtr operand)
{
    return std::visit(
        overloaded{[&op, &operand](int) {
                       switch (op)
                       {
                       case UnaryOperator::Plus:
                           return createCompileTimeConstantValue(+std::get<int>(operand->kind), createInteger32());
                       case UnaryOperator::Minus:
                           return createCompileTimeConstantValue(-std::get<int>(operand->kind), createInteger32());
                       default:
                           throw std::runtime_error("Invalid unary operator");
                       }
                   },
                   [&op, &operand](float) {
                       switch (op)
                       {
                       case UnaryOperator::Plus:
                           return createCompileTimeConstantValue(+std::get<float>(operand->kind), createFloat32());
                       case UnaryOperator::Minus:
                           return createCompileTimeConstantValue(-std::get<float>(operand->kind), createFloat32());
                       default:
                           throw std::runtime_error("Invalid unary operator");
                       }
                   },
                   [&op, &operand](bool) {
                       switch (op)
                       {
                       case UnaryOperator::Not:
                           return createCompileTimeConstantValue(!std::get<bool>(operand->kind), createBoolean());
                       default:
                           throw std::runtime_error("Invalid unary operator");
                       }
                   },
                   [](auto &operand) {
                       throw std::runtime_error("Invalid unary operator");
                       return CompileTimeConstantValuePtr{};
                   }},
        operand->kind);
}

CompileTimeConstantValuePtr createCompileTimeConstantValueFromCast(ValueTypePtr valueType,
                                                                   CompileTimeConstantValuePtr value)
{
    return std::visit(
        overloaded{
            [&valueType, &value](valueType::Integer32 &) {
                return std::visit(
                    overloaded{[&valueType](float value) {
                                   return createCompileTimeConstantValue(static_cast<int>(value), valueType);
                               },
                               [&valueType](bool value) {
                                   return createCompileTimeConstantValue(static_cast<int>(value), valueType);
                               },
                               [&valueType](int value) { return createCompileTimeConstantValue(value, valueType); },
                               [](auto &value) {
                                   throw std::runtime_error("Invalid cast");
                                   return CompileTimeConstantValuePtr{};
                               }},
                    value->kind);
            },
            [&valueType, &value](valueType::Float32 &) {
                return std::visit(
                    overloaded{[&valueType](int value) {
                                   return createCompileTimeConstantValue(static_cast<float>(value), valueType);
                               },
                               [&valueType](bool value) {
                                   return createCompileTimeConstantValue(static_cast<float>(value), valueType);
                               },
                               [&valueType](float value) { return createCompileTimeConstantValue(value, valueType); },
                               [](auto &value) {
                                   throw std::runtime_error("Invalid cast");
                                   return CompileTimeConstantValuePtr{};
                               }},
                    value->kind);
            },
            [&valueType, &value](valueType::Boolean &) {
                return std::visit(
                    overloaded{[&valueType](int value) {
                                   return createCompileTimeConstantValue(static_cast<bool>(value), valueType);
                               },
                               [&valueType](float value) {
                                   return createCompileTimeConstantValue(static_cast<bool>(value), valueType);
                               },
                               [&valueType](bool value) { return createCompileTimeConstantValue(value, valueType); },
                               [](auto &value) {
                                   throw std::runtime_error("Invalid cast");
                                   return CompileTimeConstantValuePtr{};
                               }},
                    value->kind);
            },
            [](auto &valueType) {
                throw std::runtime_error("Invalid cast");
                return CompileTimeConstantValuePtr{};
            }},
        valueType->kind);
}
} // namespace sed::frontend