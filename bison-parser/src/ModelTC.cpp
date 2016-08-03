#include "ModelTC.h"
#include <cassert>

using std::endl;

string ModelTC::get_prefix() {
    string s;
    if (is_global_scope()) {
	s = "Global constans:";
    } else {
	s = "At Module " + current_scope->id;
    }
    return (s);
}

void ModelTC::visit(ModelAST* node) {
//checks nothing :)
     (void) node;
}

void ModelTC::accept_cond(ModelAST *node) {
    if (!log->has_errors()) {
	node->accept(*this);
    }
}

void ModelTC::visit(Model* model) {
    //check globals
    for (auto decl : model->get_globals()) {
	//no module selected yet
	current_scope = nullptr;
	accept_cond(decl);
    }
    //check modules
    for (auto entry : model->get_modules()) {
	ModuleScope *new_scope = new ModuleScope();
	const string &id = entry.first;
	if (scopes.find(id) != scopes.end()) {
	      log->put_error("There is already a module with name " + id);	      
	}
	new_scope->body = entry.second;
	new_scope->id = id;
	 //set current scope before accepting module body
	current_scope = new_scope;
	scopes[id] = new_scope;
	accept_cond(new_scope->body);
    }
}

void ModelTC::visit(ModuleBody* body) {
    assert(body != nullptr);
    assert(current_scope != nullptr);
    assert(current_scope->body == body);
    for (auto decl : body->get_local_decls()) {
	accept_cond(decl);
    }
    for (auto *action : body->get_actions()) {
	accept_cond(action);
    }
}

void ModelTC::visit(Decl* decl) {
    assert(decl != nullptr);
    const string &id = decl->id;
    //check expressions before adding identifier to scope.
    //e.g. this should be ill typed: x : bool init x;
    if (decl->has_range()) {
	//check range expressions
	accept_cond(decl->lower);
	accept_cond(decl->upper);
    }
    if (decl->is_array()) {
	//check array size expression
	accept_cond(decl->size);
    }
    for (Exp *init : decl->get_inits()) {
	//check initialization expressions
	accept_cond(init);
    }
    if (is_global_scope()) {
	//check if already in global scope
	if (globals.find(id) != globals.end()) {
	    log->put_error("Identifier \"" + id + "\" redefined");	      
	} else {
	    globals[id] = decl;
	}
    } else {
	//check if decl is already in local scope
	auto &local = current_scope->local_decls;
	if (local.find(id) != local.end()) {
	    log->put_error(get_prefix() +
			   " : Identifier \"" + id +
			   "\" is already defined in local scope");
	} else {
	    local[id] = decl;
	}
    }
}

void ModelTC::visit(Action* action) {
    assert(action != nullptr);
    assert(current_scope != nullptr);
    string &label = action->id;
    LabelType &label_type = action->type;
    auto &labels = current_scope->labels;
    if (label_type != LabelType::empty) {
	//check if label is already used with another type
	if (labels.find(label) != labels.end()) {
	    LabelType &other = labels[label];
	    if (label_type != other) {
		log->put_error("In Module " + current_scope->id +
			       ": label \"" + label +
			       "\" must have a single type");
	    }
	} else {
	    labels[label] = label_type;
	}
    }
    //Note: output label has clock: ensured by parser.
    //Note: input label has no clock: ensured by parser.
    assert(action->guard != nullptr);
    accept_cond(action->guard);
    if (action->has_clock()) {
	accept_cond(action->clock_loc);
	auto &label_clocks = current_scope->label_clocks;
	// check if label (if not empty) has at most one clock
	if (label_clocks.find(label) != label_clocks.end()) {
	    string &clock_id = label_clocks[label];
	    if (clock_id != action->clock_loc->id) {
		log->put_error("In Module " + current_scope->id +
			       ": label \"" + label +
			       "\" must have at most one clock");
	    }
	} else if (label_type != LabelType::empty) {
	    label_clocks[label] = action->clock_loc->id;
	}
    }
    for (auto *effect : action->get_effects()) {
	accept_cond(effect);
    }
}

void ModelTC::visit(Effect* effect) {
    assert(effect != nullptr);
    accept_cond(effect->loc);
    if (effect->is_clock_reset()) {
	accept_cond(effect->dist);
	// check that the same clock is not reseted
	// with diferent **type** of distribution
	// parameters should be checked after constants
	// reduction
	auto &clock_dists = current_scope->clock_dists;
	string &clock_id = effect->loc->id;
	if (clock_dists.find(clock_id) != clock_dists.end()) {
	    DistType dist_type = clock_dists[clock_id]->type;
	    if (dist_type != effect->dist->type) {
		log->put_error(get_prefix() +
			       ": Clock \"" + clock_id +
			       "\" must have a single distribution type");
	    }
	} else {
	    clock_dists[clock_id] = effect->dist;
	}
    }
    if (effect->is_state_change()) {
	accept_cond(effect->arg);
    }
}

void ModelTC::visit(Dist* dist) {
    assert(dist != nullptr);
    if (dist->arity == Arity::one) {
	accept_cond(dist->param1);
    }
    if (dist->arity == Arity::two) {
	accept_cond(dist->param1);
	accept_cond(dist->param2);
    }
}

void ModelTC::visit(Location* loc) {
    assert(loc != nullptr);
    string &id = loc->id;
    if (is_global_scope()) {
	//check if already in global scope
	if (globals.find(id) == globals.end()) {
	    log->put_error("Identifier \"" + id + "\" not in scope");	      
	}
    }
    else {
	auto &local = current_scope->local_decls;
	//location could be local or global
	if (local.find(id) == local.end() &&
	    globals.find(id) == globals.end()) {
	    log->put_error(get_prefix() +
			   ": identifier \"" + id +
			   "\" is not in scope");
	}  
    }
    if (loc->is_array_position()) {
	accept_cond(loc->index);
    }
}

void ModelTC::visit(Exp* node) {
    (void) node;
    //this should not be called
    assert(false);
}

void ModelTC::visit(IConst* node) {
    (void) node;
}

void ModelTC::visit(BConst* node){
    (void) node;
}

void ModelTC::visit(FConst* node){
    (void) node;
}

void ModelTC::visit(LocExp* locExp){
    assert(locExp != nullptr);
    accept_cond(locExp->location);
}

void ModelTC::visit(OpExp* exp){
    assert(exp != nullptr);
    if (exp->arity == Arity::one) {
	accept_cond(exp->left);
    }
    if (exp->arity == Arity::two) {
	accept_cond(exp->left);
	accept_cond(exp->right);
    }
}
