#ifndef MODEL_TC_H
#define MODEL_TC_H

#include "ModelAST.h"
#include "Util.h"
#include <utility>


/** Typechecking on the Model AST */

using std::string;

struct ModuleScope {
    //map of shared pointers
    static shared_map<string, ModuleScope> scopes;
    static shared_map<string, Decl> globals;
    string id;
    shared_ptr<ModuleBody> body;
    //labels to type
    map<string, LabelType> labels;
    //clock assigned to a label : label->clock
    map<string, string> label_clocks;
    //distribution of a clock : clock->dist
    shared_map<string, Dist> clock_dists;
    //local declarations : id -> decl (type, range, init, ...)
    shared_map<string, Decl> local_decls;
};

class ModelTC : public Visitor {
private:
    shared_map<string, ModuleScope> &scopes  = ModuleScope::scopes;
    shared_map<string, Decl> &globals = ModuleScope::globals;
    shared_ptr<ModuleScope> current_scope;
    Type last_type;
    //accepts if no errors
    void accept_cond(shared_ptr<ModelAST> module);
    //prefix for log message
    void check_type(Type type, const string &msg);
    void check_clocks(shared_ptr<ModuleScope> scope);
    Type identifier_type(const string &id);
    bool is_global_scope() {
	return (current_scope == nullptr);
    }
    // result type of a operator given the type of its arguments
    // may be Type::tunknown
    static Type operator_type(const ExpOp &id, Type arg);
public:
    ModelTC() : current_scope {nullptr},
		last_type {Type::tunknown}{};
    virtual ~ModelTC();
    void visit(shared_ptr<Model> node);
    void visit(shared_ptr<ModuleBody> node);
    void visit(shared_ptr<Decl> node);
    void visit(shared_ptr<Action> node);
    void visit(shared_ptr<Effect> node);
    void visit(shared_ptr<Dist> node);
    void visit(shared_ptr<Location> node);
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
};

#endif
