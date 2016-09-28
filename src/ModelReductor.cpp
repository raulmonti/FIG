#include "ModelReductor.h"
#include "FigException.h"
#include "ExpReductor.h"
#include <algorithm>

void ModelReductor::accept_cond(shared_ptr<ModelAST> node) {
    if (!has_errors()) {
        node->accept(*this);
    }
}

shared_ptr<Exp> ModelReductor::reduce(shared_ptr<Exp> node) {
    ExpReductor reductor (current_scope);
    node->accept(reductor);
    if (reductor.has_errors()) {
        put_error(reductor.get_messages());
    }
    shared_ptr<Exp> reduced = reductor.get_reduced_exp();
    return (reduced);
}

void ModelReductor::reduce_vector(shared_vector<Exp>& vector) {
    auto it = vector.begin();
    while (it != vector.end()) {
        *it = reduce(*it);
        it++;
    }
}

void ModelReductor::visit(shared_ptr<Model> node)  {
    //visit globals
    for (auto decl : node->get_globals()) {
        accept_cond(decl);
    }
    //visit modules
    for (auto module : node->get_modules()) {
        current_scope = ModuleScope::scopes.at(module->get_name());
        accept_cond(module);
    }
    //visit props
    for (auto prop : node->get_props()) {
        accept_cond(prop);
    }
}

void ModelReductor::visit(shared_ptr<ModuleAST> node) {
    //visit local decl.
    for (auto decl : node->get_local_decls()) {
        accept_cond(decl);
    }
    //visit transitions
    for (auto tr : node->get_transitions()) {
        accept_cond(tr);
    }
}

void ModelReductor::visit(shared_ptr<TransientProp> node) {
    node->set_left(reduce(node->get_left()));
    node->set_right(reduce(node->get_right()));
}

void ModelReductor::visit(shared_ptr<RateProp> node) {
    node->set_expression(reduce(node->get_expression()));
}

// Declarations
void ModelReductor::visit(shared_ptr<InitializedDecl> decl){
    decl->set_init(reduce(decl->get_init()));
    if (decl->is_constant()) {
        if (!decl->get_init()->is_constant()) {
            put_error("Constant \"" + decl->get_id()
                      + "\" is not reducible at compilation time.");
        }
    }
}

void ModelReductor::visit(shared_ptr<RangedDecl> decl) {
    decl->set_init(reduce(decl->get_init()));
    decl->set_lower_bound(reduce(decl->get_lower_bound()));
    decl->set_upper_bound(reduce(decl->get_upper_bound()));
}

void ModelReductor::visit(shared_ptr<ClockDecl>) {
    //nothing to reduce
}

void ModelReductor::visit(shared_ptr<ArrayDecl> node) {
    node->set_size(reduce(node->get_size()));
}

void ModelReductor::visit(shared_ptr<InitializedArray> node) {
    node->set_init(reduce(node->get_init()));
    node->set_size(reduce(node->get_size()));
}

void ModelReductor::visit(shared_ptr<MultipleInitializedArray> node) {
    //reduce every initialization
    reduce_vector(node->get_inits());
    node->set_size(reduce(node->get_size()));
}

void ModelReductor::visit(shared_ptr<RangedInitializedArray> node) {
    node->set_lower_bound(reduce(node->get_lower_bound()));
    node->set_upper_bound(reduce(node->get_upper_bound()));
    node->set_init(reduce(node->get_init()));
    node->set_size(reduce(node->get_size()));
}

void ModelReductor::visit(shared_ptr<RangedMultipleInitializedArray> node) {
    reduce_vector(node->get_inits());
    node->set_lower_bound(reduce(node->get_lower_bound()));
    node->set_upper_bound(reduce(node->get_upper_bound()));
    node->set_size(reduce(node->get_size()));
}

//Transitions
void ModelReductor::visit(shared_ptr<TransitionAST> node) {
    node->set_precondition(reduce(node->get_precondition()));
    for (auto a : node->get_assignments()) {
        accept_cond(a);
    }
    for (auto c : node->get_clock_resets()) {
        accept_cond(c);
    }
}

void ModelReductor::visit(shared_ptr<Assignment> node) {
    node->set_rhs(reduce(node->get_rhs()));
}

void ModelReductor::visit(shared_ptr<ClockReset> node) {
    accept_cond(node->get_dist());
}

//Distributions
void ModelReductor::visit(shared_ptr<SingleParameterDist> node) {
    node->set_parameter(reduce(node->get_parameter()));
    if (!node->get_parameter()->is_constant()) {
        put_error("Distribution paremeters must be reducible at compilation time");
    }
}

void ModelReductor::visit(shared_ptr<MultipleParameterDist> node) {
    node->set_first_parameter(reduce(node->get_first_parameter()));
    node->set_second_parameter(reduce(node->get_second_parameter()));
    if (!node->get_first_parameter()->is_constant()) {
        put_error("Distribution paremeters must be reducible at compilation time");
    }
    if (!node->get_second_parameter()->is_constant()) {
        put_error("Distribution paremeters must be reducible at compilation time");
    }
}

//Locations
void ModelReductor::visit(shared_ptr<ArrayPosition> node) {
    node->set_index(reduce(node->get_index()));
}
