/* Leonardo Rodríguez */

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
	ExpReductor reductor(current_scope, reduceLocations_);
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
    current_scope = CompositeModuleScope::get_instance();
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

void ModelReductor::visit(shared_ptr<TBoundSSProp> node) {
    node->set_expression(reduce(node->get_expression()));
}

// Declarations
void ModelReductor::visit(shared_ptr<InitializedDecl> decl) {
    decl->set_init(reduce(decl->get_init()));
    if (decl->is_constant()) {
		if (!decl->get_init()->is_constant() && reduceLocations_) {
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
void ModelReductor::compute_int(int &to,
                                shared_ptr<Exp> exp) {
	ExpEvaluator ev(current_scope);
    exp->accept(ev);
    if (ev.has_errors()) {
        put_error(ev.get_messages() + "Reduction error, setting value to 0");
        to = 0;
    } else if (ev.has_type_int()) {
        to = ev.get_int();
    } else if (ev.has_type_bool()){
        to = ev.get_bool() ? 1 : 0;
    } else {
        put_error("Wrong type for value.");
    }
}

void ModelReductor::reduce_size(shared_ptr<ArrayDecl> node) {
    node->set_size(reduce(node->get_size()));
}

void ModelReductor::reduce_range(shared_ptr<Ranged> node) {
    node->set_lower_bound(reduce(node->get_lower_bound()));
    node->set_upper_bound(reduce(node->get_upper_bound()));
}

void ModelReductor::reduce_init(shared_ptr<Initialized> decl) {
    decl->set_init(reduce(decl->get_init()));
}

void ModelReductor::reduce_multiple_init(shared_ptr<MultipleInitialized> decl) {
    reduce_vector(decl->get_inits());
}

void ModelReductor::check_data(const ArrayData& data) {
    if (data.data_size <= 0) {
        put_error("Empty arrays not supported");
        return;
    }
    if (data.data_min > data.data_max) {
        put_error("Wrong range for array: lower > upper");
        return;
    }
	if (data.data_inits.size() != static_cast<size_t>(data.data_size)) {
        put_error("Wrong number of array initializations");
        return;
    }
    for (const int x : data.data_inits) {
        if (x < data.data_min) {
            put_error("Initialization lesser than lower bound");
            return;
        }
        if (x > data.data_max) {
            put_error("Initialization greater than upper bound");
            return;
        }
    }
}

void ModelReductor::visit(shared_ptr<RangedInitializedArray> node) {
    reduce_size(node);
    reduce_range(node);
    reduce_init(node);
    ArrayData data;
    compute_int(data.data_size, node->get_size());
    compute_int(data.data_min, node->get_lower_bound());
    compute_int(data.data_max, node->get_upper_bound());

    if (has_errors()) {
        return; //failed on arrays parameters reduction
    }

    int x = 0;
    compute_int(x, node->get_init());
	data.data_inits.resize(static_cast<size_t>(data.data_size));
    //repeat the same initialization for every element
	for (auto i = 0ul ; i < static_cast<size_t>(data.data_size); i++) {
        data.data_inits[i] = x;
    }
    check_data(data);
    node->set_data(data);
}

void ModelReductor::visit(shared_ptr<RangedMultipleInitializedArray> node) {
    reduce_size(node);
    reduce_range(node);
    reduce_multiple_init(node);
    ArrayData data;
    compute_int(data.data_size, node->get_size());
    compute_int(data.data_min, node->get_lower_bound());
    compute_int(data.data_max, node->get_upper_bound());

    if (has_errors()) {
        return; //failed on arrays parameters reduction
    }
    std::vector<shared_ptr<Exp>>& exps = node->get_inits();
	if (exps.size() != static_cast<size_t>(data.data_size)) {
        put_error("Wrong number of initializations, "
                  "expected " + std::to_string(data.data_size));
        return;
    }
	data.data_inits.resize(static_cast<size_t>(data.data_size));
	for (auto i = 0ul; i < static_cast<size_t>(data.data_size); i++) {
        int x;
        compute_int(x, exps[i]);
        data.data_inits[i] = x;
    }
    check_data(data);
    node->set_data(data);
}

void ModelReductor::visit(shared_ptr<InitializedArray> node) {
    reduce_size(node);
    reduce_init(node);
    ArrayData data;
    compute_int(data.data_size, node->get_size());
    //only boolean supported here:
    data.data_min = 0;
    data.data_max = 1;
	data.data_inits.resize(static_cast<size_t>(data.data_size));
    int x = 0;
    compute_int(x, node->get_init());
	for (auto i = 0ul; i < static_cast<size_t>(data.data_size); i++) {
        data.data_inits[i] = x;
    }
    check_data(data);
    node->set_data(data);
}

void ModelReductor::visit(shared_ptr<MultipleInitializedArray> node) {
    reduce_size(node);
    reduce_multiple_init(node);
    ArrayData data;
    compute_int(data.data_size, node->get_size());
    //only booleands supported here:
    data.data_min = 0;
    data.data_max = 1;
	data.data_inits.resize(static_cast<size_t>(data.data_size));
    std::vector<shared_ptr<Exp>>& exps = node->get_inits();
	if (exps.size() != static_cast<size_t>(data.data_size)) {
        put_error("Wrong number of initializations, "
                  "expected " + std::to_string(data.data_size));
        return;
    }
	data.data_inits.resize(static_cast<size_t>(data.data_size));
	for (auto i = 0ul; i < static_cast<size_t>(data.data_size); i++) {
        int x;
        compute_int(x, exps[i]);
        data.data_inits[i] = x;
    }
    check_data(data);
    node->set_data(data);
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
    shared_ptr<Location> loc = node->get_effect_location();
    //find declaration and save for convenience:
    const std::string& id = loc->get_identifier();
    shared_ptr<Decl> decl = ModuleScope::find_identifier_on(current_scope, id);
    assert(decl != nullptr);
    loc->set_decl(decl);
    if (loc->is_array_position()) {
        assert(current_scope != nullptr);
        //reduce the index in the array position
        shared_ptr<ArrayPosition> ap = loc->to_array_position();
        ap->set_index(reduce(ap->get_index()));
        assert(ap->get_decl() != nullptr); //just set it.
        assert(ap->get_decl()->to_array() != nullptr);
    }
    node->set_rhs(reduce(node->get_rhs()));
}

void ModelReductor::visit(shared_ptr<ClockReset> node) {
    accept_cond(node->get_dist());
}

//Distributions
void ModelReductor::visit(shared_ptr<SingleParameterDist> node) {
    node->set_parameter(reduce(node->get_parameter()));
	if (!node->get_parameter()->is_constant() && reduceLocations_) {
        put_error("Distribution paremeters must be reducible at compilation time");
    }
}

void ModelReductor::visit(shared_ptr<MultipleParameterDist> node) {
    node->set_first_parameter(reduce(node->get_first_parameter()));
	node->set_second_parameter(reduce(node->get_second_parameter()));
	if (!node->get_first_parameter()->is_constant() && reduceLocations_) {
        put_error("Distribution paremeters must be reducible at compilation time");
    }
	if (!node->get_second_parameter()->is_constant() && reduceLocations_) {
		put_error("Distribution paremeters must be reducible at compilation time");
	}
	if (node->num_parameters() == 3ul) {
		node->set_third_parameter(reduce(node->get_third_parameter()));
		if (!node->get_third_parameter()->is_constant() && reduceLocations_)
			put_error("Distribution paremeters must be reducible at compilation time");
	}
}
