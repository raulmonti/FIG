#include "ExpReductor.h"

shared_ptr<Exp> ExpReductor::eval_if_possible(shared_ptr<Exp> exp) {
    ExpEvaluator ev(this->scope);
    exp->accept(ev);
    return (ev.has_errors() ? exp : ev.value_to_ast());
}

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
    reduced_exp = eval_if_possible(node);
}

void ExpReductor::visit(shared_ptr<BinOpExp> exp) {
    exp->get_first_argument()->accept(*this);
    shared_ptr<Exp> left_reduced = eval_if_possible(reduced_exp);
    exp->get_second_argument()->accept(*this);
    shared_ptr<Exp> right_reduced = eval_if_possible(reduced_exp);
    //build a copy of exp but with the operands reduced
    reduced_exp =
            make_shared<BinOpExp>(exp->get_operator(),
                                  left_reduced, right_reduced);
    reduced_exp->set_type(exp->get_type());
    //may be this new expression could still be reduced
    reduced_exp = eval_if_possible(reduced_exp);
}

void ExpReductor::visit(shared_ptr<UnOpExp> exp) {
    exp->get_argument()->accept(*this);
    shared_ptr<Exp> reduced = eval_if_possible(reduced_exp);
    //build a copy of exp but with the operands reduced
    reduced_exp =
            make_shared<UnOpExp>(exp->get_operator(), reduced);
    reduced_exp->set_type(exp->get_type());
    //may be this new expression could still be reduced
    reduced_exp = eval_if_possible(reduced_exp);
}
