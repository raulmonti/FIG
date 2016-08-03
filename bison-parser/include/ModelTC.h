#ifndef MODEL_TC_H
#define MODEL_TC_H

#include "ModelAST.h"

using std::string;
using namespace ASTNode;

struct ModuleScope {
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
    map<string, ModuleScope *> scopes;
    map<string, Decl *> globals;
    ModuleScope *current_scope;
    Log *log;
    //accepts if no errors
    void accept_cond(ModelAST *module);
    //prefix for log message
    string get_prefix();
    bool is_global_scope() {
	return (current_scope == nullptr);
    }
public:
    ModelTC(Log *log) : current_scope {nullptr}, log {log} {};
    
    void visit(ModelAST* node);
    void visit(Model* node);
    void visit(ModuleBody* node);
    void visit(Decl* node);
    void visit(Action* node);
    void visit(Effect* node);
    void visit(Dist* node);
    void visit(Location* node);
    void visit(Exp* node);
    void visit(IConst* node);
    void visit(BConst* node);
    void visit(FConst* node);
    void visit(LocExp* node);
    void visit(OpExp* node);

    ~ModelTC() {};
};

#endif
