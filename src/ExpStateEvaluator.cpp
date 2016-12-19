#include "ExpStateEvaluator.h"
#include "ExpStringBuilder.h"
#include <functional>
#include <cmath>
#include <FigException.h>

namespace fig {

void ExpStateEvaluator::prepare(const State<STYPE> &state) {
    for (shared_ptr<Exp> exp : expVec) {
        ExpStatePositionSetter visitor (state);
        exp->accept(visitor);
    }
}

void ExpStateEvaluator::prepare(const PositionsMap& posMap) {
    for (shared_ptr<Exp> exp : expVec) {
        ExpMapPositionSetter visitor (posMap);
        exp->accept(visitor);
    }
}

STYPE
ExpStateEvaluator::eval(const State<STYPE> &state) const {
    return eval_all(state).at(0);
}

STYPE
ExpStateEvaluator::eval(const StateInstance &state) const {
    return eval_all(state).at(0);
}

std::vector<STYPE>
ExpStateEvaluator::eval_all(const StateInstance& state) const {
    std::vector<STYPE> results;
    results.resize(expVec.size());
    size_t i = 0;
    for (shared_ptr<Exp> exp : expVec) {
        StateInstanceEvalVisitor visitor (state);
        exp->accept(visitor);
        results[i] = visitor.get_value();
        i++;
    }
    return (results);
}

std::vector<STYPE>
ExpStateEvaluator::eval_all(const State<STYPE>& state) const {
    std::vector<STYPE> results;
    results.resize(expVec.size());
    size_t i = 0;
    for (shared_ptr<Exp> exp : expVec) {
        StateEvalVisitor visitor (state);
        exp->accept(visitor);
        results[i] = visitor.get_value();
        i++;
    }
    return (results);
}

inline void
StateEvalVisitor::visit(shared_ptr<BConst> node) {
    value = node->get_value() ? 1 : 0;
}

inline void
StateEvalVisitor::visit(shared_ptr<IConst> node) {
    value = node->get_value();
}

inline void
StateEvalVisitor::visit(shared_ptr<FConst> node) {
    (void) node;
    throw_FigException("Attempt to evaluate a float during simulation");
}

inline void
StateEvalVisitor::visit(shared_ptr<UnOpExp> node) {
    node->get_argument()->accept(*this);
    STYPE arg = value;
    value = node->get_callable()(arg);
}

inline void
StateEvalVisitor::visit(shared_ptr<BinOpExp> node) {
    node->get_first_argument()->accept(*this);
    STYPE arg1 = value;
    node->get_second_argument()->accept(*this);
    STYPE arg2 = value;
    value = node->get_callable()(arg1, arg2);
}

inline void
StateEvalVisitor::visit(shared_ptr<LocExp> node) {
    value = (*stateptr)[node->get_exp_location()->get_position()]->val();
}

STYPE
inline StateEvalVisitor::get_value() {
    return (value);
}

inline void
StateInstanceEvalVisitor::visit(shared_ptr<BConst> node) {
    value = node->get_value() ? 1 : 0;
}

inline void
StateInstanceEvalVisitor::visit(shared_ptr<IConst> node) {
    value = node->get_value();
}

inline void
StateInstanceEvalVisitor::visit(shared_ptr<FConst> node) {
    (void) node;
    throw_FigException("Attempt to evaluate a float during simulation");
}

inline void
StateInstanceEvalVisitor::visit(shared_ptr<UnOpExp> node) {
    node->get_argument()->accept(*this);
    STYPE arg = value;
    value = node->get_callable()(arg);
}

inline void
StateInstanceEvalVisitor::visit(shared_ptr<BinOpExp> node) {
    node->get_first_argument()->accept(*this);
    STYPE arg1 = value;
    node->get_second_argument()->accept(*this);
    STYPE arg2 = value;
    value = node->get_callable()(arg1, arg2);
}

inline void
StateInstanceEvalVisitor::visit(shared_ptr<LocExp> node) {
    value = (*stateptr)[node->get_exp_location()->get_position()];
}

inline STYPE
StateInstanceEvalVisitor::get_value() {
    return (value);
}

inline void ExpStatePositionSetter::visit(shared_ptr<BinOpExp> node) {
    node->get_first_argument()->accept(*this);
    node->get_second_argument()->accept(*this);
}

inline void ExpStatePositionSetter::visit(shared_ptr<UnOpExp> node) {
    node->get_argument()->accept(*this);
}

inline void ExpStatePositionSetter::visit(shared_ptr<LocExp> node) {
    shared_ptr<Location> loc = node->get_exp_location();
    size_t position = (*stateptr).position_of_var(loc->get_identifier());
    loc->set_position(position);
}

inline void ExpMapPositionSetter::visit(shared_ptr<BinOpExp> node) {
    node->get_first_argument()->accept(*this);
    node->get_second_argument()->accept(*this);
}

inline void ExpMapPositionSetter::visit(shared_ptr<UnOpExp> node) {
    node->get_argument()->accept(*this);
}

inline void ExpMapPositionSetter::visit(shared_ptr<LocExp> node) {
    shared_ptr<Location> loc = node->get_exp_location();
    size_t position = (*posMapPtr).at(loc->get_identifier());
    loc->set_position(position);
}

} //namespace fig
