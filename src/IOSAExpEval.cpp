/* Leonardo Rodríguez */

#include "IOSAExpEval.h"
#include "Operators.h"
#include "FigException.h"

namespace iosa {

void Evaluator::visit(shared_ptr<BConst> node) {
    value = node->get_value() ? 1 : 0;
}

void Evaluator::visit(shared_ptr<IConst> node) {
    value = node->get_value();
}

void Evaluator::visit(shared_ptr<FConst>) {
    throw_FigException("Float constant not supported in this stage.");
}

void Evaluator::visit(shared_ptr<LocExp> exp) {
    shared_ptr<Location> loc = exp->get_exp_location();
    string id = loc->get_identifier();
    value = state->get_variable_value(id);
}

void Evaluator::visit(shared_ptr<UnOpExp> exp) {
    if (!exp->has_inferred_type()) {
        throw_FigException("ExpEvaluator called without typechecking.");
    }
    UnaryOpTy inferred = exp->get_inferred_type();
    exp->get_argument()->accept(*this);
    state_value_t value_arg = value;
    //only operators with a boolear or int result.
    if (Unary::fi == inferred) {
        auto f = Unary::get_fi(exp->get_operator());
        value = f (value_arg);
    } else if (Unary::ii == inferred) {
        auto f = Unary::get_ii(exp->get_operator());
        value = f (value_arg);
    } else if (Unary::bb == inferred) {
        auto f = Unary::get_bb(exp->get_operator());
        value = f (value_arg) ? 1 : 0;
    } else {
        throw_FigException("Operator unsupported type.");
    }
}

void Evaluator::visit(shared_ptr<BinOpExp> exp) {
    exp->get_first_argument()->accept(*this);
    state_value_t val_left = value;
    exp->get_second_argument()->accept(*this);
    state_value_t val_right = value;
    BinaryOpTy inferred = exp->get_inferred_type();
    if (Binary::iii == inferred) {
        auto f = Binary::get_iii(exp->get_operator());
        value = f (val_left, val_right);
    } else if (Binary::iib == inferred) {
        auto f = Binary::get_iib(exp->get_operator());
        value = f (val_left, val_right) ? 1 : 0;
    } else if (Binary::ffb == inferred) {
        auto f = Binary::get_ffb(exp->get_operator());
        value = f (val_left, val_right) ? 1 : 0;
    } else if (Binary::bbb == inferred) {
        auto f = Binary::get_bbb(exp->get_operator());
        value = f (val_left, val_right);
    } else {
        throw_FigException("Operator unsupported type");
    }
}

} //
