#ifndef SED_INDEX_H
#define SED_INDEX_H
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <functional>

template <class... Ts> struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace sed
{
namespace frontend
{
namespace valueType
{
struct Integer32;
struct Boolean;
struct Float32;
struct Array;
struct Void;
struct Pointer;
struct Function;
} // namespace valueType

using ValueTypeKind = std::variant<valueType::Integer32, valueType::Float32, valueType::Array, valueType::Void,
                                   valueType::Boolean, valueType::Pointer, valueType::Function>;
struct ValueType;

using ValueTypePtr = std::shared_ptr<ValueType>;

enum class BinaryOperator;
enum class UnaryOperator;

struct ZeroInitializer;
// compile time computed constant value
struct CompileTimeConstantValue;

using CompileTimeConstantValuePtr = std::shared_ptr<CompileTimeConstantValue>;

using CompileTimeConstantValueKind =
    std::variant<int, float, bool, std::vector<CompileTimeConstantValuePtr>, ZeroInitializer>;

enum class Scope;

struct SymbolEntry;
struct SymbolTable;

using SymbolEntryPtr = std::shared_ptr<SymbolEntry>;

using SymbolTablePtr = std::shared_ptr<SymbolTable>;

namespace ast
{
namespace expression
{
struct Identifier;
struct Binary;
struct Unary;
struct FunctionCall;
struct Cast;
struct Constant;
struct InitializerList;

} // namespace expression
using ExpressionKind =
    std::variant<expression::Identifier, expression::Binary, expression::Unary, expression::FunctionCall,
                 expression::Cast, expression::Constant, expression::InitializerList>;

struct Expression;

using ExpressionPtr = std::shared_ptr<Expression>;

namespace statement
{
struct If;
struct While;
struct Break;
struct Continue;
struct Return;
struct Block;
struct Blank;
struct Assignment;
struct Expression;
struct Declaration;
struct FunctionDefinition;
} // namespace statement

using StatementKind = std::variant<statement::If, statement::While, statement::Break, statement::Continue,
                                   statement::Return, statement::Block, statement::Assignment, statement::Expression,
                                   statement::Declaration, statement::FunctionDefinition, statement::Blank>;

struct Statement;

using StatementPtr = std::shared_ptr<Statement>;

struct CompilationUnit;

} // namespace ast

struct Driver;

} // namespace frontend
namespace ir
{
// function.h
struct Function;
using FunctionPtr = std::shared_ptr<Function>;

// builder.h
struct Builder;

// context.h
struct Context;

// basic_block.h
using BasicBlockID = size_t;
struct BasicBlock;
using BasicBlockPtr = std::shared_ptr<BasicBlock>;
using BasicBlockPrevPtr = std::weak_ptr<BasicBlock>;

// instruction.h
using InstructionID = size_t;

namespace instruction
{
enum class BinaryOp;
struct Binary;
enum class ICmpOp;
struct ICmp;
enum class FCmpOp;
struct FCmp;
enum class CastOp;
struct Cast;
struct Ret;
struct Branch;
struct Jump;
struct Phi;
struct Alloca;
struct Load;
struct Store;
struct Call;
struct GetElementPtr;
struct Dummy;
} // namespace instruction
struct Instruction;

using InstructionKind =
    std::variant<instruction::Binary, instruction::ICmp, instruction::FCmp, instruction::Cast, instruction::Ret,
                 instruction::Branch, instruction::Jump, instruction::Phi, instruction::Alloca, instruction::Load,
                 instruction::Store, instruction::Call, instruction::GetElementPtr, instruction::Dummy>;
using InstructionPtr = std::shared_ptr<Instruction>;
using InstructionPrevPtr = std::weak_ptr<Instruction>;

// type.h
namespace type
{
struct Void;
struct Int;
struct Float;
struct Array;
struct Pointer;
} // namespace type

using TypeKind = std::variant<type::Void, type::Int, type::Float, type::Array, type::Pointer>;

struct Type;

using TypePtr = std::shared_ptr<Type>;

// operand.h
using OperandID = size_t;

namespace operand
{
struct Global;
struct Parameter;
struct ZeroInitializer;
struct Constant;
struct Local;
using ConstantPtr = std::shared_ptr<Constant>;
using ConstantKind = std::variant<int, float, std::vector<ConstantPtr>, ZeroInitializer>;
} // namespace operand

using OperandKind = std::variant<operand::Global, operand::Parameter, operand::ConstantPtr, operand::Local>;

struct Operand;

using OperandPtr = std::shared_ptr<Operand>;
} // namespace ir

namespace backend
{

enum class GeneralRegister;
enum class FloatRegister;

class Register;

enum class VirtualRegisterKind;

using VirtualRegisterID = size_t;

class VirtualRegister;

class Immediate;

using ImmediateValue = std::variant<int32_t, uint32_t, int64_t, uint64_t>;

using BasicBlockID = size_t;

struct BasicBlock;

using BasicBlockPtr = std::shared_ptr<BasicBlock>;
using BasicBlockPrevPtr = std::weak_ptr<BasicBlock>;

class Function;

using FunctionPtr = std::shared_ptr<Function>;

struct Context;
namespace instruction
{

struct Load;
struct FloatLoad;
struct PseudoLoad;
struct PseudoStore;
struct FloatPseudoLoad;
struct FloatPseudoStore;
struct Store;
struct FloatStore;
struct FloatMove;
struct FloatConvert;
struct Binary;
struct BinaryImmediate;
struct FloatBinary;
struct FloatMultiplyAdd;
struct FloatUnary;
struct Lui;
struct Li;
struct Call;
struct Branch;
struct Ret;
struct Jump;
struct Dummy;
struct Phi;

} // namespace instruction

using InstructionKind =
    std::variant<instruction::Load, instruction::FloatLoad, instruction::PseudoLoad, instruction::PseudoStore,
                 instruction::FloatPseudoLoad, instruction::FloatPseudoStore, instruction::Store,
                 instruction::FloatStore, instruction::FloatMove, instruction::FloatConvert, instruction::Binary,
                 instruction::BinaryImmediate, instruction::FloatBinary, instruction::FloatMultiplyAdd,
                 instruction::FloatUnary, instruction::Lui, instruction::Li, instruction::Call, instruction::Branch,
                 instruction::Ret, instruction::Jump, instruction::Dummy, instruction::Phi>;

using InstructionID = size_t;

struct Instruction;

using InstructionPtr = std::shared_ptr<Instruction>;
using InstructionPrevPtr = std::weak_ptr<Instruction>;

struct LocalMemory;

using OperandID = size_t;

struct Global;

using GlobalValue = std::variant<std::vector<uint32_t>, uint64_t>;

using OperandKind = std::variant<Immediate, VirtualRegister, Register, Global, LocalMemory>;

class Operand;

using OperandPtr = std::shared_ptr<Operand>;

struct Function;

struct Builder;

} // namespace backend

} // namespace sed

#endif // !SED_INDEX_H