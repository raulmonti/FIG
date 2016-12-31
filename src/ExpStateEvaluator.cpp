#include "ExpStateEvaluator.h"
#include "ExpStringBuilder.h"
#include <functional>
#include <cmath>
#include <FigException.h>
#include <sstream>
#include <string.h>

namespace {
/*
std::string exp_str(shared_ptr<Exp> exp) {
    ExpStringBuilder visitor (CompositeModuleScope::get_instance());
    exp->accept(visitor);
    return (visitor.str());
}

int show_symbol_table(const symbol_table_t &table) {
    std::vector<std::pair<std::string, NUMTYPE>> v;
    table.get_variable_list(v);
    for (auto p : v) {
        std::cout << p.first << " -> "
                  << static_cast<int>(p.second) << std::endl;
    }
    return (0);
}
*/
}

namespace fig {

// Definitions of ExpTranslatorVisitor:

inline std::string
ExpTranslatorVisitor::exprtk_name(ExpOp op) {
    switch (op) {
    case ExpOp::abs: return "abs";
    case ExpOp::andd: return "&";
    case ExpOp::ceil: return "ceil";
    case ExpOp::div: return "/";
    case ExpOp::eq: return "equal"; //don't use ==, is floating point equality.
    case ExpOp::floor: return "floor";
    case ExpOp::ge: return ">=";
    case ExpOp::gt: return ">";
    case ExpOp::le: return "<=";
    case ExpOp::log: return "log";
    case ExpOp::lt: return "<";
    case ExpOp::max: return "max";
    case ExpOp::min: return "min";
    case ExpOp::minus: return "-";
    case ExpOp::mod: return "%";
    case ExpOp::neq: return "nequal";
    case ExpOp::nott: return "not";
    case ExpOp::orr: return "|";
    case ExpOp::plus: return "+";
    case ExpOp::pow: return "pow";
    case ExpOp::sgn: return "sgn";
    case ExpOp::times: return "*";
    case ExpOp::implies:
    default: throw_FigException("Exprtk not supported operator");
    }
}

inline OpKind
ExpTranslatorVisitor::exprtk_kind(ExpOp op) {
    switch (op) {
    case ExpOp::abs: return OpKind::FUN;
    case ExpOp::andd:return OpKind::OP;
    case ExpOp::ceil:return OpKind::FUN;
    case ExpOp::div: return OpKind::OP;
    case ExpOp::eq: return OpKind::FUN;
    case ExpOp::floor:return OpKind::FUN;
    case ExpOp::ge:return OpKind::OP;
    case ExpOp::gt:return OpKind::OP;
    case ExpOp::implies:return OpKind::OP;
    case ExpOp::le:return OpKind::OP;
    case ExpOp::log:return OpKind::FUN;
    case ExpOp::lt:return OpKind::OP;
    case ExpOp::max:return OpKind::FUN;
    case ExpOp::min:return OpKind::FUN;
    case ExpOp::minus:return OpKind::OP;
    case ExpOp::mod:return OpKind::OP;
    case ExpOp::neq:return OpKind::FUN;
    case ExpOp::nott:return OpKind::FUN;
    case ExpOp::orr:return OpKind::OP;
    case ExpOp::plus:return OpKind::OP;
    case ExpOp::pow:return OpKind::FUN;
    case ExpOp::sgn:return OpKind::FUN;
    case ExpOp::times:return OpKind::OP;
    default:
        throw_FigException("ExprTk unsupported operator");
    }
}

inline void
ExpTranslatorVisitor::visit(shared_ptr<IConst> node) noexcept {
    exprStr = std::to_string(node->get_value());
}

inline void
ExpTranslatorVisitor::visit(shared_ptr<BConst> node) noexcept {
    exprStr = std::to_string(node->get_value());
}

inline void
ExpTranslatorVisitor::visit(shared_ptr<LocExp> node) noexcept {
    shared_ptr<Location> loc = node->get_exp_location();
    if (loc->is_array()) {
        loc->to_array_position()->get_index()->accept(*this);
        exprStr = loc->get_identifier() + "[" + exprStr + "]";
    } else {
        const std::string &id = loc->get_identifier();
        exprStr = id;
    }
}

inline void
ExpTranslatorVisitor::visit(shared_ptr<UnOpExp> node) noexcept {
    node->get_argument()->accept(*this);
    std::string arg1 = exprStr;
    std::stringstream ss;
    ss << exprtk_name(node->get_operator());
    ss << "(" << arg1 << ")";
    exprStr = ss.str();
}

inline void
ExpTranslatorVisitor::visit(shared_ptr<BinOpExp> node) noexcept {
    node->get_first_argument()->accept(*this);
    std::string arg1 = exprStr;
    node->get_second_argument()->accept(*this);
    std::string arg2 = exprStr;
    std::stringstream ss;
    if (exprtk_kind(node->get_operator()) == OpKind::FUN) {
        ss << exprtk_name(node->get_operator());
        ss << "("  << arg1 << "," << arg2 << ")";
    } else {
        ss << "(" << arg1 << ")";
        ss << exprtk_name(node->get_operator());
        ss << "(" << arg2 << ")";
    }
    exprStr = ss.str();
}

inline expression_t
ExpTranslatorVisitor::get_expression(symbol_table_t& table) noexcept {
    this->expr.register_symbol_table(table);
    ExpTranslatorVisitor::parser.compile(exprStr, this->expr);
    return (this->expr);
}

inline std::string
ExpTranslatorVisitor::get_string() const noexcept {
    return (this->exprStr);
}

exprtk::parser<NUMTYPE> ExpTranslatorVisitor::parser;

// Definitions of ExpStateEvaluator

ExpStateEvaluator::ExpStateEvaluator(const ExpContainer& astVec) noexcept
    : astVec {astVec}, expState {astVec} {
    numExp = astVec.size();
    exprVec.resize(numExp);
    expStrings.resize(numExp);
    expState.fill_symbol_table(table);
    // Translate all the ast expressions
    for (size_t i = 0; i < numExp; i++) {
        ExpTranslatorVisitor visitor;
        astVec[i]->accept(visitor);
        //parse string and produce a expression
        exprVec[i] = visitor.get_expression(table);
        expStrings[i] = visitor.get_string();
    }
}

ExpStateEvaluator::ExpStateEvaluator(const ExpStateEvaluator &that) noexcept
    : astVec {that.astVec}, expState {that.expState} {
    numExp = that.numExp;
    prepared = that.prepared;
    //expression and tables are shallow copied by default, let's create new ones
    //new symbol table must refer to the new vector of values.
    //@todo dig up why is neccesary to have a copy constructor of this class.
    exprVec.resize(numExp);
    expStrings = that.expStrings;
    std::vector<std::pair<std::string, NUMTYPE>> v;
    table.clear();
    expState.fill_symbol_table(table);
    for (size_t i = 0; i < numExp; i++) {
        exprVec[i].register_symbol_table(table);
        ExpTranslatorVisitor::parser.compile(that.expStrings[i], exprVec[i]);
    }
}

void ExpStateEvaluator::prepare(const State<STATE_INTERNAL_TYPE> &state)
noexcept {
    expState.project_positions(state);
    prepared = true;
}

void ExpStateEvaluator::prepare(const PositionsMap& posMap) noexcept {
    expState.project_positions(posMap);
    prepared = true;
}

STYPE
ExpStateEvaluator::eval(const State<STATE_INTERNAL_TYPE> &state) const
noexcept {
    return eval_all(state)[0];
}

STYPE
ExpStateEvaluator::eval(const StateInstance &state) const noexcept {
    return eval_all(state)[0];
}

std::vector<STYPE>
ExpStateEvaluator::eval_all(const StateInstance& state) const noexcept {
    expState.project_values(state);
    std::vector<STYPE> results;
    results.resize(numExp);
    for (size_t i = 0; i < numExp; i++) {
        results[i] = (STYPE) exprVec[i].value();
    }
    return (results);
}

std::vector<STYPE>
ExpStateEvaluator::eval_all(const State<STYPE>& state) const noexcept {
    expState.project_values(state);
    std::vector<STYPE> results;
    results.resize(numExp);
    for (size_t i = 0; i < numExp; i++) {
        results[i] = exprVec[i].value();
    }
    return (results);
}

} //namespace fig
