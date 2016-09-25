/* Leonardo Rodríguez */
#include "Type.h"
#include "Operators.h"
#include <algorithm>

BasicTy Ty::to_basic() const {
    assert(is_basic());
    return (static_cast<const BasicTy&>(*this));
}

std::string BasicTy::to_string() const {
    return Ty::to_string(this->get_type());
}

std::string FunTy::to_string() const {
    return this->get_ty1()->to_string() + "->" + this->get_ty2()->to_string();
}


FunTy Ty::to_fun() const {
    assert(is_fun());
    return (static_cast<const FunTy&>(*this));
}

UnaryOpTy Ty::to_unary_ty() const {
    assert(is_unary_type());
    return (static_cast<const UnaryOpTy&>(*this));
}

BinaryOpTy Ty::to_binary_ty() const {
    assert(is_binary_type());
    return (static_cast<const BinaryOpTy&>(*this));
}

bool operator==(const Ty& ty1, const Ty& ty2) {
    bool result = false;
    if (ty1.is_basic()) {
        if (ty2.is_basic()) {
            BasicTy basic1 = ty1.to_basic();
            BasicTy basic2 = ty2.to_basic();
            result = (basic1.get_type() == basic2.get_type());
        }
    } else if (ty1.is_fun()) {
        if (ty2.is_fun()) {
            FunTy fun1 = ty1.to_fun();
            FunTy fun2 = ty2.to_fun();
            bool fst_eq = *(fun1.get_ty1()) == *(fun2.get_ty1());
            bool snd_eq = *(fun1.get_ty2()) == *(fun2.get_ty2());
            result = (fst_eq && snd_eq);
        }
    }
    return (result);
}

/*
 *   float
 *      |
 *      |
 *     int  bool tclock
 *       \   |   /
 *        \  |  /
 *         tunknown
 *
 *    t0 <= t0' /\ t1' <= t1 => t0' -> t1' <= t0 -> t1
 *
 *    not included, for convenience: (t0 -> t1) <= tunknown
 */
bool operator<=(const Ty& ty1, const Ty& ty2) {
    bool result = false;
    if (ty1.is_basic()) {
        if (ty2.is_basic()) {
            BasicTy basic1 = ty1.to_basic();
            BasicTy basic2 = ty2.to_basic();
            //equal
            bool equal = (basic1.get_type() == basic2.get_type());
            //or ty1 is tunkwnown
            bool compared_to_ns = (basic1.get_type() == Type::tunknown);
            //or int convertible to float.
            bool int_to_float = (basic1.get_type() == Type::tint)
                    && (basic2.get_type() == Type::tfloat);
            result = equal || compared_to_ns || int_to_float;
        }
    } else if (ty1.is_fun()) {
        if (ty2.is_fun()) {
            FunTy fun1 = ty1.to_fun();
            FunTy fun2 = ty2.to_fun();
            bool fst_eq = *(fun2.get_ty1()) <= *(fun1.get_ty1());
            bool snd_eq = *(fun1.get_ty2()) <= *(fun2.get_ty2());
            result = (fst_eq && snd_eq);
        }
    }
    return (result);
}

bool operator<(const Ty& ty1, const Ty& ty2) {
    return (ty1 <= ty2) && !(ty1 == ty2);
}


