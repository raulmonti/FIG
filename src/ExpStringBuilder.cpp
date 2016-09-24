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
    if (eval.has_errors()) {
      //could not reduce the constant.
        result = node->get_exp_location()->get_identifier();
        names.push_back(result);
    } else {
        result = eval.value_to_string();
    }
}

void ExpStringBuilder::visit(shared_ptr<BinOpExp> exp) {
    string op = ModelPrinter::to_str(exp->get_operator());
    exp->get_first_argument()->accept(*this);
    string left_s  = result;
    exp->get_second_argument()->accept(*this);
    string right_s = result;
    if (Operator::is_infix_operator(exp->get_operator())) {
        result = left_s + op + right_s;
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

pair<string, vector<string>>
ExpStringBuilder::make_conjunction_str(shared_ptr<ModuleScope> scope,
                                       const vector<shared_ptr<Exp>>& expvec) {
    stringstream ss;
    vector<string> names;
    auto it = expvec.begin();
    while (it != expvec.end()) {
        ExpStringBuilder str_b (scope);
        (*it)->accept(str_b);
        ss << "(" << str_b.str() << ")";
        auto &vec = str_b.get_names();
        names.insert(names.end(), vec.begin(), vec.end());
        if (it + 1 != expvec.end()) {
            ss << " & ";
        }
        it++;
    }
    return make_pair(ss.str(), names);
}

