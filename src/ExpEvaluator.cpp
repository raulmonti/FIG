/* Leonardo Rodríguez */

#include <cassert>
#include <type_traits>
#include "ExpEvaluator.h"
#include "FigException.h"
#include "ModelPrinter.h"

bool ExpEvaluator::was_reduced() {
    return (type != Type::tunknown);
}

bool ExpEvaluator::has_type_int() {
    return (type == Type::tint);
}

bool ExpEvaluator::has_type_bool() {
    return (type == Type::tbool);
}

bool ExpEvaluator::has_type_float() {
    return (type == Type::tfloat);
}

string ExpEvaluator::value_to_string() {
    switch (type) {
    case Type::tint: return std::to_string(value.ivalue);
    case Type::tfloat: return std::to_string(value.fvalue);
    case Type::tbool: return std::string(value.bvalue ? "true" : "false");
    default:
        throw_FigException("Unsupported ground type for values");
    }
}

shared_ptr<Exp> ExpEvaluator::value_to_ast() {
    switch (type) {
    case Type::tint: return make_shared<IConst>(value.ivalue);
    case Type::tfloat: return make_shared<FConst>(value.fvalue);
    case Type::tbool: return make_shared<BConst>(value.bvalue);
    default:
        throw_FigException("Unsupported ground type for values");
    }
}

void ExpEvaluator::mark_not_reducible() {
    type = Type::tunknown;
}

int ExpEvaluator::get_int() {
    assert (type == Type::tint);
    return (value.ivalue);
}

bool ExpEvaluator::get_bool() {
    assert (type == Type::tbool);
    return (value.bvalue);
}

float ExpEvaluator::get_float() {
    assert (type == Type::tfloat);
    return (value.fvalue);
}

void ExpEvaluator::reset() {
    type = Type::tunknown;
}

template<typename T>
std::function<T (T)> unop_as_fun(ExpOp op) {
    switch (op) {
    case ExpOp::minus: return std::negate<T>();
    case ExpOp::nott: return std::logical_not<T>();
    default:
        std::string msg = "Interpretation of " + ModelPrinter::to_str(op)
                + " as a function failed";
        throw_FigException(msg);
    }
}

// Define modulus for integral types only:
// http://stackoverflow.com/questions/23970532/
template<typename T>
std::function<T (T, T)> modulus_(std::true_type) {
    return std::modulus<T>();
}

template<typename T>
std::function<T (T, T)> modulus_(std::false_type) {
    throw_FigException("Modulus undefined for non integral types");
}

template<typename T>
std::function<T (T, T)> ExpEvaluator::bop_as_fun(ExpOp op) {
    switch(op) {
    case ExpOp::plus: return std::plus<T>();
    case ExpOp::times: return std::multiplies<T>();
    case ExpOp::div: return std::divides<T>();
    case ExpOp::minus: return std::minus<T>();
    case ExpOp::mod: return modulus_<T>(std::is_integral<T>());
    case ExpOp::andd: return std::logical_and<T>();
    case ExpOp::orr: return std::logical_or<T>();
    default:
        std::string msg = "Interpretation of " + ModelPrinter::to_str(op)
                + " as a function failed";
        throw_FigException(msg);
    }
}

template<typename T>
std::function<bool (T, T)> ExpEvaluator::bop_as_rel(ExpOp op) {
    switch(op) {
    case ExpOp::lt: return std::less<T>();
    case ExpOp::le: return std::less_equal<T>();
    case ExpOp::gt: return std::greater<T>();
    case ExpOp::ge: return std::greater_equal<T>();
    case ExpOp::eq: return std::equal_to<T>();
    case ExpOp::neq: return [] (const T& x, const T& y) {
            bool temp = std::equal_to<T>() (x, y);
            return std::logical_not<bool>() (temp);
        };
    default:
        std::string msg = "Interpretation of " + ModelPrinter::to_str(op)
                + " as relation failed";
        throw_FigException(msg);
    }
}

void ExpEvaluator::visit(shared_ptr<IConst> iexp) {
    value.ivalue = iexp->value;
    type = Type::tint;
}

void ExpEvaluator::visit(shared_ptr<BConst> bexp) {
    value.bvalue = bexp->value;
    type = Type::tbool;
}

void ExpEvaluator::visit(shared_ptr<FConst> fexp) {
    value.fvalue = fexp->value;
    type = Type::tfloat;
}

