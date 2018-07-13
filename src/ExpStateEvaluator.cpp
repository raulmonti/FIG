/* Leonardo Rodríguez */

#include "ExpStateEvaluator.h"
#include "string_utils.h"
#include <functional>
#include <cmath>
#include <FigException.h>
#include <sstream>
#include <string.h>

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
	case ExpOp::neq: return "not_equal";  // documentation says "nequal" but exprtk code has "not_equal" !!!
	case ExpOp::nott: return "not";
    case ExpOp::orr: return "|";
    case ExpOp::plus: return "+";
    case ExpOp::pow: return "pow";
    case ExpOp::sgn: return "sgn";
    case ExpOp::times: return "*";
        //custom functions
	case ExpOp::fsteq: return "fsteq";
    case ExpOp::lsteq: return "lsteq";
	case ExpOp::rndeq: return "rndeq";
    case ExpOp::minfrom: return "minfrom";
    case ExpOp::maxfrom: return "maxfrom";
    case ExpOp::sumfrom: return "sumfrom";
	case ExpOp::sumkmax: return "sumkmax";
	case ExpOp::consec: return "consec";
    case ExpOp::broken: return "broken";
    case ExpOp::fstexclude: return "fstexclude";
    case ExpOp::implies: return "implies";
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
    case ExpOp::implies: return OpKind::FUN;
    case ExpOp::fsteq: return OpKind::FUN;
    case ExpOp::lsteq: return OpKind::FUN;
    case ExpOp::rndeq: return OpKind::FUN;
    case ExpOp::minfrom: return OpKind::FUN;
    case ExpOp::maxfrom: return OpKind::FUN;
    case ExpOp::sumfrom: return OpKind::FUN;
	case ExpOp::sumkmax: return OpKind::FUN;
	case ExpOp::consec: return OpKind::FUN;
    case ExpOp::broken: return OpKind::FUN;
    case ExpOp::fstexclude: return OpKind::FUN;
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
    if (loc->is_array_position()) {
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

inline std::string
ExpTranslatorVisitor::get_string() const noexcept {
    return (this->exprStr);
}

// Definitions of ExpStateEvaluator

ExpStateEvaluator::ExpStateEvaluator(const ExpContainer& astVector) :
    astVec(astVector),
    numExp(astVector.size()),
    expState(astVector),
    expStrings(numExp),
    prepared(false),
    valuation(numExp)
{
#ifndef NDEBUG
	static exprtk::expression_helper<NTYPE> checkExpr;
#endif
	exprVec.resize(numExp);
	exprtk::parser<NTYPE> parser;
    // Translate all the ast expressions
    for (size_t i = 0; i < numExp; i++) {
        ExpTranslatorVisitor visitor;
		astVec[i]->accept(visitor);
		// Now parse string and produce an expression
		expStrings[i] = visitor.get_string();
		expState.register_expression(exprVec[i]);
        parser.compile(expStrings[i], exprVec[i]);
		assert(exprtk::is_valid<NTYPE>(exprVec[i]));
		assert(!checkExpr.is_null(exprVec[i]) || expStrings[i].length() < 2);
    }
}

ExpStateEvaluator::ExpStateEvaluator(const ExpStateEvaluator &that) :
    astVec(that.astVec),  // is deep copy because contents are shared_ptr<>
    numExp(that.numExp),
    expState(astVec),
    expStrings(that.expStrings),  // default is deep-copy
    prepared(that.prepared),
    valuation(that.valuation)
{
#ifndef NDEBUG
	static exprtk::expression_helper<NTYPE> checkExpr;
#endif
	//new symbol table must refer to the new vector of values.
	//@todo dig up why is neccesary to have a copy constructor of this class.
	//@note just removing the old table of each expression (without reparsing) didn't work
	// but I don't remember why, we should try again to save some time.
	exprVec.resize(numExp);
	exprtk::parser<NTYPE> parser;
	for (size_t i = 0; i < numExp; i++) {
		expState.register_expression(exprVec[i]);
		parser.compile(expStrings[i], exprVec[i]);
		assert(exprtk::is_valid<NTYPE>(exprVec[i]));
		assert(!checkExpr.is_null(exprVec[i]) || expStrings[i].length() < 2);
	}
}


ExpStateEvaluator::ExpStateEvaluator(ExpStateEvaluator&& that) :
    astVec(std::move(that.astVec)),
    numExp(that.numExp),
    expState(std::move(that.expState)),
    exprVec(std::move(that.exprVec)),
    expStrings(std::move(that.expStrings)),
    prepared(that.prepared),
    valuation(std::move(that.valuation)) {}


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
ExpStateEvaluator::eval(const State<STATE_INTERNAL_TYPE> &state) const noexcept {
    expState.project_values(state);
	assert(exprVec.size() == 1ul);  // TODO erase?
	return exprVec[0].value();
}

STYPE
ExpStateEvaluator::eval(const StateInstance &state) const noexcept {
    expState.project_values(state);
	assert(exprVec.size() == 1ul);  // TODO erase?
	return exprVec[0].value();
}


std::vector<STYPE> &
ExpStateEvaluator::eval_all(const StateInstance& state) const noexcept {
    expState.project_values(state);
    for (size_t i = 0; i < numExp; i++) {
        valuation[i] = (STYPE) exprVec[i].value();
    }
    return (valuation);
}

std::vector<STYPE> &
ExpStateEvaluator::eval_all(const State<STYPE>& state) const noexcept {
    expState.project_values(state);
    for (size_t i = 0; i < numExp; i++) {
        valuation[i] = exprVec[i].value();
    }
    return (valuation);
}

} //namespace fig
