#include "ModelTC.h"
#include "ModuleInstance.h"
#include "State.h"

using fig::ModuleInstance;
using Vars = fig::State<fig::STATE_INTERNAL_TYPE>;
using Var  = fig::VariableDefinition<fig::STATE_INTERNAL_TYPE>;

class ModelBuilder : public Visitor {
private:
    void accept_visitor(ModelAST *node, Visitor& visitor);
    int get_int_or_error(Exp *exp, const string &msg);
    bool get_bool_or_error(Exp *exp, const string &msg);
    void accept_cond(ModelAST *node);
    ModuleInstance *current_module;
    vector<Var> *module_vars;
    Log *log = &Log::get_instance();
public:
    ModelBuilder();
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
    ~ModelBuilder();
};
