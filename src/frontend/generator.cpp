#include "frontend/ast.h"
#include "index.h"
#include <sstream>
#include <variant>

namespace sed::frontend
{
std::string ast::Expression::to_string() const
{
    std::stringstream ss;
    return std::visit(overloaded{[&](const ast::expression::Identifier &id) {
                                     ss << id.name;
                                     return ss.str();
                                 },
                                 [&](const ast::expression::Binary &bin) {
                                     if (bin.op == BinaryOperator::At)
                                     {
                                         ss << bin.lhs->to_string() << "[" << bin.rhs->to_string() << "]";
                                         return ss.str();
                                     }
                                     ss << "(" << bin.lhs->to_string();
                                     switch (bin.op)
                                     {
                                     case BinaryOperator::Add:
                                         ss << "+";
                                         break;
                                     case BinaryOperator::Sub:
                                         ss << "-";
                                         break;
                                     case BinaryOperator::Mul:
                                         ss << "*";
                                         break;
                                     case BinaryOperator::Div:
                                         ss << "/";
                                         break;
                                     case BinaryOperator::Mod:
                                         ss << "%";
                                         break;
                                     case BinaryOperator::And:
                                         ss << "&&";
                                         break;
                                     case BinaryOperator::Or:
                                         ss << "||";
                                         break;
                                     case BinaryOperator::Eq:
                                         ss << "==";
                                         break;
                                     case BinaryOperator::Ne:
                                         ss << "!=";
                                         break;
                                     case BinaryOperator::Lt:
                                         ss << "<";
                                         break;
                                     case BinaryOperator::Le:
                                         ss << "<=";
                                         break;
                                     case BinaryOperator::Gt:
                                         ss << ">";
                                         break;
                                     case BinaryOperator::Ge:
                                         ss << ">=";
                                         break;
                                     case BinaryOperator::At:
                                         ss << "[";
                                         break;
                                     }
                                     ss << bin.rhs->to_string() << ")";
                                     return ss.str();
                                 },
                                 [&](const ast::expression::Unary &un) {
                                     ss << "(";
                                     switch (un.op)
                                     {
                                     case UnaryOperator::Plus:
                                         ss << "+";
                                         break;
                                     case UnaryOperator::Minus:
                                         ss << "-";
                                         break;
                                     case UnaryOperator::Not:
                                         ss << "!";
                                         break;
                                     }
                                     ss << un.operand->to_string() << ")";
                                     return ss.str();
                                 },
                                 [&](const ast::expression::InitializerList &il) {
                                     ss << "{";
                                     for (auto &init : il.elements)
                                     {
                                         ss << init->to_string();
                                         if (&init != &il.elements.back())
                                         {
                                             ss << ",";
                                         }
                                     }
                                     ss << "}";
                                     return ss.str();
                                 },
                                 [&](const ast::expression::FunctionCall &fc) {
                                     ss << fc.function->to_string() << "(";
                                     for (auto &arg : fc.arguments)
                                     {
                                         ss << arg->to_string() << ",";
                                     }
                                     ss << ")";
                                     return ss.str();
                                 },
                                 [&](const ast::expression::Cast &cast) {
                                     ss << "(" << cast.targetType->to_string() << ")" << cast.value->to_string();
                                     return ss.str();
                                 },
                                 [&](const ast::expression::Constant &constant) {
                                     ss << constant.value->to_string();
                                     return ss.str();
                                 },
                                 [](auto &expr) { return std::string("not implemented"); }},
                      kind);
}

std::string ast::Statement::to_string() const
{
    std::stringstream buf;
    buf << "Stmt ";

    std::visit(overloaded{
                   [&buf](const ast::statement::Blank &kind) { buf << "Blank"; },
                   [&buf](const ast::statement::If &kind) {
                       buf << "If" << std::endl;
                       buf << "COND: " << std::endl;
                       buf << kind.condition->to_string() << std::endl;
                       buf << "THEN: " << std::endl;
                       buf << kind.thenStatement->to_string();
                       if (kind.elseStatement.has_value())
                       {
                           buf << "ELSE: " << std::endl;
                           buf << kind.elseStatement.value()->to_string();
                       }
                   },
                   [&buf](const ast::statement::While &kind) {
                       buf << "While" << std::endl;
                       buf << "COND:" << std::endl;
                       buf << kind.condition->to_string() << std::endl;
                       buf << "BODY:" << std::endl;
                       buf << kind.body->to_string();
                   },
                   [&buf](const ast::statement::Break &kind) { buf << "Break"; },
                   [&buf](const ast::statement::Continue &kind) { buf << "Continue"; },
                   [&buf](const ast::statement::Return &kind) {
                       buf << "Return" << std::endl;
                       if (kind.value.has_value())
                       {
                           buf << "VALUE:" << std::endl;
                           buf << kind.value.value()->to_string();
                       }
                   },
                   [&buf](const ast::statement::Block &kind) {
                       buf << "Block" << std::endl;
                       if (kind.statements.empty())
                       {
                           buf << "EMPTY" << std::endl;
                       }
                       for (auto &stmt : kind.statements)
                       {
                           buf << stmt->to_string();
                       }
                   },
                   [&buf](const ast::statement::Assignment &kind) {
                       buf << "Assign" << std::endl;
                       buf << kind.lhs->to_string() << " = " << kind.rhs->to_string();
                   },
                   [&buf](const ast::statement::Expression &kind) {
                       buf << "Expr" << std::endl;
                       buf << kind.expression->to_string();
                   },
                   [&buf](const ast::statement::Declaration &kind) {
                       buf << "Decl" << std::endl;
                       switch (kind.scope)
                       {
                       case Scope::Global:
                           buf << "Global ";
                           break;
                       case Scope::Local:
                           buf << "Local ";
                           break;
                       default:
                           throw std::runtime_error("Unknown scope");
                           buf << "Unknown ";
                       }
                       if (kind.isConst)
                       {
                           buf << "Const ";
                       }
                       buf << std::endl;
                       for (const auto &[type, name, value] : kind.definitions)
                       {
                           buf << type->to_string() << " " << name;
                           if (value.has_value())
                           {
                               buf << "=" << value.value()->to_string() << std::endl;
                           }
                       }
                   },
                   [&buf](const ast::statement::FunctionDefinition &kind) {
                       buf << "FuncDef" << std::endl;
                       buf << "Signature:" << std::endl;
                       buf << kind.function->to_string() << std::endl;
                       buf << "Params:" << std::endl;
                       if (kind.parameters.empty())
                       {
                           buf << "void" << std::endl;
                       }
                       for (auto &param : kind.parameters)
                       {
                           buf << kind.symbolTable->find(param).value()->to_string() << std::endl;
                       }
                       buf << "Body:" << std::endl;
                       if (kind.body.has_value())
                       {
                           buf << kind.body.value()->to_string() << std::endl;
                       }
                       else
                       {
                           buf << "EMPTY" << std::endl;
                       }
                   },
                   // unreachable.
                   [&buf](const auto &kind) { buf << "UNREACHABLE_STMT"; },
               },
               this->kind);

    return buf.str();
}

std::string ast::CompilationUnit::to_string() const
{
    std::stringstream ss;
    for (auto &stmt : statements)
    {
        ss << stmt->to_string() << std::endl;
    }
    return ss.str();
}
} // namespace sed::frontend