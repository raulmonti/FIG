#include "ModelTC.h"
#include "ModelPrinter.h"
#include <cassert>

using std::endl;
using std::make_pair;

/* Typechecking on Model */

//First some macros to print errors:
#define UNEXPECTED_TYPE(expected, got)			   \
    "Expected type is " + ModelPrinter::to_str(expected) + \
    " - Inferred type is " + ModelPrinter::to_str(got)	   \
    
#define EXPECTED_NUMERIC				  \
    "Expected type " + ModelPrinter::to_str(Type::tint) + \
    " or type " + ModelPrinter::to_str(Type::tfloat)	  \

#define PREFIX						\
    (is_global_scope() ? "At global constants" :	\
     "At Module " +					\
     current_scope->id)					\
    
#define TC_INDEX_INT(id)			\
    PREFIX		+			\
    " - Identifier \""  +			\
    id +					\
    "\" - Index expression - "			\
    UNEXPECTED_TYPE(Type::tint, last_type)	\
    
#define TC_ID_REDEFINED(id)			\
    PREFIX		+			\
    " - Identifier \""  +			\
    id +					\
    "\" was redefined"				\
    
#define TC_ID_SCOPE(id)					\
    PREFIX		+				\
    " - Identifier \""  +				\
    id +						\
    "\" is not in scope"				\
    
#define TC_LOWER_BOUND(id)				\
    PREFIX		+				\
    " - Identifier \""  +				\
    id +						\
    "\" - Lower bound of range is ill-typed - " +	\
    UNEXPECTED_TYPE(Type::tint, last_type)		\

#define TC_UPPER_BOUND(id)				\
    PREFIX		+				\
    " - Identifier \""  +				\
    id +						\
    "\" - Upper bound of range is ill-typed - " +	\
    UNEXPECTED_TYPE(Type::tint, last_type)		\
    
#define TC_SIZE_EXP(id)					\
    PREFIX		+				\
    " - Identifier \""  +				\
    id +						\
    "\" - Array size expression is ill typed - " +	\
    UNEXPECTED_TYPE(Type::tint, last_type)		\
    
#define TC_DIST_FIRST_PARAM(dist)			\
    PREFIX		+				\
    " - Distribution "  +				\
    ModelPrinter::to_str(dist) +			\
    " - First parameter is ill typed - " +		\
    UNEXPECTED_TYPE(Type::tfloat, last_type)		\
    
#define TC_DIST_SECOND_PARAM(dist)			\
    PREFIX		+				\
    " - Distribution "  +				\
    ModelPrinter::to_str(dist) +			\
    " - Second parameter is ill typed - " +		\
    UNEXPECTED_TYPE(Type::tfloat, last_type)		\
    
#define TC_INIT_EXP(id, expected)			\
    PREFIX		+				\
    " - Identifier \""  +				\
    id +						\
    "\" - Initializer is ill-typed - " +		\
    UNEXPECTED_TYPE(Type::tint, last_type)		\
    
#define TC_LABEL_TYPE(label)				\
    PREFIX		+				\
    " - Label \""  +					\
    label +						\
    "\" must have a single type "			\
    
#define TC_CLOCK_TYPE(clock_id)				\
    PREFIX		+				\
    " - Clock \""  +					\
    clock_id +						\
    "\" must have a single distribution type"		\
    
#define TC_LABEL_CLOCK(label)					\
    PREFIX		+					\
    " - Label \""  +						\
    label +							\
    "\" must have a single clock"				\
    
#define TC_LABEL_NOT_A_CLOCK(label, clock_id)			\
    PREFIX		+					\
    " - Transition of Label \""  +				\
    label +							\
    "\" - Identifier \"" +					\
    clock_id +							\
    "\" is not a clock - " +					\
    UNEXPECTED_TYPE(Type::tint, last_type)			\
    
#define TC_LABEL_GUARD(label)					\
    PREFIX		+					\
    " - Transition of Label \""  +				\
    label +							\
    "\" - Condition is ill-typed "				\
    UNEXPECTED_TYPE(Type::tbool, last_type)			\
    
#define TC_LABEL_SILENT_GUARD						\
    PREFIX		+						\
    " - Transition of silent label - "  +				\
    " - Condition is ill-typed - " +					\
    UNEXPECTED_TYPE(Type::tbool, last_type)				\
    
#define TC_STATE_EXP(id, expected)					\
    PREFIX		+						\
    " - Assignment of state \""  +					\
    id +								\
    "'\" - Expression is ill-typed - "	+				\
    UNEXPECTED_TYPE(expected, last_type)				\
    
