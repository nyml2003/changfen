#include "frontend/type.h"
#include "index.h"

// It's the unit test of the type system.

#include "gtest/gtest.h"

using namespace sed::frontend;

TEST(Type, TrivialType)
{
    ValueTypePtr type = nullptr;
    type = createInteger32();
    EXPECT_TRUE(type->getTrivialType()->isInteger32());
    type = createFloat32();
    EXPECT_TRUE(type->getTrivialType()->isFloat32());
    type = createBoolean();
    EXPECT_TRUE(type->getTrivialType()->isBoolean());
    type = createVoid();
    EXPECT_TRUE(type->getTrivialType()->isVoid());
    type = createArray(createInteger32(), 10);
    EXPECT_TRUE(type->getTrivialType()->isInteger32());
    type = createPointer(createInteger32());
    EXPECT_TRUE(type->getTrivialType()->isInteger32());
}

TEST(Type, OPEARTOR_EQ)
{
    // bool operator==(ValueTypePtr &lhs, ValueTypePtr &rhs);
    ValueTypePtr intType1 = createInteger32();
    ValueTypePtr intType2 = createInteger32();
    EXPECT_TRUE(intType1 == intType2);
    ValueTypePtr floatType1 = createFloat32();
    ValueTypePtr floatType2 = createFloat32();
    EXPECT_TRUE(floatType1 == floatType2);
    ValueTypePtr boolType1 = createBoolean();
    ValueTypePtr boolType2 = createBoolean();
    EXPECT_TRUE(boolType1 == boolType2);
    ValueTypePtr voidType1 = createVoid();
    ValueTypePtr voidType2 = createVoid();
    EXPECT_TRUE(voidType1 == voidType2);
    ValueTypePtr arrayType1 = createArray(createInteger32(), 10);
    ValueTypePtr arrayType2 = createArray(createInteger32(), 10);
    EXPECT_TRUE(arrayType1 == arrayType2);
    ValueTypePtr pointerType1 = createPointer(createInteger32());
    ValueTypePtr pointerType2 = createPointer(createInteger32());
    EXPECT_TRUE(pointerType1 == pointerType2);

    EXPECT_TRUE(intType1 != floatType1);
    EXPECT_TRUE(intType1 != boolType1);
    EXPECT_TRUE(intType1 != voidType1);
    EXPECT_TRUE(intType1 != arrayType1);
    EXPECT_TRUE(intType1 != pointerType1);

    EXPECT_TRUE(floatType1 != intType1);
    EXPECT_TRUE(floatType1 != boolType1);
    EXPECT_TRUE(floatType1 != voidType1);
    EXPECT_TRUE(floatType1 != arrayType1);
    EXPECT_TRUE(floatType1 != pointerType1);

    EXPECT_TRUE(boolType1 != intType1);
    EXPECT_TRUE(boolType1 != floatType1);
    EXPECT_TRUE(boolType1 != voidType1);
    EXPECT_TRUE(boolType1 != arrayType1);
    EXPECT_TRUE(boolType1 != pointerType1);

    EXPECT_TRUE(voidType1 != intType1);
    EXPECT_TRUE(voidType1 != floatType1);
    EXPECT_TRUE(voidType1 != boolType1);
    EXPECT_TRUE(voidType1 != arrayType1);
    EXPECT_TRUE(voidType1 != pointerType1);

    EXPECT_TRUE(arrayType1 != intType1);
    EXPECT_TRUE(arrayType1 != floatType1);
    EXPECT_TRUE(arrayType1 != boolType1);
    EXPECT_TRUE(arrayType1 != voidType1);
    EXPECT_TRUE(arrayType1 != pointerType1);

    EXPECT_TRUE(pointerType1 != intType1);
    EXPECT_TRUE(pointerType1 != floatType1);
    EXPECT_TRUE(pointerType1 != boolType1);
    EXPECT_TRUE(pointerType1 != voidType1);
    EXPECT_TRUE(pointerType1 != arrayType1);
}

TEST(Type, Array)
{
    ValueTypePtr type = createArray(createInteger32(), 10);
    EXPECT_TRUE(type->isArray());
    EXPECT_EQ(type->size(), 10 * type->getTrivialType()->size());
    EXPECT_TRUE(type->getTrivialType()->isInteger32());
}

TEST(Type, getSize)
{
    ValueTypePtr type = createInteger32();
    EXPECT_EQ(type->size(), 32);
    type = createFloat32();
    EXPECT_EQ(type->size(), 32);
    type = createBoolean();
    EXPECT_EQ(type->size(), 8);
    type = createVoid();
    EXPECT_EQ(type->size(), 0);
    type = createArray(createInteger32(), 10);
    EXPECT_EQ(type->size(), 10 * 32);
    type = createPointer(createInteger32());
    EXPECT_EQ(type->size(), 64);
}

TEST(Type, Pointer)
{
    ValueTypePtr type = createPointer(createInteger32());
    EXPECT_TRUE(type->isPointer());
    EXPECT_TRUE(type->getTrivialType()->isInteger32());
}

TEST(Type, toString)
{
    ValueTypePtr type = createInteger32();
    EXPECT_EQ(type->to_string(), "i32");
    type = createFloat32();
    EXPECT_EQ(type->to_string(), "f32");
    type = createBoolean();
    EXPECT_EQ(type->to_string(), "bool");
    type = createVoid();
    EXPECT_EQ(type->to_string(), "void");
    type = createArray(createInteger32(), 10);
    EXPECT_EQ(type->to_string(), "i32[10]");
    type = createPointer(createInteger32());
    EXPECT_EQ(type->to_string(), "i32*");
    type = createPointer(createArray(createInteger32(), 10));
    EXPECT_EQ(type->to_string(), "i32[10]*");
    type = createPointer(createPointer(createInteger32()));
    EXPECT_EQ(type->to_string(), "i32**");
    type = createFunction(createInteger32(), {createInteger32(), createFloat32()});
    EXPECT_EQ(type->to_string(), "i32(i32, f32)");
}

TEST(Type, TypeInArray)
{
    ValueTypePtr type = createArray(createInteger32(), 10);
    ValueTypePtr i32 = createInteger32();
    EXPECT_TRUE(type->isArray());
    EXPECT_TRUE(type->getTrivialType()->isInteger32());
    EXPECT_EQ(type->size(), 10 * 32);
    EXPECT_TRUE(type->getTrivialType() == i32);
    ValueTypePtr type2 = createArray(type, 10);
    EXPECT_TRUE(type2->isArray());
    EXPECT_TRUE(type2->getTrivialType() == i32);
    EXPECT_TRUE(std::get<valueType::Array>(type2->kind).elementType == createArray(createInteger32(), 10));
}