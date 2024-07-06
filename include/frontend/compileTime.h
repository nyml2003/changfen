//
// Created by DELL on 2024/6/9.
//

#ifndef SED_COMPILE_TIME_H
#define SED_COMPILE_TIME_H
#include "index.h"

namespace sed::frontend
{
struct ZeroInitializer
{
};

struct CompileTimeConstantValue
{
    CompileTimeConstantValueKind kind;
    ValueTypePtr valueType;

    [[nodiscard]] std::string to_string() const;
};

CompileTimeConstantValuePtr createCompileTimeConstantValue(CompileTimeConstantValueKind kind, ValueTypePtr valueType);

CompileTimeConstantValuePtr createZeroCompileTimeConstantValue(ValueTypePtr valueType);

CompileTimeConstantValuePtr createCompileTimeConstantValueFromBinary(BinaryOperator op, CompileTimeConstantValuePtr lhs,
                                                                     CompileTimeConstantValuePtr rhs);

CompileTimeConstantValuePtr createCompileTimeConstantValueFromUnary(UnaryOperator op,
                                                                    CompileTimeConstantValuePtr operand);

CompileTimeConstantValuePtr createCompileTimeConstantValueFromCast(ValueTypePtr valueType,
                                                                   CompileTimeConstantValuePtr value);

} // namespace sed::frontend

#endif // SED_COMPILE_TIME_H