void ExpEvaluator::visit(shared_ptr<LocExp> loc) {
    shared_ptr<Location> location = loc->location;
    string &id = location->id;
    if (globals.find(id) != globals.end()) {
        shared_ptr<Decl> decl = globals[id];
        if (location->is_array_position()) {
            // case: id [ indexp ]
            shared_ptr<Exp> indexp = location->index;
            //evaluate the index
            indexp->accept(*this);
            if (was_reduced() && decl->has_array_init()) {
                int index = get_int();
                if (decl->has_single_init()) {
                    //e.g: id [ indexp ] init 4;
                    shared_ptr<Exp> init = decl->inits.at(0);
                    init->accept(*this);
                } else {
                    //e.g: id [ indexp ] = {1, 2, 3}
                    shared_ptr<Exp> valexp = decl->inits.at(index);
                    valexp->accept(*this);
                }
            } else {
                mark_not_reducible();
            }
        } else {
            //case: id
            if (decl->has_single_init()) {
                //e.g: id int init 42
                //take the value of the initialization
                shared_ptr<Exp> init = decl->inits.at(0);
                init->accept(*this);
            }
        }
    }
}

inline void ExpEvaluator::reduce_unary_operator(shared_ptr<OpExp> exp) {
    shared_ptr<Exp> left = exp->left;
    left->accept(*this);
    //convert to float if expects float type
    if (exp->type == Type::tfloat && type == Type::tint) {
        value.fvalue = (float) value.ivalue;
        type = Type::tfloat;
    }
    if (type == Type::tbool) {
        const auto &f = unop_as_fun<bool>(exp->bop);
        value.bvalue = f (value.bvalue);
        type = Type::tbool;
    } else if (type == Type::tint) {
        const auto &f = unop_as_fun<int>(exp->bop);
        value.ivalue = f (value.ivalue);
        type = Type::tint;
    } else if (type == Type::tfloat) {
        const auto &f = unop_as_fun<float>(exp->bop);
        value.fvalue = f (value.fvalue);
        type = Type::tfloat;
    }
}

inline void ExpEvaluator::reduce_binary_operator(shared_ptr<OpExp> exp) {
    shared_ptr<Exp> left = exp->left;
    shared_ptr<Exp> right = exp->right;
    //reduce left argument and store result
    left->accept(*this);
    value_holder_t val_left = value;
    Type type_left = type;
    //reduce right argument and store result
    right->accept(*this);
    value_holder_t val_right = value;
    Type type_right = type;
    //if left or right argument is float
    //and the other is int, then
    //then the other be converted to float
    if (type_left == Type::tfloat && type_right == Type::tint) {
        type_right = Type::tfloat;
        val_right.fvalue = (float) val_right.ivalue;
    }
    if (type_left == Type::tint && type_right == Type::tfloat) {
        type_left = Type::tfloat;
        val_left.fvalue = (float) val_left.ivalue;
    }
    //now we now that both arguments must have the same type
    if (type_left == Type::tunknown ||
            type_right == Type::tunknown || type_left != type_right) {
        mark_not_reducible();
        type_left  = Type::tunknown;
        type_right = Type::tunknown;
    }
    bool expects_boolean = exp->type == Type::tbool;
    // arguments of type int, interpret as binary (int) operator
    if (!expects_boolean && type_left == Type::tint) {
        const auto &f = ExpEvaluator::bop_as_fun<int>(exp->bop);
        value.ivalue = f (val_left.ivalue, val_right.ivalue);
        type = Type::tint;
    } else if (!expects_boolean && type_left == Type::tfloat) {
        const auto &f = ExpEvaluator::bop_as_fun<float>(exp->bop);
        value.fvalue = f (val_left.fvalue, val_right.fvalue);
        type = Type::tfloat;
    } else if (expects_boolean && type_left == Type::tbool) {
        const auto &f = ExpEvaluator::bop_as_fun<bool>(exp->bop);
        value.fvalue = f (val_left.bvalue, val_right.bvalue);
        type = Type::tbool;
    } else if (expects_boolean && type_left == Type::tint) {
        const auto &f = ExpEvaluator::bop_as_rel<int>(exp->bop);
        value.bvalue = f (val_left.ivalue, val_right.ivalue);
        type = Type::tbool;
    } else if (expects_boolean && type_left == Type::tfloat) {
        const auto &f = ExpEvaluator::bop_as_rel<float>(exp->bop);
        value.bvalue = f (val_left.ivalue, val_right.ivalue);
        type = Type::tbool;
    } else {
        throw_FigException("Unsupported evaluation of expression");
    }
}

void ExpEvaluator::visit(shared_ptr<OpExp> exp) {
    if (exp->arity == Arity::one) {
        reduce_unary_operator(exp);
    } else if (exp->arity == Arity::two) {
        reduce_binary_operator(exp);
    }
}
