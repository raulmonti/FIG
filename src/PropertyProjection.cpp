/** Leonardo Rodríguez **/

// C++
#include <iterator>
#include <algorithm>  // std::find()
// FIG
#include <PropertyProjection.h>
#include <PropertyTransient.h>
#include <PropertyRate.h>
#include <string_utils.h>
#include <ExprDNFBuilder.h>
#include <ModelBuilder.h>

// ADL
using std::begin;
using std::end;

namespace {
using Clause = parser::PropertyProjection::Clause;
using Term = parser::PropertyProjection::Term;
using DNF  = parser::PropertyProjection::DNF;

Term make_conjunction_until(const vector<Term>& terms, int until) {
    assert(terms.size() > 0);
    assert(until < (int) terms.size());
    Term result = nullptr;
    if (until == 0) {
        result = terms.at(0);
    } else if (until > 0) {
        Term last_exp = terms.at(until);
        Term left = make_conjunction_until(terms, until - 1);
        result =
                make_shared<OpExp>(Arity::two, ExpOp::andd, left, last_exp);
    }
    return (result);
}

Term make_conjunction(const vector<Term>& terms) {
    assert(terms.size() > 0);
    return (make_conjunction_until(terms, terms.size() - 1));
}

Clause clause_from_terms(const vector<Term>& terms) {
    assert(terms.size() > 0);
    Term exp = make_conjunction(terms);
    ExpStringBuilder str_b;
    exp->accept(str_b);
    assert(!str_b.has_errors());
    return Precondition(str_b.str(), str_b.get_names());
}

bool has_identifiers_in(Term exp, const vector<string>& varnames) {
    ExpStringBuilder str_b;
    //@todo write another visitor that returns names without
    // making a string in the process
    exp->accept(str_b);
    auto in_varnames = [&] (const std::string &id) -> bool
    {
        auto it = std::find(varnames.begin(), varnames.end(), id);
        return (it != varnames.end());
    };
    auto &names = str_b.get_names();
    bool result = std::find_if(names.begin(), names.end(), in_varnames)
            != names.end();
    return (result);
}

vector<Clause> project_on_var_set(const DNF& dnf,
                                  const vector<string>& varnames) {
    vector<Clause> result;
    for (auto& term_vec : dnf) {
        vector<Term> p_term_vec;
        for (auto &term : term_vec) {
            if (has_identifiers_in(term, varnames)) {
                p_term_vec.push_back(term);
            }
        }
        if (p_term_vec.size() > 0) {
            const Clause& clause = clause_from_terms(p_term_vec);
            result.push_back(clause);
        }
    }
    return (result);
}
}

namespace parser {

void PropertyProjection::populate(const fig::Property& property) {
    assert(false);
    int p_id = property.get_id();
    if (populated_ids.find(p_id) != populated_ids.end()) {
        return;
    }
    populated_ids.insert(p_id);
    shared_ptr<Prop> prop = ModelBuilder::property_ast[p_id];
    switch (prop->type) {
    case PropType::transient:
    {
        ExprDNFBuilder left_b;
        prop->left->accept(left_b);
        rares_ = left_b.get_clauses();
        ExprDNFBuilder right_b;
        prop->right->accept(right_b);
        others_ = right_b.get_clauses();
        break;
    }
    case PropType::rate:
    {
        ExprDNFBuilder left_b;
        prop->left->accept(left_b);
        rares_ = left_b.get_clauses();
        break;
    }
    default:
    {
        throw_FigException("invalid property type");
    }
    }
}

PropertyProjection::PropertyProjection() {}
PropertyProjection::PropertyProjection(const fig::Property &prop) {
    assert(false);
    populate(prop);
}

std::pair <vector<Clause>, vector<Clause>>
PropertyProjection::project(const State& localState) const {
    assert(false);
    vector<std::string> varnames = localState.varnames();
    for (auto& varname: varnames) {
        trim(varname); //@todo: why is trimming necessary here?
    }
    auto rares = ::project_on_var_set(rares_, varnames);
    for (Clause& clause: rares) {
        clause.pin_up_vars(localState);
    }
    auto others = ::project_on_var_set(others_, varnames);
    for (Clause& clause: others) {
        clause.pin_up_vars(localState);
    }
    return std::make_pair(rares, others);
}

PropertyProjection::~PropertyProjection() {}

}

