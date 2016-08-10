#include <tuple>
#include <cassert>
#include "ModelBuilder.h"
#include "ExpEvaluator.h"
#include "ModelPrinter.h"

ModelBuilder::ModelBuilder() {};
ModelBuilder::~ModelBuilder() {
};

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
	put_error(msg + " not reducible to [int] at compilation time"); 
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
	put_error(msg + " not reducible to [float] at compilation time"); 
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
	put_error(msg + " not reducible to [bool] at compilation time"); 
    }
    return (res);
}

void ModelBuilder::visit(shared_ptr<Model> model) {
    for (auto &entry : model->get_modules()) {
	const string &id = entry.first;
	std::cout << "MODULE " << id << std::endl;
	current_scope = scopes[id];
	accept_cond(entry.second);
    }
}

void ModelBuilder::visit(shared_ptr<ModuleBody> body) {
    std::cout << "LOCAL DECL :" << std::endl;
    for (auto &decl : body->get_local_decls()) {
	module_vars = make_unique<vector<Var>>();
	module_clocks = make_unique<vector<Clock>>();
	accept_cond(decl);
	for (Var var : *module_vars) {
	    std::cout << "Nombre: "  + std::get<0>(var);
	    std::cout << " Minimo: " + std::to_string(std::get<1>(var));
	    std::cout << " Maximo: " + std::to_string(std::get<2>(var));
	    std::cout << " Init:   " + std::to_string(std::get<3>(var));
	    std::cout << std::endl;
	}
	for (Clock &clock : *module_clocks) {
	    std::cout << "Nombre: "  + clock.name();
	    std::cout << "Dist: "   + clock.dist_name();
	    std::cout << std::endl;
	    fig::DistributionParameters params = clock.distribution_params();
	    for (unsigned int i = 0; i < params.size(); i++) {
		std::cout << "Param " << params[i] << " ";
	    }
	    std::cout << std::endl;
	}
    }
}

Clock ModelBuilder::build_clock(const string& id) {
    shared_ptr<Dist> dist = current_scope->clock_dists[id];
    assert(dist != nullptr);
    fig::DistributionParameters params;
    //reduce distribution parameters
    if (dist->arity == Arity::one) {
	params[0] = get_float_or_error(dist->param1,
				       "Parameter of distribution for clock " + id);
	for (unsigned int i = 1; i < params.size(); i++) {
	    params[i] = 0.0;
	}
    }
    if (dist->arity == Arity::two) {
	params[0] = get_float_or_error(dist->param1,
				       "First parameter of distribution for clock " + id);
	params[1] = get_float_or_error(dist->param2,
				       "Second parameter of distribution for clock " + id);
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
	string msg = "Lower bound error of " + decl->id;
	lower = get_int_or_error(decl->lower, msg);
	msg  =  "Upper bound error of " + decl->id;
	upper = get_int_or_error(decl->upper, msg);
	type = Type::tint;
    } else {
	lower = 0;
	upper = 1;
	type = Type::tbool;
    }
    if (decl->has_single_init()) {
	shared_ptr<Exp> iniexp = decl->inits.at(0);
	if (type == Type::tint) {
	    init = get_int_or_error(iniexp, "Initialization of " + decl->id);
	} else if (type == Type::tbool) {
	    bool res = get_bool_or_error(iniexp, "Initialization of " + decl->id);
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

void ModelBuilder::visit(shared_ptr<Action> node) {
    (void) node;
}

void ModelBuilder::visit(shared_ptr<Effect> node) {
    (void) node;
}

void ModelBuilder::visit(shared_ptr<Dist> node) {
    (void) node;
}

void ModelBuilder::visit(shared_ptr<Location> node) {
    (void) node;
}

void ModelBuilder::visit(shared_ptr<IConst> node) {
    (void) node;
}

void ModelBuilder::visit(shared_ptr<BConst> node) {
    (void) node;
}

void ModelBuilder::visit(shared_ptr<FConst> node) {
    (void) node;
}

void ModelBuilder::visit(shared_ptr<LocExp> node) {
    (void) node;
}

void ModelBuilder::visit(shared_ptr<OpExp> node) {
    (void) node;
}
