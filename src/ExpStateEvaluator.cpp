#include "ExpStateEvaluator.h"
#include <functional>
#include <Operators.h>

namespace fig {

void ExpStateEvaluator::prepare(const State<STYPE> &state) {
    for (size_t i = 0; i < varNum; i++) {
        positionOf[varNames[i]] = i;
        statePositions[i] = state.position_of_var(varNames[i]);
    }
}

void ExpStateEvaluator::prepare(const PositionsMap& posMap) {
    for (size_t i = 0; i < varNum; i++) {
        positionOf[varNames[i]] = i;
        statePositions[i] = posMap.at(varNames[i]);
    }
}

STYPE
ExpStateEvaluator::eval(const State<STYPE> &state) const {
    for (size_t i = 0; i < varNum; i++) {
        stateValues[i] = state[statePositions[i]]->val();
    }
    EvalVisitor eval(stateValues, positionOf);
    expr->accept(eval);
    return (eval.get_value());
}

STYPE
ExpStateEvaluator::eval(const StateInstance &state) const {
    for (size_t i = 0; i < varNum; i++) {
        stateValues[i] = state[statePositions[i]];
    }
    EvalVisitor eval (stateValues, positionOf);
    expr->accept(eval);
    return (eval.get_value());
}

void
ExpStateEvaluator::EvalVisitor::visit(shared_ptr<BConst> node) {
    value = node->get_value()? 1 : 0;
}

void
ExpStateEvaluator::EvalVisitor::visit(shared_ptr<IConst> node) {
    value = node->get_value();
}

void
ExpStateEvaluator::EvalVisitor::visit(shared_ptr<FConst> node) {
    (void) node;
    throw_FigException("Attempt to evaluate a float during simulation");
}

void
ExpStateEvaluator::EvalVisitor::visit(shared_ptr<UnOpExp> node) {
    node->get_argument()->accept(*this);
    STYPE arg = value;
    value = Unary::get_ii(node->get_operator())(arg);
}

void
ExpStateEvaluator::EvalVisitor::visit(shared_ptr<BinOpExp> node) {
    node->get_first_argument()->accept(*this);
    STYPE arg1 = value;
    node->get_second_argument()->accept(*this);
    STYPE arg2 = value;
    value = Binary::get_iii(node->get_operator())(arg1, arg2);
}

void
ExpStateEvaluator::EvalVisitor::visit(shared_ptr<LocExp> node) {
    const std::string &name = node->get_exp_location()->get_identifier();
    value = values[positionMap.at(name)];
}

STYPE
ExpStateEvaluator::EvalVisitor::get_value() {
    return (value);
}

void
ExpNamesCollector::visit(shared_ptr<BinOpExp> node) {
    node->get_first_argument()->accept(*this);
    node->get_second_argument()->accept(*this);
}

void
ExpNamesCollector::visit(shared_ptr<UnOpExp> node) {
    node->get_argument()->accept(*this);
}

void
ExpNamesCollector::visit(shared_ptr<LocExp> node) {
    names.push_back(node->get_exp_location()->get_identifier());
}

} //namespace fig
