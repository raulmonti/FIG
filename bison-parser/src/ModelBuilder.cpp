#include <tuple>
#include <cassert>
#include "ModelBuilder.h"
#include "ExpEvaluator.h"
#include "ModelPrinter.h"

ModelBuilder::ModelBuilder() {};
ModelBuilder::~ModelBuilder() {};

inline const string mb_error_irr(const Type& type) {
    return (" not reducible to " + ModelPrinter::to_str(type) +
	    " at compilation time");
}

inline const string mb_error_dist_1(const string &clock_id) {
    return ("Fist distribution parameter of clock " + clock_id +
	    mb_error_irr(Type::tfloat));
}

inline const string mb_error_dist_2(const string &clock_id) {
    return ("Second distribution parameter of clock " + clock_id +
	    mb_error_irr(Type::tfloat));
}

inline const string mb_error_range_1(const string &var_id) {
    return ("Lower bound of range for " + var_id +
	    mb_error_irr(Type::tint));
}

inline const string mb_error_range_2(const string &var_id) {
	return ("Upper bound of range for " + var_id +
		mb_error_irr(Type::tint));
}

inline const string mb_error_init(const string &var_id, const Type& type) {
    return ("Initialization of " + var_id +
	    mb_error_irr(type));
}

inline void ModelBuilder::accept_cond(shared_ptr<ModelAST> node) {
    if (!has_errors()) {
	node->accept(*this);
    }
}

inline void ModelBuilder::accept_visitor(shared_ptr<ModelAST> node,
					 Visitor& visitor) {
    if (!has_errors()) {
	node->accept(visitor);
    }
}

inline int ModelBuilder::get_int_or_error(shared_ptr<Exp> exp,
					  const string &msg) {
    int res = 0;
    ExpEvaluator ev;
    accept_visitor(exp, ev);
    if (ev.has_type_int()) {
	res = ev.get_int();
    } else {
	put_error(msg); 
    }
    return (res);
}

inline float ModelBuilder::get_float_or_error(shared_ptr<Exp> exp,
					      const string &msg) {
    float res = 0;
    ExpEvaluator ev;
    accept_visitor(exp, ev);
    if (ev.has_type_float()) {
	res = ev.get_float();
    } else if (ev.has_type_int()) {
	res = (float) ev.get_int();
    } else {
	put_error(msg); 
    }
    return (res);
}

inline bool ModelBuilder::get_bool_or_error(shared_ptr<Exp> exp,
					    const string &msg) {
    bool res = 0;
    ExpEvaluator ev;
    accept_visitor(exp, ev);
    if (ev.has_type_bool()) {
	res = ev.get_bool();
    } else {
	put_error(msg); 
    }
    return (res);
}

void ModelBuilder::visit(shared_ptr<Model> model) {
    for (auto &entry : model->get_modules()) {
	const string &id = entry.first;
	current_scope = scopes[id];
	accept_cond(entry.second);
    }
}

void ModelBuilder::visit(shared_ptr<ModuleBody> body) {
    module_vars = make_unique<vector<Var>>();
    module_clocks = make_unique<vector<Clock>>();
    module_transitions = make_unique<vector<Transition>>();
    for (auto &decl : body->get_local_decls()) {
	accept_cond(decl);
    }
    for (auto &action : body->get_actions()) {
	accept_cond(action);
    }
}

Clock ModelBuilder::build_clock(const string& id) {
    shared_ptr<Dist> dist = current_scope->clock_dists[id];
    assert(dist != nullptr);
    fig::DistributionParameters params;
    //reduce distribution parameters
    if (dist->arity == Arity::one) {
	params[0] = get_float_or_error(dist->param1, mb_error_dist_1(id));
	for (unsigned int i = 1; i < params.size(); i++) {
	    params[i] = 0.0;
	}
    }
    if (dist->arity == Arity::two) {
	params[0] = get_float_or_error(dist->param1, mb_error_dist_1(id));
	params[1] = get_float_or_error(dist->param2, mb_error_dist_2(id));
	for (unsigned int i = 2; i < params.size(); i++) {
		params[i] = 0.0;
	}
    }
    //todo: constructor should accept the Distribution object directly, not the name.
    return Clock(id, ModelPrinter::to_str(dist->type), params);
}

