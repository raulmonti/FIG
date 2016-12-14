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
#include <ExpStringBuilder.h>
#include <ModelBuilder.h>

// ADL
using std::begin;
using std::end;

namespace {
using Clause = parser::PropertyProjection::Clause;
using Term = parser::PropertyProjection::Term;
using DNF  = parser::PropertyProjection::DNF;

bool has_identifiers_in(Term exp, const vector<string>& varnames) {
    ExpStringBuilder str_b (CompositeModuleScope::get_instance());
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

inline Clause clause_from_terms(const vector<Term> &terms) {
    //shared_ptr<ModuleScope> scope = CompositeModuleScope::get_instance();
    //std::pair<string, vector<string>> str_names
    //        =  ExpStringBuilder::make_conjunction_str(scope, terms);
    //return Precondition(str_names.first, str_names.second);
    throw_FigException("FIXME!");
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
    int p_id = property.get_id();
    if (populated_ids.find(p_id) != populated_ids.end()) {
        return;
    }
    populated_ids.insert(p_id);
    shared_ptr<Prop> prop = ModelBuilder::property_ast[p_id];
    switch (prop->get_type()) {
    case PropType::transient:
    {
        shared_ptr<TransientProp> tprop = prop->to_transient();
        ExprDNFBuilder left_b (CompositeModuleScope::get_instance());
        tprop->get_left()->accept(left_b);
        others_ = left_b.get_clauses();
        ExprDNFBuilder right_b (CompositeModuleScope::get_instance());
        tprop->get_right()->accept(right_b);
        rares_ = right_b.get_clauses();
        break;
    }
    case PropType::rate:
    {
        shared_ptr<RateProp> rprop = prop->to_rate();
        ExprDNFBuilder left_b (CompositeModuleScope::get_instance());
        rprop->get_expression()->accept(left_b);
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
    populate(prop);
}

std::pair <vector<Clause>, vector<Clause>>
PropertyProjection::project(const State& localState) const {
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

