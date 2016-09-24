/* Leonardo Rodríguez */

#include "Operators.h"
#include "Type.h"

#include <cmath>
#include <FigException.h>

 const UnaryOpTy Unary::ff = UnaryOpTy(Type::tfloat, Type::tfloat);
 const UnaryOpTy Unary::ii = UnaryOpTy(Type::tint, Type::tint);
 const UnaryOpTy Unary::fi = UnaryOpTy(Type::tfloat, Type::tint);
 const UnaryOpTy Unary::bb = UnaryOpTy(Type::tbool, Type::tbool);
 const BinaryOpTy Binary::iff
= BinaryOpTy(Type::tint, Type::tfloat, Type::tfloat);
 const BinaryOpTy Binary::fif
= BinaryOpTy(Type::tfloat, Type::tint, Type::tfloat);
 const BinaryOpTy Binary::fff
= BinaryOpTy(Type::tfloat, Type::tfloat, Type::tfloat);
 const BinaryOpTy Binary::iii
= BinaryOpTy(Type::tint, Type::tint, Type::tint);
 const BinaryOpTy Binary::bbb
= BinaryOpTy(Type::tbool, Type::tbool, Type::tbool);
 const BinaryOpTy Binary::ffb
= BinaryOpTy(Type::tfloat, Type::tfloat, Type::tbool);
 const BinaryOpTy Binary::iib
= BinaryOpTy(Type::tint, Type::tint, Type::tbool);

std::string Operator::operator_string(ExpOp op) {
    std::string result;
    switch(op) {
    case ExpOp::plus: result = "+"; break;
    case ExpOp::times: result = "*"; break;
    case ExpOp::minus: result = "-"; break;
    case ExpOp::div: result = "/"; break;
    case ExpOp::mod: result = "%"; break;
    case ExpOp::andd: result = "&"; break;
    case ExpOp::orr: result = "|"; break;
    case ExpOp::nott: result = "!"; break;
    case ExpOp::eq: result = "=="; break;
    case ExpOp::neq: result = "!="; break;
    case ExpOp::lt: result = "<"; break;
    case ExpOp::gt: result = ">"; break;
    case ExpOp::le: result = "<="; break;
    case ExpOp::ge: result = ">="; break;
    case ExpOp::floor: result = "floor" ; break;
    case ExpOp::abs: result = "abs" ; break;
    case ExpOp::ceil: result = "ceil" ; break;
    case ExpOp::sgn: result = "sgn"; break;
    case ExpOp::min: result = "min"; break;
    case ExpOp::max: result = "max"; break;
    case ExpOp::pow: result = "pow"; break;
    case ExpOp::log: result = "log"; break;
    }
    return result;
}

bool Operator::is_infix_operator(ExpOp op) {
    switch(op) {
    case ExpOp::andd: return true;
    case ExpOp::orr: return true;
    case ExpOp::le: return true;
    case ExpOp::lt: return true;
    case ExpOp::ge: return true;
    case ExpOp::gt: return true;
    case ExpOp::eq: return true;
    case ExpOp::neq: return true;
    case ExpOp::mod: return true;
    case ExpOp::minus: return true;
    case ExpOp::div: return true;
    case ExpOp::times: return true;
    case ExpOp::plus: return true;
    default: ;
    }
    return (false);
}

std::vector<Ty> Operator::supported_types(ExpOp op) {
    switch(op) {
    case ExpOp::sgn: {
        return std::vector<Ty> { Unary::fi };
    }
    case ExpOp::floor: {
        return std::vector<Ty> { Unary::fi };
    }
    case ExpOp::ceil: {
        return std::vector<Ty> { Unary::fi };
    }
    case ExpOp::minus: {
        return std::vector<Ty> {
            Unary::ii, Unary::ff,
            Binary::fff, Binary::iii
        };
    }
    case ExpOp::abs: {
        return std::vector<Ty> { Unary::ii, Unary::ff };
    }
    case ExpOp::nott: {
        return std::vector<Ty> { Unary::bb };
    }
    case ExpOp::times: //same type as plus
    case ExpOp::div: //
    case ExpOp::min: //
    case ExpOp::max: //
    case ExpOp::plus: {
        return std::vector<Ty> { Binary::fff, Binary::iii };
    }
    case ExpOp::mod: {
        return std::vector<Ty> { Binary::iii };
    }
    case ExpOp::andd: //same type as or
    case ExpOp::orr: {
        return std::vector<Ty> { Binary::bbb };
    }
    case ExpOp::eq: //same as neq
    case ExpOp::neq: {
        return std::vector<Ty> {
            Binary::ffb, Binary::iib, Binary::bbb
        };
    }
    case ExpOp::lt: //same type as ge
    case ExpOp::gt: //
    case ExpOp::le: //
    case ExpOp::ge: {
        return std::vector<Ty> { Binary::ffb, Binary::iib };
    }
    case ExpOp::log: {
        return std::vector<Ty> { Binary::fff };
    }
    case ExpOp::pow: {
        return std::vector<Ty> {
            Binary::fff, Binary::fif,
            Binary::iff, Binary::iii
        };
    }
    }
    return (std::vector<Ty>());
}

std::function<float (float)> Unary::get_ff(ExpOp op) {
    switch (op) {
    case ExpOp::minus: return [] (float x) -> float {return -x;};
    case ExpOp::abs: return [] (float x) -> float {return std::abs(x);};
    default:
        throw_FigException("Operator is not float->float");
    }
}

