#include <ll1/AST/AST.h>

namespace ll1 {

const char *builtinTypeName(BuiltinType t) {
    switch (t) {
    case BuiltinType::Void: return "void";
    case BuiltinType::Int:  return "int";
    case BuiltinType::Char: return "char";
    case BuiltinType::Bool: return "bool";
    }
    return "unknown";
}

const char *BinaryExpr::opName(BinaryExpr::Op op) {
    switch (op) {
    case Op::Add: return "+";
    case Op::Sub: return "-";
    case Op::Mul: return "*";
    case Op::Div: return "/";
    case Op::Rem: return "%";
    case Op::Lt:  return "<";
    case Op::Gt:  return ">";
    case Op::Le:  return "<=";
    case Op::Ge:  return ">=";
    case Op::Eq:  return "==";
    case Op::Ne:  return "!=";
    case Op::And: return "&&";
    case Op::Or:  return "||";
    case Op::Assign: return "=";
    }
    return "?";
}

const char *UnaryExpr::opName(UnaryExpr::Op op) {
    switch (op) {
    case Op::Neg: return "-";
    case Op::Not: return "!";
    }
    return "?";
}

} // namespace ll1
