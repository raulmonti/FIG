#include "ExpReductor.h"

namespace {
shared_ptr<Exp> eval_if_possible(shared_ptr<Exp> exp) {
    ExpEvaluator ev;
    exp->accept(ev);
    return (ev.has_errors() ? exp : ev.value_to_ast());
}
} //namespace

void ExpReductor::visit(shared_ptr<IConst> node) {
   reduced_exp = node;
}

void ExpReductor::visit(shared_ptr<BConst> node) {
    reduced_exp = node;
}

void ExpReductor::visit(shared_ptr<FConst> node) {
    reduced_exp = node;
}

void ExpReductor::visit(shared_ptr<LocExp> node) {
    reduced_exp = ::eval_if_possible(node);
}

void ExpReductor::visit(shared_ptr<OpExp> exp) {
    exp->left->accept(*this);
    shared_ptr<Exp> left_reduced = eval_if_possible(reduced_exp);
    shared_ptr<Exp> right_reduced = nullptr;
    if (exp->arity == Arity::two) {
        exp->right->accept(*this);
        right_reduced = eval_if_possible(reduced_exp);
    }
    //build a copy of exp but with the operands reduced
    reduced_exp = make_shared<OpExp>(exp->arity, exp->bop,
                                     left_reduced, right_reduced);
    reduced_exp->type = exp->type;
    //may be this new expression could still be reduced
    reduced_exp = eval_if_possible(reduced_exp);
}
