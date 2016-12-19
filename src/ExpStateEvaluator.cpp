#include "ExpStateEvaluator.h"
#include "ExpStringBuilder.h"
#include <functional>
#include <cmath>
#include <FigException.h>

namespace {
///@todo Unify with Operator.cpp
inline std::function<int (int, int)> binary_fun(ExpOp op) {
    switch(op) {
    case ExpOp::plus: return [] (int x, int y) -> int {
            return (x + y);
        };
    case ExpOp::times: return [] (int x, int y) -> int {
            return (x * y);
        } ;
    case ExpOp::minus: return [] (int x, int y) -> int {
            return (x - y);
        };
    case ExpOp::div: return [] (int x, int y) -> int {
            return (x / y);
        };
    case ExpOp::mod: return [] (int x, int y) -> int {
            return (x % y);
        };
    case ExpOp::andd: return [] (int x, int y) -> int {
            return (x && y);
        };
    case ExpOp::implies: return [] (int x, int y) -> int {
            return (!x || y);
        };
    case ExpOp::orr: return [] (int x, int y) -> int {
            return (x || y);
        };
    case ExpOp::eq: return [] (int x, int y) -> int {
            return (x == y);
        };
    case ExpOp::neq: return [] (int x, int y) -> int {
            return (x != y);
        };
    case ExpOp::lt: return [] (int x, int y) -> int {
            return (x < y);
        };
    case ExpOp::gt: return [] (int x, int y) -> int {
            return (x > y);
        };
    case ExpOp::le: return [] (int x, int y) -> int {
            return (x <= y);
        };
    case ExpOp::ge: return [] (int x, int y) -> int {
            return (x >= y);
        };
    case ExpOp::min: return [] (int x, int y) -> int {
            return (std::min(x, y));
        };
    case ExpOp::max: return [] (int x, int y) -> int {
            return (std::max(x, y));
        };
    case ExpOp::pow: return [] (int x, int y) -> int {
            return (std::pow(x, y));
        };
    default: throw_FigException("operator not supported on simulation");
    }
}

inline std::function<int (int)> unary_fun(ExpOp op) {
    switch(op) {
    case ExpOp::nott: return [] (int x) -> int {
            return (!x);
        };
    case ExpOp::minus: return [] (int x) -> int {
            return (-x);
        };
    case ExpOp::abs: return [] (int x) -> int {
            return (std::abs(x));
        };
    default: throw_FigException("invalid expression operator"); break;
    }
}

} //namespace

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
    return eval_all(state).at(0);
}

STYPE
ExpStateEvaluator::eval(const StateInstance &state) const {
    return eval_all(state).at(0);
}

std::vector<STYPE> ExpStateEvaluator::eval_all(const StateInstance& state)
const {
    for (size_t i = 0; i < varNum; i++) {
        stateValues[i] = state[statePositions[i]];
    }
    std::vector<STYPE> results;
    results.resize(expVec.size());
    size_t i = 0;
    for (shared_ptr<Exp> exp : expVec) {
        EvalVisitor eval (stateValues, positionOf);
        exp->accept(eval);
        results[i] = eval.get_value();
        i++;
    }
    return (results);
}


std::vector<STYPE> ExpStateEvaluator::eval_all(const State<STYPE>& state)
const {
    for (size_t i = 0; i < varNum; i++) {
        stateValues[i] = state[statePositions[i]]->val();
    }
    std::vector<STYPE> results;
    results.resize(expVec.size());
    size_t i = 0;
    for (shared_ptr<Exp> exp : expVec) {

        ExpStringBuilder builder(CompositeModuleScope::get_instance());
        exp->accept(builder);
        std::cout << "EXP:" << builder.str() << std::endl;
        std::cout << "STATE:" << std::endl;
        state.print_out(std::cout, true);
        for (STYPE x : stateValues) {
            std::cout << "VAL=" << x << ",";
        }
        std::cout << std::endl;

        EvalVisitor eval (stateValues, positionOf);
        exp->accept(eval);
        results[i] = eval.get_value();

        std::cout << "RESULT: " << results[i] << std::endl;

        i++;
    }
    return (results);
}

void
ExpStateEvaluator::EvalVisitor::visit(shared_ptr<BConst> node) {
    value = node->get_value() ? 1 : 0;
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
    value = ::unary_fun(node->get_operator())(arg);
}

void
ExpStateEvaluator::EvalVisitor::visit(shared_ptr<BinOpExp> node) {
    node->get_first_argument()->accept(*this);
    STYPE arg1 = value;
    node->get_second_argument()->accept(*this);
    STYPE arg2 = value;
    value = ::binary_fun(node->get_operator())(arg1, arg2);
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
