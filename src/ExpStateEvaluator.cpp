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
    case ExpOp::eq: return "equal";
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
    case ExpOp::neq: return "!=";
    case ExpOp::nott: return "nequal";
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
ExpTranslatorVisitor::visit(shared_ptr<IConst> node) {
    exprStr = std::to_string(node->get_value());
}

inline void
ExpTranslatorVisitor::visit(shared_ptr<BConst> node) {
    exprStr = std::to_string(node->get_value());
}

inline void
ExpTranslatorVisitor::visit(shared_ptr<LocExp> node) {
    const std::string &id = node->get_exp_location()->get_identifier();
    exprStr = id;
}

inline void
ExpTranslatorVisitor::visit(shared_ptr<UnOpExp> node) {
    node->get_argument()->accept(*this);
    std::string arg1 = exprStr;
    std::stringstream ss;
    ss << exprtk_name(node->get_operator());
    ss << "(" << arg1 << ")";
    exprStr = ss.str();
}

inline void
ExpTranslatorVisitor::visit(shared_ptr<BinOpExp> node) {
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
ExpTranslatorVisitor::get_expression(symbol_table_t& table) {
    this->expr.register_symbol_table(table);
    ExpTranslatorVisitor::parser.compile(exprStr, this->expr);
    return (this->expr);
}

exprtk::parser<NUMTYPE> ExpTranslatorVisitor::parser;

//  Definitions of ExpNamesCollector:
inline void
ExpNameCollector::visit(shared_ptr<LocExp> node) {
   const std::string &id = node->get_exp_location()->get_identifier();
   if (std::find(names.begin(), names.end(), id) == names.end()) {
       names.push_back(id);
   }
}

inline void
ExpNameCollector::visit(shared_ptr<BinOpExp> node) {
    node->get_first_argument()->accept(*this);
    node->get_second_argument()->accept(*this);
}

inline void
ExpNameCollector::visit(shared_ptr<UnOpExp> node) {
    node->get_argument()->accept(*this);
}

// Definitions of ExpInternalState

inline void
ExpInternalState::add_variables(const ExpContainer &astVec) {
    for (shared_ptr<Exp> ast : astVec) {
        ExpNameCollector visitor;
        ast->accept(visitor);
        NameContainer astNames = visitor.get_names();
        for (const std::string &name : astNames) {
           auto it = std::find(varNames.begin(), varNames.end(), name);
           if (it == varNames.end()) {
               varNames.push_back(name);
           }
        }
    }
    varPos.resize(varNames.size());
    varValues.resize(varNames.size(),0);
}

inline void
ExpInternalState::project_positions(const State<STATE_INTERNAL_TYPE> &state) {
    for (size_t i = 0; i < varNames.size(); i++) {
       varPos[i] = state.position_of_var(varNames[i]);
    }
}

inline void
ExpInternalState::project_positions(const PositionsMap &posMap) {
    for (size_t i = 0; i < varNames.size(); i++) {
       varPos[i] = posMap.at(varNames[i]);
    }
}

inline void
ExpInternalState::project_values(const State<STATE_INTERNAL_TYPE> &state) {
    for (size_t i = 0; i < varNames.size(); i++) {
        varValues[i] = state[varPos[i]]->val();
    }
}

inline void
ExpInternalState::project_values(const StateInstance &state) {
    for (size_t i = 0; i < varNames.size(); i++) {
        varValues[i] = state[varPos[i]];
    }
}

inline size_t
ExpInternalState::get_local_position(const std::string &name) {
    size_t i = 0;
    while (i < varNames.size() && varNames[i] != name) {
        i++;
    }
    assert(i < varNames.size());
    return (i);
}

// Definitions of ExpTableFiller
inline void
ExpTableFiller::visit(shared_ptr<BinOpExp> node) {
    node->get_first_argument()->accept(*this);
    node->get_second_argument()->accept(*this);
}

inline void
ExpTableFiller::visit(shared_ptr<UnOpExp> node) {
    node->get_argument()->accept(*this);
}

inline void
ExpTableFiller::visit(shared_ptr<LocExp> node) {
    const std::string &id = node->get_exp_location()->get_identifier();
    size_t pos = expState.get_local_position(id);
    ///@todo try to avoid references to vector elements, since
    /// pointers to them may change during reallocation:
    table.add_variable(id, expState.varValues[pos]);
}

// Definitions of ExpStateEvaluator

ExpStateEvaluator::ExpStateEvaluator(const ExpContainer& astVec)
    : astVec {astVec} {

    expState = make_shared<ExpInternalState>();
    exprVec.resize(astVec.size());
    expState->add_variables(astVec);

    // fill the symbol table before parsing
    ExpTableFiller filler (table, *expState);
    for (size_t i = 0; i < astVec.size(); i++) {
        astVec[i]->accept(filler);
    }

    // Translate all the ast expressions
    for (size_t i = 0; i < astVec.size(); i++) {
        ExpTranslatorVisitor visitor;
        astVec[i]->accept(visitor);
        //parse string and produce a expression
        exprVec[i] = visitor.get_expression(table);
    }
}

void ExpStateEvaluator::prepare(const State<STATE_INTERNAL_TYPE> &state) {
    expState->project_positions(state);
}

void ExpStateEvaluator::prepare(const PositionsMap& posMap) {
    expState->project_positions(posMap);
}

STYPE
ExpStateEvaluator::eval(const State<STATE_INTERNAL_TYPE> &state) const {
    return eval_all(state).at(0);
}

STYPE
ExpStateEvaluator::eval(const StateInstance &state) const {
    return eval_all(state).at(0);
}

std::vector<STYPE>
ExpStateEvaluator::eval_all(const StateInstance& state) const {
    expState->project_values(state);
    std::vector<STYPE> results;
    results.resize(exprVec.size());
    for (size_t i = 0; i < astVec.size(); i++) {
        results[i] = (STYPE) exprVec[i].value();
    }
    return (results);
}

std::vector<STYPE>
ExpStateEvaluator::eval_all(const State<STYPE>& state) const {
    expState->project_values(state);
    std::vector<STYPE> results;
    results.resize(exprVec.size());
    for (size_t i = 0; i < astVec.size(); i++) {
        results[i] = exprVec[i].value();
    }
    return (results);
}

} //namespace fig
