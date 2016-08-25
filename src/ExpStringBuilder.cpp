#include <utility>
#include <sstream>

#include "ExpStringBuilder.h"
#include "FigException.h"
#include "ExpEvaluator.h"
#include "ModelPrinter.h"
#include "ModelTC.h"

using std::vector;
using std::pair;
using std::string;

// ExpStringBuilder
void ExpStringBuilder::visit(shared_ptr<IConst> node) {
    result = std::to_string(node->value);
}

void ExpStringBuilder::visit(shared_ptr<BConst> node) {
    //@todo: support boolean variables directly?
    result = node->value ? "true" : "false";
}

void ExpStringBuilder::visit(shared_ptr<FConst> node) {
    result = std::to_string(node->value);
}

void ExpStringBuilder::visit(shared_ptr<LocExp> node) {
    if (node->location->is_array_position()) {
        throw_FigException("Array position are not yet supported");
    }
    if (ModuleScope::globals.find(node->location->id)
            != ModuleScope::globals.end()) {
        //a global constant, not a module state.
        ExpEvaluator eval;
        node->accept(eval);
        if (eval.has_errors()) {
            throw_FigException("Could not reduce constant at compilation time");
        }
        result = eval.value_to_string();
    } else {
        result = node->location->id;
        names.push_back(result);
    }
    should_enclose = false;
}

void ExpStringBuilder::visit(shared_ptr<OpExp> exp) {
    string left_s;
    string right_s;
    string op = ModelPrinter::to_str(exp->bop);
    if (exp->arity == Arity::one) {
        exp->left->accept(*this);
        left_s = should_enclose ? "(" + result + ")" : result;
        result = op + left_s;
    } else if (exp->arity == Arity::two) {
        exp->left->accept(*this);
        left_s = should_enclose ? "(" + result + ")" : result;
        exp->right->accept(*this);
        right_s = should_enclose ? "(" + result + ")" : result;
        result = left_s + op + right_s;
    }
    should_enclose = false;
}

string ExpStringBuilder::str() {
    return (result);
}

const vector<string>& ExpStringBuilder::get_names() {
    return (names);
}

pair<string, vector<string>>
ExpStringBuilder::make_conjunction_str(const vector<shared_ptr<Exp>>& expvec) {
    stringstream ss;
    vector<string> names;
    auto it = expvec.begin();
    while (it != expvec.end()) {
        ExpStringBuilder str_b;
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

