#include "ExpStateEvaluator.h"
#include <functional>
#include <Operators.h>
#include <FigException.h>

namespace {
    inline std::function<int (int, int)> get_iii_forced(ExpOp op) {
        auto f = Binary::get_iii(op);
        if (f) {
            return (f);
        }
        auto g = Binary::get_bbb(op);
        if (g) {
            return (g);
        }
        auto h = Binary::get_iib(op);
        if (h) {
            return (h);
        }
        throw_FigException("Could not interpret binary operator");
    }

    inline std::function<int (int)> get_ii_forced(ExpOp op) {
        auto f = Unary::get_ii(op);
        if (f) {
            return (f);
        }
        auto g = Unary::get_bb(op);
        if (g) {
            return (g);
        }
        throw_FigException("Could not interpret binary operator");
    }
}

namespace fig {

void ExpStateEvaluator::prepare(const State<STYPE> &state) {
    for (size_t i = 0; i < varNum; i++) {
        positionOf[varNames[i]] = i;
        statePositions.push_back(state.position_of_var(varNames[i]));
    }
}

void ExpStateEvaluator::prepare(const PositionsMap& posMap) {
    for (size_t i = 0; i < varNum; i++) {
        positionOf[varNames[i]] = i;
        statePositions.push_back(posMap.at(varNames[i]));
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

std::vector<STYPE> ExpStateEvaluator::eval_all(const StateInstance& state)
const {
    for (size_t i = 0; i < varNum; i++) {
        stateValues.push_back(state[statePositions[i]]);
    }
    std::vector<STYPE> results;
    results.reserve(expVec.size());
    for (shared_ptr<Exp> exp : expVec) {
        EvalVisitor eval (stateValues, positionOf);
        exp->accept(eval);
        results.push_back(eval.get_value());
    }
    return (results);
}


std::vector<STYPE> ExpStateEvaluator::eval_all(const State<STYPE>& state)
const {
    for (size_t i = 0; i < varNum; i++) {
        stateValues.push_back(state[statePositions[i]]->val());
    }
    std::vector<STYPE> results;
    results.reserve(expVec.size());
    for (shared_ptr<Exp> exp : expVec) {
        EvalVisitor eval (stateValues, positionOf);
        exp->accept(eval);
        results.push_back(eval.get_value());
    }
    return (results);
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
    value = ::get_ii_forced(node->get_operator())(arg);
}

void
ExpStateEvaluator::EvalVisitor::visit(shared_ptr<BinOpExp> node) {
    node->get_first_argument()->accept(*this);
    STYPE arg1 = value;
    node->get_second_argument()->accept(*this);
    STYPE arg2 = value;
    value = ::get_iii_forced(node->get_operator())(arg1, arg2);
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