#define TC_OP_FIRST_ARG(op)				\
    PREFIX		+				\
    " - Operator "  +					\
    ModelPrinter::to_str(op) +				\
    "- First argument has an incompatible type "	\
    
#define TC_OP_SECOND_ARG(op)			\
    PREFIX +					\
    " - Operator "  +				\
    ModelPrinter::to_str(op) +			\
    "- Second argument has an " +		\
    "incompatible type "			\

bool type_leq(Type t1, Type t2) {
    bool res = (t1 == Type::tint && t2 == Type::tfloat);
    res = res || (t1 == t2);
    return (res);
};

inline void ModelTC::check_type(Type type, const string &msg) {
    if (!log->has_errors() && !type_leq(last_type, type)) {
	log->put_error(msg);
    }
}

inline Type ModelTC::identifier_type(const string &id) {
    Type type = Type::tunknown;
    if (globals.find(id) != globals.end()) {
	type = globals[id]->type;
    }				    
    if (type == Type::tunknown && current_scope != nullptr) {
	auto &local = current_scope->local_decls;
	if (local.find(id) != local.end()) {
	    type = local[id]->type;
	}
    }
    return (type);
}

inline Type _numeric_result(Type type) {
    Type result;
    if (type == Type::tint) {
	result = type;
    } else if (type == Type::tfloat) {
	result = type;
    } else {
	result = Type::tunknown;
    }
    return (result);
}

inline Type _bool_op(Type type) {
    Type result;
    if (type == Type::tbool) {
	result = type;
    } else {
	result = Type::tunknown;
    }
    return (result);
}

inline Type _rel_op(Type type) {
    Type result;
    if (type == Type::tint || type == Type::tfloat) {
	result = Type::tbool;
    } else {
	result = Type::tunknown;
    }
    return (result);
}

inline Type ModelTC::operator_type(const ExpOp &op, Type arg) {
    Type result = Type::tunknown;
    switch(op) {
    case ExpOp::plus: result = _numeric_result(arg); break;
    case ExpOp::times: result = _numeric_result(arg); break;
    case ExpOp::minus: result = _numeric_result(arg); break;
    case ExpOp::div: result = _numeric_result(arg); break;
    case ExpOp::mod: result = _numeric_result(arg); break;
    case ExpOp::andd: result = _bool_op(arg); break;
    case ExpOp::orr: result = _bool_op(arg); break;
    case ExpOp::nott: result = _bool_op(arg); break;
    case ExpOp::eq: result = Type::tbool; break; //equality for all types?
    case ExpOp::neq: result = Type::tbool;  break; 
    case ExpOp::lt: result = _rel_op(arg); break; 
    case ExpOp::gt: result = _rel_op(arg); break; 
    case ExpOp::le: result = _rel_op(arg); break; 
    case ExpOp::ge: result = _rel_op(arg); break; 
    }
    return (result);
}

void ModelTC::visit(ModelAST* node) {
    (void) node;
}

inline void ModelTC::accept_cond(ModelAST *node) {
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
	    log->put_error(TC_ID_REDEFINED(id));	      
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
	check_type(Type::tint, TC_LOWER_BOUND(id));
	accept_cond(decl->upper);
	check_type(Type::tint, TC_UPPER_BOUND(id));
    }
    if (decl->is_array()) {
	//check array size expression
	accept_cond(decl->size);
	check_type(Type::tint, TC_SIZE_EXP(id));
    }
    for (Exp *init : decl->get_inits()) {
	//check initialization expressions
	accept_cond(init);
	check_type(decl->type, TC_INIT_EXP(id, decl->type)); 
    }
    if (is_global_scope()) {
	//check if already in global scope
	if (globals.find(id) != globals.end()) {
	    log->put_error(TC_ID_REDEFINED(id));	      
	} else {
	    globals[id] = decl;
	}
    } else {
	//check if decl is already in local scope
	auto &local = current_scope->local_decls;
	if (local.find(id) != local.end()) {
	    log->put_error(TC_ID_REDEFINED(id));
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
		log->put_error(TC_LABEL_TYPE(label));
	    }
	} else {
	    labels[label] = label_type;
	}
    }
    //Note: output label has clock: ensured by parser.
    //Note: input label has no clock: ensured by parser.
    assert(action->guard != nullptr);
    accept_cond(action->guard);
    if (label_type != LabelType::empty) {
	check_type(Type::tbool, TC_LABEL_GUARD(label)) ; 
    } else {
	check_type(Type::tbool, TC_LABEL_SILENT_GUARD) ; 
    }
    
    if (action->has_clock()) {
	accept_cond(action->clock_loc);
	check_type(Type::tclock,
		   TC_LABEL_NOT_A_CLOCK(label, action->clock_loc->id)); 
	auto &label_clocks = current_scope->label_clocks;
	// check if label (if not empty) has at most one clock
	if (label_clocks.find(label) != label_clocks.end()) {
	    string &clock_id = label_clocks[label];
	    if (clock_id != action->clock_loc->id) {
		log->put_error(TC_LABEL_CLOCK(clock_id));
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
    //save type of location
    Type loc_type = last_type;
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
		log->put_error(TC_CLOCK_TYPE(clock_id));
	    }
	} else {
	    clock_dists[clock_id] = effect->dist;
	}
    }
    if (effect->is_state_change()) {
	accept_cond(effect->arg);
	check_type(loc_type, TC_STATE_EXP(effect->loc->id, loc_type));
    }
}

