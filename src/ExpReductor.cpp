#include "ExpReductor.h"

const vector<ExpOp> ExpReductor::not_supported_op =
{ //only supported if the expression is reducible at compilation time
    ExpOp::floor,
    ExpOp::ceil,
    ExpOp::log,
    ExpOp::pow,
    ExpOp::sgn,
    ExpOp::min,
    ExpOp::max,
    ExpOp::abs,
    ExpOp::implies
};

bool ExpReductor::is_not_supported_op(ExpOp op) {
    auto same = [op] (const ExpOp &op1) {
        return (op == op1);
    };
    const auto &vec = not_supported_op;
    auto res = std::find_if(vec.cbegin(), vec.cend(), same);
    return (res != vec.cend());
}

shared_ptr<Exp> ExpReductor::eval_if_possible(shared_ptr<Exp> exp) {
    ExpEvaluator ev(this->scope);
    exp->accept(ev);
    return (ev.has_errors() ? exp : ev.value_to_ast());
}

void ExpReductor::visit(shared_ptr<IConst> node) {
    assert(node->get_type() != Type::tunknown);
    reduced_exp = node;
}

void ExpReductor::visit(shared_ptr<BConst> node) {
    assert(node->get_type() != Type::tunknown);
    reduced_exp = node;
}

void ExpReductor::visit(shared_ptr<FConst> node) {
    assert(node->get_type() != Type::tunknown);
    reduced_exp = node;
}

void ExpReductor::visit(shared_ptr<LocExp> node) {
    assert(node->get_type() != Type::tunknown);
    shared_ptr<Location> loc = node->get_exp_location();
    if (!loc->is_array()) {
        reduced_exp = eval_if_possible(node);
        return;
    }
    //is array
    assert (scope != nullptr);
    shared_ptr<ArrayPosition> ap = loc->to_array_position();
    //reduce index of array position
    ap->get_index()->accept(*this);
    shared_ptr<Exp> reduced_index = eval_if_possible(reduced_exp);
    ap->set_index(reduced_index);
    //for convenience save the declaration
    const std::string& id = ap->get_identifier();
    shared_ptr<Decl> decl = scope->find_identifier(id);
    assert(decl != nullptr);
    assert(decl->to_array() != nullptr);
    ap->set_decl(decl->to_array());
    reduced_exp = eval_if_possible(node);
}

void ExpReductor::visit(shared_ptr<BinOpExp> exp) {
    assert(exp->get_type() != Type::tunknown);
    exp->get_first_argument()->accept(*this);
    shared_ptr<Exp> left_reduced = eval_if_possible(reduced_exp);
    exp->get_second_argument()->accept(*this);
    shared_ptr<Exp> right_reduced = eval_if_possible(reduced_exp);
    //build a copy of exp but with the operands reduced
    shared_ptr<BinOpExp> new_exp =
            make_shared<BinOpExp>(exp->get_operator(),
                                  left_reduced, right_reduced);
    new_exp->set_type(exp->get_type());
    new_exp->set_inferred_type(exp->get_inferred_type());
    new_exp->set_callable(exp->get_callable());
    assert(new_exp->get_inferred_type() == exp->get_inferred_type());
    //may be this new expression could still be reduced
    reduced_exp = eval_if_possible(new_exp);
    if (reduced_exp->is_binary_operator()) {
        //couldn't reduce the operator.
        shared_ptr<BinOpExp> exp
                = std::static_pointer_cast<BinOpExp>(reduced_exp);
        ExpOp op = exp->get_operator();
        if (is_not_supported_op(op)) {
            put_error("Operator \"" + Operator::operator_string(op)
                      + "\" must be reducible at compilation time");
        }
    }
}

void ExpReductor::visit(shared_ptr<UnOpExp> exp) {
    assert(exp->get_type() != Type::tunknown);
    exp->get_argument()->accept(*this);
    shared_ptr<Exp> reduced = eval_if_possible(reduced_exp);
    //build a copy of exp but with the operands reduced
    shared_ptr<UnOpExp> new_exp =
            make_shared<UnOpExp>(exp->get_operator(), reduced);
    new_exp->set_type(exp->get_type());
    new_exp->set_inferred_type(exp->get_inferred_type());
    assert(new_exp->get_inferred_type() == exp->get_inferred_type());
    //may be this new expression could still be reduced
    reduced_exp = eval_if_possible(new_exp);
}
