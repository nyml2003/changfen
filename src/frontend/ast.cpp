#include "frontend/ast.h"
#include "frontend/compileTime.h"
#include "frontend/driver.h"
#include "frontend/symbolTable.h"
namespace sed::frontend
{

namespace ast
{

void outputElements(std::vector<ExpressionPtr> elements, std::string message)
{
    std::cout << message << ":";
    if (elements.empty())
    {
        std::cout << "empty" << std::endl;
        return;
    }
    else
    {
        std::cout << "{";
        for (size_t i = 0; i < elements.size(); ++i)
        {
            std::cout << elements[i]->to_string();
            if (i != elements.size() - 1)
            {
                std::cout << ", ";
            }
            else
            {
                std::cout << "}" << std::endl;
            }
        }
    }
}

size_t countElements(std::vector<size_t> dims, size_t from, size_t to, std::vector<ExpressionPtr> elements)
{
    size_t weight = 1;
    for (size_t i = from; i < to; ++i)
    {
        weight *= dims[i];
    }
    size_t count = 0;
    for (size_t i = 0; i < elements.size(); ++i)
    {
        if (std::holds_alternative<expression::InitializerList>(elements[i]->kind))
        {
            count +=
                countElements(dims, from - 1, to, std::get<expression::InitializerList>(elements[i]->kind).elements);
        }
        else
        {
            count += weight;
        }
    }
    return count;
}

void expression::InitializerList::setValueType(ValueTypePtr valueType, Driver &driver)
{
    // valueType should be an array, like [3 x [4 x i32]]
    if (!valueType->isArray())
    {
        throw std::runtime_error("Invalid value type");
    }
    this->valueType = valueType;
    if (elements.size() == 0)
    {
        is_zeroinitializer = true;
        return;
    }
    is_zeroinitializer = false;
    auto capacity = std::get<valueType::Array>(valueType->kind).capacity;
    auto elementType = std::get<valueType::Array>(valueType->kind).elementType;
    auto trivialType = valueType->getTrivialType();
    if (elementType == trivialType) // this is a trivial array
    {
        for (size_t i = 0; i < elements.size(); ++i)
        {
            if (elements[i]->getValueType() != elementType)
            {
                elements[i] = createCast(elementType, elements[i], driver);
            }
        }

        if (elements.size() < capacity)
        {
            for (size_t i = elements.size(); i < capacity; ++i)
            {
                elements.push_back(createConstant(createZeroCompileTimeConstantValue(elementType)));
            }
        }
    }
    else // this is a nested array
    {
        size_t elementSize = elementType->size() / trivialType->size();
        std::vector<size_t> dims = std::get<valueType::Array>(elementType->kind).getDimensions();
        std::vector<ExpressionPtr> newElements = {};
        std::vector<ExpressionPtr> elementBuffer = {};
        for (size_t i = 0; i < elements.size(); ++i)
        {
            auto e = elements[i];
            if (std::holds_alternative<expression::InitializerList>(e->kind))
            {
                if (elementBuffer.empty())
                {
                    std::get<expression::InitializerList>(e->kind).setValueType(elementType, driver);
                    newElements.push_back(e);
                }
                else
                {
                    elementBuffer.push_back(e);
                    if (countElements(dims, dims.size(), dims.size(), elementBuffer) == elementSize)
                    {
                        auto initializerList = createInitializerList(elementBuffer);
                        std::get<expression::InitializerList>(initializerList->kind).setValueType(elementType, driver);
                        newElements.push_back(initializerList);
                        elementBuffer.clear();
                    }
                }
            }
            else if (e->getValueType() == trivialType)
            {
                elementBuffer.push_back(e);
                if (countElements(dims, dims.size(), dims.size(), elementBuffer) == elementSize)
                {
                    auto initializerList = createInitializerList(elementBuffer);
                    std::get<expression::InitializerList>(initializerList->kind).setValueType(elementType, driver);
                    newElements.push_back(initializerList);
                    elementBuffer.clear();
                }
            }
        }
        if (!elementBuffer.empty())
        {
            auto initializerList = createInitializerList(elementBuffer);
            std::get<expression::InitializerList>(initializerList->kind).setValueType(elementType, driver);
            newElements.push_back(initializerList);
            elementBuffer.clear();
        }
        if (newElements.size() < capacity)
        {
            for (size_t i = newElements.size(); i < capacity; ++i)
            {
                auto initializerList = createInitializerList({});
                std::get<expression::InitializerList>(initializerList->kind).setValueType(elementType, driver);
                newElements.push_back(initializerList);
            }
        }
        elements = newElements;
    }
}
Expression::Expression(ExpressionKind kind) : kind(kind)
{
}

bool Expression::isCompileTimeConstant() const
{
    return std::visit(
        overloaded{
            [](const expression::Binary &binary) {
                return binary.lhs->isCompileTimeConstant() && binary.rhs->isCompileTimeConstant() &&
                       binary.op != BinaryOperator::At;
            },
            [](const expression::Unary &unary) { return unary.operand->isCompileTimeConstant(); },
            [](const expression::Cast &cast) { return cast.value->isCompileTimeConstant(); },
            [](const expression::Constant &) { return true; },
            [](const expression::InitializerList &initializerList) {
                for (const auto &element : initializerList.elements)
                {
                    if (!element->isCompileTimeConstant())
                    {
                        return false;
                    }
                }
                return true;
            },
            [](const expression::Identifier &identifier) { return identifier.symbolEntry->isCompileTimeConstant(); },
            [](const auto &) { return false; },
        },
        kind);
}

std::optional<CompileTimeConstantValuePtr> Expression::getCompileTimeConstantValue() const
{
    if (!isCompileTimeConstant())
    {
        return std::nullopt;
    }
    if (std::holds_alternative<expression::Binary>(kind))
    {
        auto binary = std::get<expression::Binary>(kind);
        if (binary.op == BinaryOperator::At)
        {
            return std::nullopt;
        }
    }
    return std::visit(overloaded{
                          [](const expression::Binary &binary) {
                              return std::make_optional(createCompileTimeConstantValueFromBinary(
                                  binary.op, binary.lhs->getCompileTimeConstantValue().value(),
                                  binary.rhs->getCompileTimeConstantValue().value()));
                          },
                          [](const expression::Unary &unary) {
                              return std::make_optional(createCompileTimeConstantValueFromUnary(
                                  unary.op, unary.operand->getCompileTimeConstantValue().value()));
                          },
                          [](const expression::InitializerList &initializerList) {
                              if (initializerList.elements.empty())
                              {
                                  return std::make_optional(
                                      createCompileTimeConstantValue(ZeroInitializer{}, initializerList.valueType));
                              }
                              std::vector<CompileTimeConstantValuePtr> elements = {};
                              elements.reserve(initializerList.elements.size());
                              for (const auto &element : initializerList.elements)
                              {
                                  elements.push_back(element->getCompileTimeConstantValue().value());
                              }
                              return std::make_optional(
                                  createCompileTimeConstantValue(elements, initializerList.valueType));
                          },
                          [](const expression::Cast &cast) {
                              return std::make_optional(createCompileTimeConstantValueFromCast(
                                  cast.targetType, cast.value->getCompileTimeConstantValue().value()));
                          },
                          [](const expression::Constant &constant) { return std::make_optional(constant.value); },
                          [](const expression::Identifier &identifier) { return identifier.symbolEntry->value; },
                          [](auto &) -> std::optional<CompileTimeConstantValuePtr> { return std::nullopt; },
                      },
                      kind);
}

ValueTypePtr Expression::getValueType() const
{
    return std::visit(overloaded{
                          [](const expression::Binary &binary) { return binary.symbol->valueType; },
                          [](const expression::Unary &unary) { return unary.symbol->valueType; },
                          [](const expression::FunctionCall &functionCall) { return functionCall.call->valueType; },
                          [](const expression::Cast &cast) { return cast.symbol->valueType; },
                          [](const expression::Constant &constant) { return constant.value->valueType; },
                          [](const expression::InitializerList &initializerList) { return initializerList.valueType; },
                          [](const expression::Identifier &identifier) { return identifier.symbolEntry->valueType; },
                      },
                      kind);
}

Statement::Statement(StatementKind kind) : kind(kind)
{
}

CompilationUnit::CompilationUnit() : symbolTable(createSymbolTable()), statements({})
{
}

ExpressionPtr createIdentifier(SymbolEntryPtr symbolEntry)
{
    return std::make_shared<Expression>(ExpressionKind{expression::Identifier{
        symbolEntry->name,
        symbolEntry,
    }});
}

ExpressionPtr createConstant(CompileTimeConstantValuePtr value)
{
    return std::make_shared<Expression>(ExpressionKind{expression::Constant{value}});
}

ExpressionPtr createBinary(BinaryOperator op, ExpressionPtr lhs, ExpressionPtr rhs, Driver &driver)
{
    auto name = driver.getTempSymbolEntryName();
    auto symbolTable = driver.currentSymbolTable;
    std::optional<ValueTypePtr> valueType = std::nullopt; // 在switch里确定valueType
    if (op != BinaryOperator::At)
    {
        if (lhs->isCompileTimeConstant())
        {
            lhs = createConstant(lhs->getCompileTimeConstantValue().value());
        }
    }
    if (rhs->isCompileTimeConstant())
    {
        rhs = createConstant(rhs->getCompileTimeConstantValue().value());
    }
    auto ltype = lhs->getValueType();
    auto rtype = rhs->getValueType();
    switch (op)
    {
    case BinaryOperator::Add:
    case BinaryOperator::Sub:
    case BinaryOperator::Mul:
    case BinaryOperator::Div:
    case BinaryOperator::Mod: {
        valueType = std::visit(overloaded{
                                   [](valueType::Integer32 &, valueType::Integer32 &) { return createInteger32(); },
                                   [](valueType::Float32 &, valueType::Float32 &) { return createFloat32(); },
                                   [&](valueType::Integer32 &, valueType::Float32 &) {
                                       lhs = createCast(createFloat32(), lhs, driver);
                                       return createFloat32();
                                   },
                                   [&](valueType::Float32 &, valueType::Integer32 &) {
                                       rhs = createCast(createFloat32(), rhs, driver);
                                       return createFloat32();
                                   },
                                   [&](valueType::Integer32 &, valueType::Boolean &) {
                                       rhs = createCast(createInteger32(), rhs, driver);
                                       return createInteger32();
                                   },
                                   [&](valueType::Boolean &, valueType::Integer32 &) {
                                       lhs = createCast(createInteger32(), lhs, driver);
                                       return createInteger32();
                                   },
                                   [&](valueType::Float32 &, valueType::Boolean &) {
                                       rhs = createCast(createFloat32(), rhs, driver);
                                       return createFloat32();
                                   },
                                   [&](valueType::Boolean &, valueType::Float32 &) {
                                       lhs = createCast(createFloat32(), lhs, driver);
                                       return createFloat32();
                                   },
                                   [](auto &, auto &) {
                                       throw std::runtime_error("Unsupported binary operation between");
                                       return createVoid();
                                   },
                               },
                               ltype->kind, rtype->kind);
        break;
    }
    case BinaryOperator::Eq:
    case BinaryOperator::Ne: {
        valueType = createBoolean();
        std::visit(
            overloaded{
                [&](valueType::Integer32 &, valueType::Float32 &) { lhs = createCast(createFloat32(), lhs, driver); },
                [&](valueType::Float32 &, valueType::Integer32 &) { rhs = createCast(createFloat32(), rhs, driver); },
                [](valueType::Integer32, valueType::Integer32) {},
                [](valueType::Float32, valueType::Float32) {},
                [](valueType::Boolean, valueType::Boolean) {},
                [&](valueType::Integer32, valueType::Boolean) { rhs = createCast(createInteger32(), rhs, driver); },
                [&](valueType::Boolean, valueType::Integer32) { lhs = createCast(createInteger32(), lhs, driver); },
                [&](valueType::Float32, valueType::Boolean) { rhs = createCast(createFloat32(), rhs, driver); },
                [&](valueType::Boolean, valueType::Float32) { lhs = createCast(createFloat32(), lhs, driver); },
                [](auto &, auto &) { throw std::runtime_error("Unsupported binary operation"); },
            },
            ltype->kind, rtype->kind);
        break;
    }
    case BinaryOperator::Lt:
    case BinaryOperator::Le:
    case BinaryOperator::Gt:
    case BinaryOperator::Ge: {
        valueType = createBoolean();
        std::visit(
            overloaded{
                [&](valueType::Integer32 &, valueType::Float32 &) { lhs = createCast(createFloat32(), lhs, driver); },
                [&](valueType::Float32 &, valueType::Integer32 &) { rhs = createCast(createFloat32(), rhs, driver); },
                [](valueType::Integer32, valueType::Integer32) {},
                [](valueType::Float32, valueType::Float32) {},
                [](auto &lhs, auto &rhs) { throw std::runtime_error("Unsupported binary operation"); },
            },
            ltype->kind, rtype->kind);
        break;
    }
    case BinaryOperator::And:
    case BinaryOperator::Or: {
        valueType = createBoolean();
        std::visit(
            overloaded{
                [](valueType::Boolean, valueType::Boolean) {},
                [&](valueType::Integer32, valueType::Integer32) {
                    lhs = createCast(createBoolean(), lhs, driver);
                    rhs = createCast(createBoolean(), rhs, driver);
                },
                [&](valueType::Float32, valueType::Float32) {
                    lhs = createCast(createBoolean(), lhs, driver);
                    rhs = createCast(createBoolean(), rhs, driver);
                },
                [&](valueType::Boolean, valueType::Integer32) { rhs = createCast(createBoolean(), rhs, driver); },
                [&](valueType::Boolean, valueType::Float32) { rhs = createCast(createBoolean(), rhs, driver); },
                [&](valueType::Integer32, valueType::Boolean) { lhs = createCast(createBoolean(), lhs, driver); },
                [&](valueType::Float32, valueType::Boolean) { lhs = createCast(createBoolean(), lhs, driver); },
                [&](valueType::Integer32, valueType::Float32) {
                    lhs = createCast(createBoolean(), lhs, driver);
                    rhs = createCast(createBoolean(), rhs, driver);
                },
                [&](valueType::Float32, valueType::Integer32) {
                    lhs = createCast(createBoolean(), lhs, driver);
                    rhs = createCast(createBoolean(), rhs, driver);
                },
                [](auto &lhs, auto &rhs) { throw std::runtime_error("Unsupported binary operation"); },
            },
            ltype->kind, rtype->kind);
    }
    break;
    case BinaryOperator::At: {
        std::visit(overloaded{
                       [&](valueType::Pointer &pointer, valueType::Integer32 &) { valueType = pointer.valueType; },
                       [&](valueType::Pointer &pointer, valueType::Float32 &) {
                           rhs = createCast(createInteger32(), rhs, driver);
                           valueType = pointer.valueType;
                       },
                       [&](valueType::Array &array, valueType::Integer32 &) { valueType = array.elementType; },
                       [&](valueType::Array &array, valueType::Float32 &) {
                           rhs = createCast(createInteger32(), rhs, driver);
                           valueType = array.elementType;
                       },
                       [&](auto &lhs, auto &rhs) { throw std::runtime_error("Unsupported binary operation"); },
                   },
                   ltype->kind, rtype->kind);
    }
    break;
    }
    auto symbolEntry = createSymbolEntry(name, valueType.value(), Scope::Temp, false, std::nullopt);
    symbolTable->insert(symbolEntry);
    return std::make_shared<Expression>(ExpressionKind{expression::Binary{
        op,
        lhs,
        rhs,
        symbolEntry,
    }});
}

ExpressionPtr createUnary(UnaryOperator op, ExpressionPtr operand, Driver &driver)
{
    auto name = driver.getTempSymbolEntryName();
    auto symbolTable = driver.currentSymbolTable;
    if (operand->isCompileTimeConstant())
    {
        operand = createConstant(operand->getCompileTimeConstantValue().value());
        auto valueType = operand->getValueType();
        switch (op)
        {
        case UnaryOperator::Plus:
        case UnaryOperator::Minus: {
            return std::visit(
                overloaded{
                    [&](valueType::Integer32 &value) {
                        return createConstant(createCompileTimeConstantValueFromUnary(
                            op, operand->getCompileTimeConstantValue().value()));
                    },
                    [&](valueType::Float32 &value) {
                        return createConstant(createCompileTimeConstantValueFromUnary(
                            op, operand->getCompileTimeConstantValue().value()));
                    },
                    [&](valueType::Boolean &value) {
                        return createConstant(createCompileTimeConstantValueFromUnary(
                            op, createCast(createInteger32(), operand, driver)->getCompileTimeConstantValue().value()));
                    },
                    [](auto &value) {
                        throw std::runtime_error("Unsupported unary operation");
                        return createConstant(createZeroCompileTimeConstantValue(createVoid()));
                    },
                },
                valueType->kind);
            break;
        }
        case UnaryOperator::Not: {
            return std::visit(overloaded{
                                  [&](valueType::Boolean &value) {
                                      return createConstant(createCompileTimeConstantValueFromUnary(
                                          op, operand->getCompileTimeConstantValue().value()));
                                  },
                                  [&](valueType::Integer32 &value) {
                                      operand = createCast(createBoolean(), operand, driver);
                                      return createConstant(createCompileTimeConstantValueFromUnary(
                                          op, operand->getCompileTimeConstantValue().value()));
                                  },
                                  [&](valueType::Float32 &value) {
                                      operand = createCast(createBoolean(), operand, driver);
                                      return createConstant(createCompileTimeConstantValueFromUnary(
                                          op, operand->getCompileTimeConstantValue().value()));
                                  },
                                  [](auto &value) {
                                      throw std::runtime_error("Unsupported unary operation");
                                      return createConstant(createZeroCompileTimeConstantValue(createVoid()));
                                  },
                              },
                              valueType->kind);
            break;
        }
        }
    }
    auto valueType = operand->getValueType();
    switch (op)
    {
    case UnaryOperator::Plus:
    case UnaryOperator::Minus: {
        std::visit(overloaded{
                       [](valueType::Integer32 &value) {},
                       [](valueType::Float32 &value) {},
                       [&](valueType::Boolean &value) { operand = createCast(createInteger32(), operand, driver); },
                       [](auto &value) { throw std::runtime_error("Unsupported unary operation"); },
                   },
                   valueType->kind);
        break;
    }
    case UnaryOperator::Not: {
        std::visit(overloaded{
                       [](valueType::Boolean &value) {},
                       [&](valueType::Integer32 &value) { operand = createCast(createBoolean(), operand, driver); },
                       [&](valueType::Float32 &value) { operand = createCast(createBoolean(), operand, driver); },
                       [](auto &value) { throw std::runtime_error("Unsupported unary operation"); },
                   },
                   valueType->kind);
        break;
    }
    }
    auto symbolEntry = createSymbolEntry(name, valueType, Scope::Temp, false, std::nullopt);
    symbolTable->insert(symbolEntry);
    return std::make_shared<Expression>(ExpressionKind{expression::Unary{
        op,
        operand,
        symbolEntry,
    }});
}

ExpressionPtr createFunctionCall(SymbolEntryPtr function, std::vector<ExpressionPtr> arguments, Driver &driver)
{
    auto name = driver.getTempSymbolEntryName();
    auto symbolTable = driver.currentSymbolTable;
    auto returnType = std::get<valueType::Function>(function->valueType->kind).returnType;
    auto parameterTypes = std::get<valueType::Function>(function->valueType->kind).parameterTypes;
    if (arguments.size() != parameterTypes.size())
    {
        throw std::runtime_error("Invalid number of arguments");
    }
    for (std::size_t i = 0; i < arguments.size(); ++i)
    {
        if (parameterTypes[i]->isArray() || parameterTypes[i]->isPointer())
        {
            continue;
        }
        if (arguments[i]->isCompileTimeConstant())
        {
            arguments[i] = createConstant(arguments[i]->getCompileTimeConstantValue().value());
        }
        if (arguments[i]->getValueType() != parameterTypes[i])
        {
            arguments[i] = createCast(parameterTypes[i], arguments[i], driver);
        }
    }
    auto symbolEntry = createSymbolEntry(name, returnType, Scope::Temp, false, std::nullopt);
    symbolTable->insert(symbolEntry);
    return std::make_shared<Expression>(ExpressionKind{expression::FunctionCall{
        arguments,
        function,
        symbolEntry,
    }});
}

ExpressionPtr createCast(ValueTypePtr targetType, ExpressionPtr value, Driver &driver)
{
    if (value->isCompileTimeConstant())
    {
        value = createConstant(value->getCompileTimeConstantValue().value());
    }
    if (value->getValueType() == targetType)
    {
        return value;
    }
    auto valueType = value->getValueType();
    return std::visit(
        overloaded{[&](valueType::Boolean &, valueType::Integer32 &) {
                       return createBinary(BinaryOperator::Ne, value,
                                           createConstant(createZeroCompileTimeConstantValue(value->getValueType())),
                                           driver);
                   },
                   [&](valueType::Boolean &, valueType::Float32 &) {
                       return createBinary(BinaryOperator::Ne, value,
                                           createConstant(createZeroCompileTimeConstantValue(value->getValueType())),
                                           driver);
                   },

                   [&](auto &, auto &) {
                       auto name = driver.getTempSymbolEntryName();
                       auto symbolTable = driver.currentSymbolTable;
                       auto symbolEntry = createSymbolEntry(name, targetType, Scope::Temp, false, std::nullopt);
                       symbolTable->insert(symbolEntry);
                       return std::make_shared<Expression>(ExpressionKind{expression::Cast{
                           targetType,
                           value,
                           symbolEntry,
                       }});
                   }},

        targetType->kind, valueType->kind);
}

ExpressionPtr createInitializerList(std::vector<ExpressionPtr> elements)
{
    for (auto element : elements)
    {
        if (element->isCompileTimeConstant())
        {
            element = createConstant(element->getCompileTimeConstantValue().value());
        }
    }
    return std::make_shared<Expression>(ExpressionKind{expression::InitializerList{elements, nullptr}});
}

StatementPtr createBlank()
{
    return std::make_shared<Statement>(StatementKind(statement::Blank{}));
}

StatementPtr createIf(ExpressionPtr condition, StatementPtr thenStatement, std::optional<StatementPtr> elseStatement,
                      Driver &driver)
{
    condition = createCast(createBoolean(), condition, driver);
    if (condition->isCompileTimeConstant())
    {
        if (std::get<bool>(condition->getCompileTimeConstantValue().value()->kind))
        {
            return thenStatement;
        }
        else
        {
            if (elseStatement.has_value())
            {
                return elseStatement.value();
            }
            return createBlank();
        }
        return thenStatement;
    }
    return std::make_shared<Statement>(StatementKind(statement::If{
        condition,
        thenStatement,
        elseStatement,
    }));
}

StatementPtr createWhile(ExpressionPtr condition, StatementPtr body, Driver &driver)
{
    condition = createCast(createBoolean(), condition, driver);
    return std::make_shared<Statement>(StatementKind{statement::While{
        condition,
        body,
    }});
}

StatementPtr createReturn(std::optional<ExpressionPtr> value, Driver &driver)
{
    if (value.has_value())
    {
        auto expression = value.value();
        auto functionType = std::get<statement::FunctionDefinition>(driver.currentFunction->kind).function->valueType;
        auto returnType = std::get<valueType::Function>(functionType->kind).returnType;
        if (expression->getValueType() != returnType)
        {
            value = createCast(returnType, expression, driver);
        }
    }
    return std::make_shared<Statement>(StatementKind{statement::Return{value}});
}

StatementPtr createBreak()
{
    return std::make_shared<Statement>(StatementKind(statement::Break{}));
}

StatementPtr createContinue()
{
    return std::make_shared<Statement>(StatementKind(statement::Continue{}));
}

StatementPtr createBlock(SymbolTablePtr parent)
{
    auto symbolTable = createSymbolTable(parent);
    return std::make_shared<Statement>(StatementKind(statement::Block{symbolTable, {}}));
}

StatementPtr createAssignment(ExpressionPtr lhs, ExpressionPtr rhs, Driver &driver)
{
    if (rhs->isCompileTimeConstant())
    {
        rhs = createConstant(rhs->getCompileTimeConstantValue().value());
    }
    if (lhs->getValueType() != rhs->getValueType())
    {
        rhs = createCast(lhs->getValueType(), rhs, driver);
    }
    return std::make_shared<Statement>(StatementKind(statement::Assignment{
        lhs,
        rhs,
    }));
}

StatementPtr createExpression(ExpressionPtr expression)
{
    if (expression->isCompileTimeConstant())
    {
        return createBlank();
    }
    return std::make_shared<Statement>(StatementKind(statement::Expression{expression}));
}

StatementPtr createDeclaration(
    Scope scope, bool isConst,
    std::vector<std::tuple<ValueTypePtr, std::string, std::optional<ExpressionPtr>>> definitions, Driver &driver)
{
    for (auto [valueType, name, value] : definitions)
    {
        if (value.has_value())
        {
            if (value.value()->isCompileTimeConstant())
            {
                value = createConstant(value.value()->getCompileTimeConstantValue().value());
            }
            if (value.value()->getValueType() != valueType)
            {
                value = createCast(valueType, value.value(), driver);
            }
        }
    }
    return std::make_shared<Statement>(StatementKind(statement::Declaration{
        scope,
        isConst,
        definitions,
    }));
}

StatementPtr createFunctionDefinition(SymbolTablePtr parent, ValueTypePtr returnType, std::string name,
                                      std::vector<std::tuple<ValueTypePtr, std::string>> parameters)
{
    std::vector<ValueTypePtr> parameterTypes = {};
    std::vector<std::string> parameterNames = {};
    for (auto [parameterType, parameterName] : parameters)
    {
        parameterTypes.push_back(parameterType->isArray()
                                     ? createPointer(std::get<valueType::Array>(parameterType->kind).elementType)
                                     : parameterType);
        parameterNames.push_back(parameterName);
    }
    auto function = createFunction(returnType, parameterTypes);
    auto symbolEntry = createSymbolEntry(name, function, Scope::Global, false, std::nullopt);
    auto symbolTable = createSymbolTable(parent);
    parent->insert(symbolEntry);
    for (auto [parameterType, parameterName] : parameters)
    {
        symbolTable->insert(createSymbolEntry(
            parameterName,
            parameterType->isArray() ? createPointer(std::get<valueType::Array>(parameterType->kind).elementType)
                                     : parameterType,
            Scope::Param, false, std::nullopt));
    }
    return std::make_shared<Statement>(
        StatementKind(statement::FunctionDefinition{symbolTable, symbolEntry, parameterNames, std::nullopt}));
}
SymbolEntryPtr createSymbolEntry(Scope scope, bool isConstant,
                                 std::tuple<ValueTypePtr, std::string, std::optional<ExpressionPtr>> definition)
{
    auto [valueType, name, value] = definition;
    return frontend::createSymbolEntry(name, valueType, scope, isConstant,
                                       isConstant ? (value.has_value() ? value.value()->getCompileTimeConstantValue()
                                                                       : createZeroCompileTimeConstantValue(valueType))
                                                  : std::nullopt);
}

std::optional<statement::Block> Statement::asBlock() const
{
    if (std::holds_alternative<statement::Block>(this->kind))
    {
        return std::get<statement::Block>(this->kind);
    }
    return std::nullopt;
}

} // namespace ast
} // namespace sed::frontend
