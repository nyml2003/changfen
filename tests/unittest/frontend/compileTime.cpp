#include "frontend/compileTime.h"
#include "frontend/symbolTable.h"
#include "frontend/type.h"
#include "index.h"
#include "gtest/gtest.h"

using namespace sed::frontend;

TEST(compileTime, add)
{
    auto a = createCompileTimeConstantValue(114, createInteger32());
    auto b = createCompileTimeConstantValue(514, createInteger32());
    auto c = BinaryOperator::Add;
    CompileTimeConstantValuePtr result = createCompileTimeConstantValueFromBinary(c, a, b);
    EXPECT_EQ(result->to_string(), "628");
    a = createCompileTimeConstantValue((float)1.14, createFloat32());
    b = createCompileTimeConstantValue((float)5.14, createFloat32());
    c = BinaryOperator::Add;
    result = createCompileTimeConstantValueFromBinary(c, a, b);
    EXPECT_EQ(result->to_string(), "6.280000");
    result = createCompileTimeConstantValueFromCast(createInteger32(), result);
    EXPECT_EQ(result->to_string(), "6");
}