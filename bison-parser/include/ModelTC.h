#ifndef MODEL_TC_H
#define MODEL_TC_H

#include "ModelAST.h"
#include "Util.h"
#include <utility>


/** Typechecking on the Model AST */

using std::string;

struct ModuleScope {
    //Note: Pointers here are owned by ModelAST
    static map<string, ModuleScope *> scopes;
    static map<string, Decl *> globals;
    string id;
    ModuleBody *body;
    //labels to type
    map<string, LabelType> labels;
    //clock assigned to a label : label->clock
    map<string, string> label_clocks;
    //distribution of a clock : clock->dist
    map<string, Dist*> clock_dists;
    //local declarations : id -> decl (type, range, init, ...)
    map<string, Decl*> local_decls;
};

class ModelTC : public Visitor {
private:
    map<string, ModuleScope *> &scopes  = ModuleScope::scopes;
    map<string, Decl *> &globals = ModuleScope::globals;
    
    ModuleScope *current_scope;
    Type last_type;
    //accepts if no errors
    void accept_cond(ModelAST *module);
    //prefix for log message
    void check_type(Type type, const string &msg);
    Type identifier_type(const string &id);
    bool is_global_scope() {
	return (current_scope == nullptr);
    }
    // result type of a operator given the type of its arguments
    // may be Type::tunknown
    static Type operator_type(const ExpOp &id, Type arg);
    Log *log;
public:
    ModelTC() : current_scope {nullptr},
		last_type {Type::tunknown}, log {&Log::get_instance()} {};
    void visit(Model* node);
    void visit(ModuleBody* node);
    void visit(Decl* node);
    void visit(Action* node);
    void visit(Effect* node);
    void visit(Dist* node);
    void visit(Location* node);
    void visit(IConst* node);
    void visit(BConst* node);
    void visit(FConst* node);
    void visit(LocExp* node);
    void visit(OpExp* node);
    
    ~ModelTC();
};

#endif