void ModelTC::visit(Dist* dist) {
    assert(dist != nullptr);
    if (dist->arity == Arity::one) {
	accept_cond(dist->param1);
	check_type(Type::tfloat, TC_DIST_FIRST_PARAM(dist->type));
    }
    if (dist->arity == Arity::two) {
	accept_cond(dist->param1);
	check_type(Type::tfloat, TC_DIST_FIRST_PARAM(dist->type));
	accept_cond(dist->param2);
	check_type(Type::tfloat, TC_DIST_SECOND_PARAM(dist->type));
    }
}

void ModelTC::visit(Location* loc) {
    assert(loc != nullptr);
    string &id = loc->id;
    if (is_global_scope()) {
	//check if already in global scope
	if (globals.find(id) == globals.end()) {
	    log->put_error(TC_ID_SCOPE(id));	      
	}
    }
    else {
	auto &local = current_scope->local_decls;
	//location could be local or global
	if (local.find(id) == local.end() &&
	    globals.find(id) == globals.end()) {
	    log->put_error(TC_ID_SCOPE(id));
	}  
    }

    if (loc->is_array_position()) {
	accept_cond(loc->index);
	check_type(Type::tint, TC_INDEX_INT(loc->id));
    }
    last_type = identifier_type(loc->id);
}

void ModelTC::visit(Exp* node) {
    (void) node;
    //this should not be called
    assert(false);
}

void ModelTC::visit(IConst* node) {
    (void) node;
    last_type = Type::tint;
}

void ModelTC::visit(BConst* node){
    (void) node;
    last_type = Type::tbool;
}

void ModelTC::visit(FConst* node){
    (void) node;
    last_type = Type::tfloat;
}

void ModelTC::visit(LocExp* locExp){
    assert(locExp != nullptr);
    accept_cond(locExp->location);
}

void ModelTC::visit(OpExp* exp){
    assert(exp != nullptr);
    Type res_type = Type::tunknown;
    if (exp->arity == Arity::one) {
	accept_cond(exp->left);
	res_type = operator_type(exp->bop, last_type);
	if (!log->has_errors() && res_type == Type::tunknown) {
	    log->put_error(TC_OP_FIRST_ARG(exp->bop));
	}
	last_type = res_type;
    } else if (exp->arity == Arity::two) {
	accept_cond(exp->left);
	res_type = operator_type(exp->bop, last_type);
	if (!log->has_errors() && res_type == Type::tunknown) {
	    log->put_error(TC_OP_FIRST_ARG(exp->bop));
	}
	Type fst_type = last_type;
	accept_cond(exp->right);
	res_type = operator_type(exp->bop, last_type);
	if (!log->has_errors() &&  res_type == Type::tunknown) {
	    log->put_error(TC_OP_SECOND_ARG(exp->bop));
	}
	Type snd_type = last_type;
	if (! ((fst_type <= snd_type)
	       || (snd_type <= fst_type))) {
	    //both types should be equal or subtypes (int->float)
	    log->put_error(TC_OP_SECOND_ARG(exp->bop));
	}
	last_type = res_type;
    }
}

ModelTC::~ModelTC() {
    for (auto &entry : scopes) {
	delete entry.second;
    }
    //current_scope should be pointed by scopes
    //destructor of scope should do nothing since
    //it does not have any owned resources.
}
