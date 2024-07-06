#include "frontend/symbolTable.h"
#include "frontend/compileTime.h"
#include "frontend/type.h"
#include "index.h"
#include "gtest/gtest.h"

using namespace sed::frontend;

TEST(SymbolEntry, create)
{
    auto valueType = createInteger32();
    auto symbolEntry = createSymbolEntry("a", valueType, Scope::Global, false, std::nullopt);
    EXPECT_EQ(symbolEntry->name, "a");
    EXPECT_EQ(symbolEntry->valueType, valueType);
    EXPECT_EQ(symbolEntry->scope, Scope::Global);
    EXPECT_EQ(symbolEntry->isConstant, false);
    EXPECT_EQ(symbolEntry->value, std::nullopt);
}

TEST(SymbolTable, create)
{
    auto symbolTable = createSymbolTable();
    EXPECT_EQ(symbolTable->entries.size(), 0);
    EXPECT_EQ(symbolTable->parent, std::nullopt);
}

TEST(SymbolTable, insert)
{
    auto symbolTable = createSymbolTable();
    auto valueType = createInteger32();
    auto symbolEntry = createSymbolEntry("a", valueType, Scope::Global, false, std::nullopt);
    symbolTable->insert(symbolEntry);
    EXPECT_EQ(symbolTable->entries.size(), 1);
    EXPECT_EQ(symbolTable->entries["a"], symbolEntry);
}

/**
 * int a;
 * int f() {
 *    bool a;
 *    int b;
 *    if (a) {
 *      int a[]={1,2,3};
 *         {
 *          float a;
 *          {
 *          }
 *      }
 *   }
 * }
 */

TEST(SymbolTable, find)
{
    auto globalSymbolTable = createSymbolTable();
    auto functionSymbolTable = createSymbolTable(globalSymbolTable);
    auto block1SymbolTable = createSymbolTable(functionSymbolTable);
    auto block2SymbolTable = createSymbolTable(block1SymbolTable);
    auto block3SymbolTable = createSymbolTable(block2SymbolTable);

    auto intType = createInteger32();
    auto boolType = createBoolean();
    auto floatType = createFloat32();
    auto arrayType = createArray(intType, 3);

    auto globalA = createSymbolEntry("a", intType, Scope::Global, false, std::nullopt);
    auto functionA = createSymbolEntry("a", boolType, Scope::Local, false, std::nullopt);
    auto functionB = createSymbolEntry("b", intType, Scope::Local, false, std::nullopt);
    auto block1A = createSymbolEntry("a", arrayType, Scope::Local, false, std::nullopt);
    auto block2A = createSymbolEntry("a", floatType, Scope::Local, false, std::nullopt);

    globalSymbolTable->insert(globalA);
    functionSymbolTable->insert(functionA);
    functionSymbolTable->insert(functionB);
    block1SymbolTable->insert(block1A);
    block2SymbolTable->insert(block2A);

    EXPECT_EQ(globalSymbolTable->find("a").value(), globalA);
    EXPECT_EQ(globalSymbolTable->find("b"), std::nullopt);
    EXPECT_EQ(functionSymbolTable->find("a").value(), functionA);
    EXPECT_EQ(functionSymbolTable->find("b").value(), functionB);
    EXPECT_EQ(block1SymbolTable->find("a").value(), block1A);
    EXPECT_EQ(block1SymbolTable->find("b").value(), functionB);
    EXPECT_EQ(block2SymbolTable->find("a").value(), block2A);
    EXPECT_EQ(block2SymbolTable->find("b").value(), functionB);
    EXPECT_EQ(block3SymbolTable->find("a").value(), block2A);
    EXPECT_EQ(block3SymbolTable->find("b").value(), functionB);
}

TEST(SymbolEntry, isCompileTimeConstant)
{
    auto valueType = createInteger32();
    auto symbolEntry = createSymbolEntry("a", valueType, Scope::Global, false, std::nullopt);
    EXPECT_EQ(symbolEntry->isCompileTimeConstant(), false);
}

TEST(SymbolEntry, to_string)
{
    auto valueType = createInteger32();
    auto symbolEntry = createSymbolEntry("a", valueType, Scope::Global, false, std::nullopt);
    EXPECT_EQ(symbolEntry->to_string(), "Global a: i32");
    auto constant = createCompileTimeConstantValue(3, valueType);
    symbolEntry = createSymbolEntry("b", valueType, Scope::Global, true, constant);
    EXPECT_EQ(symbolEntry->to_string(), "Global Const b: i32 = 3");
    EXPECT_TRUE(symbolEntry->isCompileTimeConstant());
}