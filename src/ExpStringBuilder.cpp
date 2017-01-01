#include <utility>
#include <sstream>

#include "ExpStringBuilder.h"
#include "FigException.h"
#include "ExpEvaluator.h"
#include "ModelPrinter.h"
#include "ModelTC.h"
#include "Operators.h"

using std::vector;
using std::pair;
using std::string;

// ExpStringBuilder
void ExpStringBuilder::visit(shared_ptr<IConst> node) {
    result = std::to_string(node->get_value());
}

void ExpStringBuilder::visit(shared_ptr<BConst> node) {
    //@todo: support boolean variables directly?
    result = node->get_value() ? "true" : "false";
}

void ExpStringBuilder::visit(shared_ptr<FConst> node) {
    result = std::to_string(node->get_value());
}

void ExpStringBuilder::visit(shared_ptr<LocExp> node) {
    ExpEvaluator eval (scope);
    node->accept(eval);
    if (!eval.has_errors()) {
        result = eval.value_to_string();
        return;
    }
    //not reducible
    shared_ptr<Location> location = node->get_exp_location();
    if (location->is_array()) {
        shared_ptr<ArrayPosition> ap = location->to_array_position();
        ExpEvaluator ev (scope);
        ap->get_index()->accept(ev);
        if (ev.has_errors()) {
            ///@todo we should recover from this.
            throw_FigException("PropertyProjection requires"
                               "reducible index at this stage");
        }
        const std::string pos = location->get_identifier()
                + "[" + std::to_string(ev.get_int()) + "]";
        result = pos;
        names.push_back(pos);
    } else {
        result = node->get_exp_location()->get_identifier();
        names.push_back(result);
    }
}

void ExpStringBuilder::visit(shared_ptr<BinOpExp> exp) {
    string op = ModelPrinter::to_str(exp->get_operator());
    exp->get_first_argument()->accept(*this);
    string left_s  = result;
    exp->get_second_argument()->accept(*this);
    string right_s = result;
    if (Operator::is_infix_operator(exp->get_operator())) {
        ///@bug Here we should add parenthesis!!
        ///@todo Add parenthesis and implement an algorithm
        /// to eliminate redundant ones
        result = left_s  + op + right_s;
    } else {
        result = op + "(" + left_s + "," + right_s + ")";
    }
}

void ExpStringBuilder::visit(shared_ptr<UnOpExp> exp) {
    string op = ModelPrinter::to_str(exp->get_operator());
    exp->get_argument()->accept(*this);
    result = op + "(" + result + ")";
}

string ExpStringBuilder::str() {
    return (result);
}

const vector<string>& ExpStringBuilder::get_names() {
    return (names);
}
