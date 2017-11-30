/* Leonardo Rodríguez */

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


/// Find if an expression may depend on the given set of identifiers.
class NameFinderVisitor : public Visitor {
    std::set<std::string> names;
    bool hasIdentifiersIn = false;
public:
    NameFinderVisitor(const std::vector<std::string>& names) {
        for (const std::string &str :names) {
            this->names.insert(str);
        }
    }

    void visit(shared_ptr<LocExp> node) override {
        shared_ptr<Location> loc = node->get_exp_location();
        if (loc->is_array_position() || loc->get_decl()->is_array()) {
            //the expressions is of the form "arr" or "arr[j]" where "j" may not
            //be known statically. We consider "arr" or "arr[j]" relevant if
            // "arr" or "arr[i]" with any "i" appears on names.
            hasIdentifiersIn = names.find(loc->get_identifier()) != names.end();
            shared_ptr<ArrayDecl> decl = loc->get_decl()->to_array();
            const ArrayData &data = decl->get_data();
            size_t i = 0;
            while (i < (size_t) data.data_size && !hasIdentifiersIn) {
                std::string indexed
                        = loc->get_identifier() + "[" + std::to_string(i) + "]";
                hasIdentifiersIn = names.find(indexed) != names.end();
                i++;
            }
        } else {
            //simple var, just check if it is in the set
            hasIdentifiersIn = names.find(loc->get_identifier()) != names.end();
        }
    }

    void visit(shared_ptr<UnOpExp> node) override {
        if (!hasIdentifiersIn) {
            node->get_argument()->accept(*this);
        }
    }

    void visit(shared_ptr<BinOpExp> node) override {
        if (!hasIdentifiersIn) {
            node->get_first_argument()->accept(*this);
        }
        if (!hasIdentifiersIn) {
            node->get_second_argument()->accept(*this);
        }
    }

    bool has_identifiers_in() const {
        return (hasIdentifiersIn);
    }
};

/// Decides whether the term may depend on the set of variables
bool has_identifiers_in(Term exp, const vector<string>& varnames) {
    NameFinderVisitor visitor (varnames);
    exp->accept(visitor);
    return (visitor.has_identifiers_in());
}


std::shared_ptr<Exp>
clause_from_terms(const vector<Term> &terms) {
    //shared_ptr<ModuleScope> scope = CompositeModuleScope::get_instance();
    //std::pair<string, vector<string>> str_names
    //        =  ExpStringBuilder::make_conjunction_str(scope, terms);
    //return Precondition(str_names.first, str_names.second);
    shared_ptr<Exp> exp = nullptr;
    size_t tam = terms.size();
    if (tam > 0) {
        exp = terms[0];
    }
    size_t i = 1; //0 already included.
    while (i < tam) {
        exp = BinOpExp::make_andd(exp, terms[i]);
        i++;
    }
    if (exp == nullptr) {
        throw_FigException("Can't construct clause");
    }
	return exp;
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
			result.emplace_back(clause_from_terms(p_term_vec));
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
	assert(nullptr != prop);
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
        clause.prepare(localState);
    }
    auto others = ::project_on_var_set(others_, varnames);
    for (Clause& clause: others) {
        clause.prepare(localState);
    }
	return std::make_pair(std::move(rares), std::move(others));
}

PropertyProjection::~PropertyProjection() {}

}

