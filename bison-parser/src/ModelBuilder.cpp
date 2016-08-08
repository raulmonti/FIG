#include "ModelBuilder.h"
#include "ExpEvaluator.h"


ModelBuilder::ModelBuilder() {};
ModelBuilder::~ModelBuilder() {};

inline void ModelBuilder::accept_cond(ModelAST *node) {
    if (!log->has_errors()) {
	node->accept(*this);
    }
}

inline void ModelBuilder::accept_visitor(ModelAST *node, Visitor& visitor) {
    if (!log->has_errors()) {
	node->accept(visitor);
    }
}

inline int ModelBuilder::get_int_or_error(Exp *exp, const string &msg) {
    int res = 0;
    ExpEvaluator ev;
    accept_visitor(exp, ev);
    if (ev.has_type_int()) {
	res = ev.get_int();
    } else {
	log->put_error(msg + " not reducible to [int] at compilation time"); 
    }
    return (res);
}

inline bool ModelBuilder::get_bool_or_error(Exp *exp, const string &msg) {
    bool res = 0;
    ExpEvaluator ev;
    accept_visitor(exp, ev);
    if (ev.has_type_bool()) {
	res = ev.get_bool();
    } else {
	log->put_error(msg + " not reducible to [bool] at compilation time"); 
    }
    return (res);
}

void ModelBuilder::visit(Model* model) {
    for (auto &entry : model->get_modules()) {
	accept_cond(entry.second);
    }
}

void ModelBuilder::visit(ModuleBody* body) {
    for (auto &decl : body->get_local_decls()) {
	accept_cond(decl);
	module_vars = new vector<Var>{};
    }
}

void ModelBuilder::visit(Decl* decl) {
    int lower;
    int upper;
    int init;
    Type type;
    if (decl->is_array()) {
	log->put_error("Arrays not yet supported");
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
	Exp *iniexp = decl->inits.at(0);
	if (type == Type::tint) {
	    init = get_int_or_error(iniexp, "Initialization of " + decl->id);
	} else if (type == Type::tbool) {
	    bool res = get_bool_or_error(iniexp, "Initialization of " + decl->id);
	    init = res ? 1 : 0;
	} else {
	    throw_FigException("Not yet supported declaration type");
	}
	if (!log->has_errors()) {
	    std::cout << "PUTTING VARIABLE " << decl->id << " LOWER : " << lower << " UPPER : " << upper << " INIT: " << init << std::endl;
	    const auto &var = make_tuple(decl->id, lower, upper, init);
	    module_vars->push_back(var);
	}
    }
}

void ModelBuilder::visit(Action* node) {
    (void) node;
}

void ModelBuilder::visit(Effect* node) {
    (void) node;
}

void ModelBuilder::visit(Dist* node) {
    (void) node;
}

void ModelBuilder::visit(Location* node) {
    (void) node;
}

void ModelBuilder::visit(IConst* node) {
    (void) node;
}

void ModelBuilder::visit(BConst* node) {
    (void) node;
}

void ModelBuilder::visit(FConst* node) {
    (void) node;
}

void ModelBuilder::visit(LocExp* node) {
    (void) node;
}

void ModelBuilder::visit(OpExp* node) {
    (void) node;
}
