#include <cassert>
#include "ModelBuilder.h"
#include "ExpEvaluator.h"


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
	accept_cond(entry.second);
    }
}

void ModelBuilder::visit(shared_ptr<ModuleBody> body) {
    for (auto &decl : body->get_local_decls()) {
	module_vars = make_unique<vector<Var>>();
	accept_cond(decl);
    }
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
	    std::cout << "PUTTING VARIABLE " << decl->id << " LOWER : " << lower << " UPPER : " << upper << " INIT: " << init << std::endl;
	    const auto &var = make_tuple(decl->id, lower, upper, init);
	    assert(module_vars != nullptr);
	    module_vars->push_back(var);
	}
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