void ModelBuilder::visit(shared_ptr<Decl> decl) {
    int lower;
    int upper;
    int init;
    Type type;
    if (decl->is_array()) {
	put_error("Arrays not yet supported");
    }
    if (decl->has_range()) {
	lower = get_int_or_error(decl->lower, mb_error_range_1(decl->id));
	upper = get_int_or_error(decl->upper, mb_error_range_2(decl->id));
	type = Type::tint;
    } else {
	lower = 0;
	upper = 1;
	type = Type::tbool;
    }
    if (decl->has_single_init()) {
	shared_ptr<Exp> iniexp = decl->inits.at(0);
	if (type == Type::tint) {
	    init = get_int_or_error(iniexp, mb_error_init(decl->id, type));
	} else if (type == Type::tbool) {
	    bool res = get_bool_or_error(iniexp, mb_error_init(decl->id, type));
	    init = res ? 1 : 0;
	} else {
	    throw_FigException("Not yet supported declaration type");
	}
	if (!has_errors()) {
	    const auto &var = make_tuple(decl->id, lower, upper, init);
	    assert(module_vars != nullptr);
	    module_vars->push_back(var);
	}
    }
    if (decl->type == Type::tclock) {
	module_clocks->push_back(build_clock(decl->id));
    }
}

Label build_label(const string &id, LabelType type) {
    switch(type) {
    case LabelType::in : return Label(id, false);
    case LabelType::out: return Label(id, true);
    case LabelType::commited: throw_FigException("Commited actions not yet supported");
    case LabelType::empty: return Label(id, true);
    default:
	throw_FigException("Unsupported label type");
    }	
}

void ModelBuilder::visit(shared_ptr<Action> action) {
    const string &label_id = action->id;
    LabelType label_type = action->type;
    Label label = build_label(label_id, label_type);
    //Transition constructor expects the id of the triggering clock,  let's get it:
    string t_clock =
	label_type == LabelType::in ? "" : current_scope->label_clocks[label_id];
    //Precondition
    ExpStringBuilder string_builder;
    action->guard->accept(string_builder);
    string result = string_builder.str();
    Precondition pre (result, string_builder.get_names());
    //Postcondition, to build the postcondition we need to visit the effects.
    transition_read_vars = make_unique<set<string>>();
    transition_write_vars = make_unique<vector<string>>();
    transition_update.str(std::string());
    transition_clocks = make_unique<set<string>>();
    auto &effects = action->get_effects();
    auto it = effects.begin();
    while (it != effects.end()) {
	(*it)->accept(*this);
	//avoid putting a ',' at the end:
	//we could also let the comma be and then delete it
	if ((it + 1) != effects.end()) {
	    const shared_ptr<Effect>& next = *(it + 1);
	    if (next->is_state_change()) {
		transition_update << ",";
	    }
	}
	it++;
    }
    const string &update = transition_update.str();
    Postcondition post (update, *transition_read_vars, *transition_write_vars);
    Transition transition (label, t_clock, pre, post, *transition_clocks);
    //Transition is ready!
    module_transitions->push_back(transition);
}

void ModelBuilder::visit(shared_ptr<Effect> effect) {
    if (effect->is_clock_reset()) {
	transition_clocks->insert(effect->loc->id);
    } else if (effect->is_state_change()) {
	ExpStringBuilder str_builder;
	effect->arg->accept(str_builder);
	const set<string> &names = str_builder.get_names();
	set<string>::iterator end = transition_read_vars->end();
	transition_read_vars->insert(names.cbegin(), names.cend());
	transition_write_vars->push_back(effect->loc->id);
	transition_update << str_builder.str();
    }
}

// ExpStringBuilder
void ExpStringBuilder::visit(shared_ptr<IConst> node) {
    result = std::to_string(node->value);
    should_enclose = false;
}

void ExpStringBuilder::visit(shared_ptr<BConst> node) {
    result = node->value ? "true" : "false";
    should_enclose = false;
}

void ExpStringBuilder::visit(shared_ptr<FConst> node) {
    result = std::to_string(node->value);
    should_enclose = false;
}

void ExpStringBuilder::visit(shared_ptr<LocExp> node) {
    if (node->location->is_array_position()) {
	throw_FigException("Array position are not yet supported");
    }
    result = node->location->id;
    names.insert(result);
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
    should_enclose = true;
}

string ExpStringBuilder::str() {
    return (result);
}

const set<string>& ExpStringBuilder::get_names() {
    return (names);
}
