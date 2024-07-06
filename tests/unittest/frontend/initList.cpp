#include "frontend/driver.h"
#include "gtest/gtest.h"
using namespace sed::frontend;

Driver driver;

TEST(InitList, index)
{

    auto i1 = ast::createConstant(createCompileTimeConstantValue(1, createInteger32()));
    auto i2 = ast::createConstant(createCompileTimeConstantValue(2, createInteger32()));
    auto i3 = ast::createConstant(createCompileTimeConstantValue(3, createInteger32()));
    auto i4 = ast::createConstant(createCompileTimeConstantValue(4, createInteger32()));
    auto i5 = ast::createConstant(createCompileTimeConstantValue(5, createInteger32()));
    auto i6 = ast::createConstant(createCompileTimeConstantValue(6, createInteger32()));
    auto i7 = ast::createConstant(createCompileTimeConstantValue(7, createInteger32()));
    auto i8 = ast::createConstant(createCompileTimeConstantValue(8, createInteger32()));
    // int rrr[2][3][4] = {1, 2, 3, 4, {5}, {6}, {7, 8}};
    auto a5 = ast::createInitializerList({i5});
    auto a6 = ast::createInitializerList({i6});
    auto a78 = ast::createInitializerList({i7, i8});
    auto rrr = ast::createInitializerList({i1, i2, i3, i4, a5, a6, a78});

    auto valueType = createArray(createArray(createArray(createInteger32(), 4), 3), 2);

    std::get<ast::expression::InitializerList>(rrr->kind).setValueType(valueType, driver);
    auto element1 = std::get<ast::expression::InitializerList>(rrr->kind).elements[0];
    EXPECT_EQ(element1->to_string(), "{{1,2,3,4},{5,0,0,0},{6,0,0,0}}");

    EXPECT_EQ(rrr->to_string(), "{{{1,2,3,4},{5,0,0,0},{6,0,0,0}},{{7,8,0,0},{},{}}}");
}

TEST(InitList, index2)
{
    // INT arr[2][3][4]
}