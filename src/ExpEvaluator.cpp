/* Leonardo Rodríguez */

#include <cassert>
#include <cmath>
#include <type_traits>
#include <utility>

#include "ExpEvaluator.h"
#include "FigException.h"
#include "ModelPrinter.h"
#include "Operators.h"

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

inline void ExpEvaluator::mark_not_reducible() {
    type = Type::tunknown;
}

inline void ExpEvaluator::accept_cond(shared_ptr<Exp> node) {
    if (!has_errors()) {
        node->accept(*this);
    }
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
    if (type == Type::tint) {
        return (value.ivalue);
    } else {
        assert(type == Type::tfloat);
        return (value.fvalue);
    }
}

int ExpEvaluator::get_int_v(value_holder_t value, Type type) {
    assert(type == Type::tint);
    return (value.ivalue);
}

bool ExpEvaluator::get_bool_v(value_holder_t value, Type type) {
    assert(type == Type::tbool);
    return (value.bvalue);
}

float ExpEvaluator::get_float_v(value_holder_t value, Type type) {
    if (type == Type::tint) {
        return (value.ivalue);
    } else {
        assert(type == Type::tfloat);
        return (value.fvalue);
    }
}

void ExpEvaluator::reset() {
    type = Type::tunknown;
}

void ExpEvaluator::visit(shared_ptr<IConst> iexp) {
    value.ivalue = iexp->get_value();
    type = Type::tint;
}

void ExpEvaluator::visit(shared_ptr<BConst> bexp) {
    value.bvalue = bexp->get_value();
    type = Type::tbool;
}

void ExpEvaluator::visit(shared_ptr<FConst> fexp) {
    value.fvalue = fexp->get_value();
    type = Type::tfloat;
}

void ExpEvaluator::visit(shared_ptr<LocExp> loc) {
    const string &id = loc->get_exp_location()->get_identifier();
    shared_ptr<Decl> decl
            = scope != nullptr ? scope->find_identifier(id) : nullptr;
    if (decl == nullptr) {
        //try in global scope
        decl = ModuleScope::find_identifier_global(id);
    }
    if (decl != nullptr && decl->is_constant()) {
        if (decl->has_init()) {
            shared_ptr<Exp> init = decl->to_initialized()->get_init();
            //evaluate the expression associated with the identifier
            init->accept(*this);
            //this could lead to infinite loop if typechecking doesn't
            //check for circular expressions.
        } else {
            put_error("Attempt to evaluate uninitialized constant");
        }
    } else {
        put_error("Location \"" + id + "\" not reducible at compilation time");
    }
}

inline void ExpEvaluator::reduce_unary_operator(shared_ptr<UnOpExp> exp) {
    if (!exp->has_inferred_type()) {
        throw_FigException("Evaluator called without typechecking.");
    }
    UnaryOpTy inferred = exp->get_inferred_type();
    accept_cond(exp->get_argument());
    if (has_errors()) {
        return; //gtfooh
    }
    //inferred type mus be one of the Operator::supported_types or error.
    if (Unary::fi == inferred) {
        auto f = Unary::get_fi(exp->get_operator());
        value.ivalue = f (get_float());
    } else if (Unary::ff == inferred) {
        auto f = Unary::get_ff(exp->get_operator());
        value.fvalue = f (get_float());
    } else if (Unary::ii == inferred) {
        auto f = Unary::get_ii(exp->get_operator());
        value.ivalue = f (get_int());
    } else if (Unary::bb == inferred) {
        auto f = Unary::get_bb(exp->get_operator());
        value.bvalue = f (get_bool());
    } else {
        throw_FigException("Operator unsupported type.");
    }
    type = inferred.get_result_type();
}

inline void ExpEvaluator::reduce_binary_operator(shared_ptr<BinOpExp> exp) {
    //reduce left argument and store result
    accept_cond(exp->get_first_argument());
    if (has_errors()) {
        return;//ntdh
    }
    value_holder_t val_left = value;
    Type type_left = type;
    //reduce right argument and store result
    accept_cond(exp->get_second_argument());
    if (has_errors()) {
        return;//ntdh,sofab
    }
    value_holder_t val_right = value;
    Type type_right = type;
    BinaryOpTy inferred = exp->get_inferred_type();
    if (Binary::iii == inferred) {
        auto f = Binary::get_iii(exp->get_operator());
        int x  = get_int_v(val_left, type_left);
        int y  = get_int_v(val_right, type_right);
        value.ivalue = f (x, y);
    } else if (Binary::iff == inferred) {
        auto f = Binary::get_iff(exp->get_operator());
        int x = get_int_v(val_left, type_left);
        float y = get_float_v(val_right, type_right);
        value.fvalue = f (x, y);
    } else if (Binary::iib == inferred) {
        auto f = Binary::get_iib(exp->get_operator());
        int x = get_int_v(val_left, type_left);
        int y = get_int_v(val_right, type_right);
        value.bvalue = f (x, y);
    } else if (Binary::fff == inferred) {
        auto f = Binary::get_fff(exp->get_operator());
        float x = get_float_v(val_left, type_left);
        float y = get_float_v(val_right, type_right);
        value.fvalue = f (x, y);
    } else if (Binary::ffb == inferred) {
        auto f = Binary::get_ffb(exp->get_operator());
        float x = get_float_v(val_left, type_left);
        float y = get_float_v(val_right, type_right);
        value.bvalue = f (x, y);
    } else if (Binary::fif == inferred) {
        auto f = Binary::get_fif(exp->get_operator());
        float x = get_float_v(val_left, type_left);
        int y = get_int_v(val_right, type_right);
        value.fvalue = f (x, y);
    } else if (Binary::bbb == inferred) {
        auto f = Binary::get_bbb(exp->get_operator());
        bool x = get_bool_v(val_left, type_left);
        bool y = get_bool_v(val_right, type_right);
        value.bvalue = f (x, y);
    } else {
        throw_FigException("Operator unsupported type");
    }
    type = inferred.get_result_type();
}

void ExpEvaluator::visit(shared_ptr<BinOpExp> exp) {
    reduce_binary_operator(exp);
}

void ExpEvaluator::visit(shared_ptr<UnOpExp> exp) {
    reduce_unary_operator(exp);
}
