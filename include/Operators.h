/* Leonardo Rodríguez */
#ifndef OPERATORS_H
#define OPERATORS_H

#include <functional>
#include <vector>
#include "Type.h"

/// @brief Expression operators (unary and binary)
enum class ExpOp {
    plus, times, minus, div, mod,
    andd, orr, nott,
    eq, neq, lt, gt, le, ge,
    floor, ceil, abs, sgn,
    min,max,pow,log
};

class Operator {

public:
    static std::vector<Ty> supported_types(ExpOp op);
    static bool is_infix_operator(ExpOp op);
    static std::string operator_string(ExpOp op);
};

class Unary : public Operator {
public:
    static std::function<float (float)> get_ff(ExpOp op);
    static std::function<int (int)> get_ii(ExpOp op);
    static std::function<int (float)> get_fi(ExpOp op);
    static std::function<bool (bool)> get_bb(ExpOp op);
    static const UnaryOpTy ff;
    static const UnaryOpTy ii;
    static const UnaryOpTy fi;
    static const UnaryOpTy bb;
};

class Binary : public Operator {
public:
    static std::function<float (int, float)> get_iff(ExpOp op);
    static std::function<float (float, int)> get_fif(ExpOp op);
    static std::function<float (float, float)> get_fff(ExpOp op);
    static std::function<int (int, int)> get_iii(ExpOp op);
    static std::function<bool (bool, bool)> get_bbb(ExpOp op);
    static std::function<bool (float, float)> get_ffb(ExpOp op);
    static std::function<bool (int, int)> get_iib(ExpOp op);
    static const BinaryOpTy iff;
    static const BinaryOpTy fif;
    static const BinaryOpTy fff;
    static const BinaryOpTy iii;
    static const BinaryOpTy bbb;
    static const BinaryOpTy ffb;
    static const BinaryOpTy iib;
};

#endif