std::function<int (int)> Unary::get_ii(ExpOp op) {
    switch (op) {
    case ExpOp::minus: return [] (int x) -> int {return -x;};
    case ExpOp::abs: return [] (int x) -> int {return std::abs(x);};
    default:
        throw_FigException("Operator is not int->int");
    }
}

std::function<int (float)> Unary::get_fi(ExpOp op) {
    switch (op) {
    case ExpOp::sgn: return [] (float x) -> int {
            return (0.0f < x) - (x < 0.0f);
        };
    case ExpOp::floor: return [] (float x) -> int {
            return std::floor(x);
        };
    case ExpOp::ceil: return [] (float x) -> int {
            return std::ceil(x);
        };
    default:
        throw_FigException("Operator is not float->int");
    }
}

std::function<bool (bool)> Unary::get_bb(ExpOp op) {
    switch(op) {
    case ExpOp::nott: return [] (bool b) -> bool {
            return !b;
        };
    default:
        throw_FigException("Operator is not bool->bool");
    }
}

std::function<float (int, float)> Binary::get_iff(ExpOp op) {
    switch (op) {
    case ExpOp::pow: return [] (int x, float y) -> float {
            return std::pow(x, y);
        };
    default:
        throw_FigException("Operator is not int->float->int");
    }
}

std::function<float (float, int)> Binary::get_fif(ExpOp op) {
    switch (op) {
    case ExpOp::pow: return [] (float x, int y) -> float {
            return std::pow(x, y);
        };
    default:
        throw_FigException("Operator is not int->float->int");
    }
}

std::function<float (float, float)> Binary::get_fff(ExpOp op) {
    switch (op) {
    case ExpOp::pow: return [] (float x, float y) -> float {
            return std::pow(x, y);
        };
    case ExpOp::minus: return [] (float x, float y) -> float {
            return x - y;
        };
    case ExpOp::plus: return [] (float x, float y) -> float {
            return x + y;
        };
    case ExpOp::times: return [] (float x, float y) -> float {
            return x * y;
        };
    case ExpOp::div: return [] (float x, float y) -> float {
            return x / y;
        };
    case ExpOp::min: return [] (float x, float y) -> float {
            return std::min(x, y);
        };
    case ExpOp::max: return [] (float x, float y) -> float {
            return std::max(x, y);
        };
    case ExpOp::log: return [] (float x, float y) -> float {
            return std::log2(x)/std::log2(y);
        };
    default:
        throw_FigException("Operator is not float->float->float");
    }
}

std::function<int (int, int)> Binary::get_iii(ExpOp op) {
    switch (op) {
    case ExpOp::pow: return [] (int x, int y) -> int {
            return std::pow(x, y);
        };
    case ExpOp::minus: return [] (int x, int y) -> int {
            return x - y;
        };
    case ExpOp::plus: return [] (int x, int y) -> int {
            return (x + y);
        };
    case ExpOp::times: return [] (int x, int y) -> int {
            return x * y;
        };
    case ExpOp::div: return [] (int x, int y) -> int {
            return x / y;
        };
    case ExpOp::min: return [] (int x, int y) -> int {
            return std::min(x, y);
        };
    case ExpOp::max: return [] (int x, int y) -> int {
            return std::max(x, y);
        };
    case ExpOp::mod: return [] (int x, int y) -> int {
            return x % y;
        };
    default:
        throw_FigException("Operator is not int->int->int");
    }
}

std::function<bool (bool, bool)> Binary::get_bbb(ExpOp op) {
    switch (op) {
    case ExpOp::andd: return [] (bool x, bool y) -> bool {
            return x && y;
        };
    case ExpOp::orr: return [] (bool x, bool y) -> bool {
            return x || y;
        };
    case ExpOp::eq: return [] (bool x, bool y) -> bool {
            return x == y;
        };
    case ExpOp::neq: return [] (bool x, bool y) -> bool {
            return x != y;
        };
    default:
        throw_FigException("Operator is not bool->bool->bool");
    }
}

std::function<bool (float, float)> Binary::get_ffb(ExpOp op) {
    switch(op) {
    case ExpOp::lt: return [] (float x, float y) -> bool {
            return x < y;
        };
    case ExpOp::le: return [] (float x, float y) -> bool {
            return x <= y;
        };
    case ExpOp::gt: return [] (float x, float y) -> bool {
            return x > y;
        };
    case ExpOp::ge: return [] (float x, float y) -> bool {
            return x >= y;
        };
    case ExpOp::eq: return [] (float x, float y) -> bool {
            return x == y;
        };
    case ExpOp::neq: return [] (float x, float y) -> bool {
            return x != y;
        };
    default:
        throw_FigException("Operator float->float->bool");
    }
}

std::function<bool (int, int)> Binary::get_iib(ExpOp op) {
    switch(op) {
    case ExpOp::lt: return [] (int x, int y) -> bool {
            return x < y;
        };
    case ExpOp::le: return [] (int x, int y) -> bool {
            return x <= y;
        };
    case ExpOp::gt: return [] (int x, int y) -> bool {
            return x > y;
        };
    case ExpOp::ge: return [] (int x, int y) -> bool {
            return x >= y;
        };
    case ExpOp::eq: return [] (int x, int y) -> bool {
            return x == y;
        };
    case ExpOp::neq: return [] (int x, int y) -> bool {
            return x != y;
        };
    default:
        throw_FigException("Operator is not int->int->bool");
    }
}
