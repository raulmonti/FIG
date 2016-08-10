#include "ModelTC.h"
#include "ModuleInstance.h"
#include "State.h"

using std::shared_ptr;
using std::unique_ptr;
using fig::ModuleInstance;
using Vars = fig::State<fig::STATE_INTERNAL_TYPE>;
using Var  = fig::VariableDefinition<fig::STATE_INTERNAL_TYPE>;

class ModelBuilder : public Visitor {
private:
    void accept_visitor(shared_ptr<ModelAST> node, Visitor& visitor);
    int get_int_or_error(shared_ptr<Exp> exp, const string &msg);
    bool get_bool_or_error(shared_ptr<Exp> exp, const string &msg);
    void accept_cond(shared_ptr<ModelAST> node);
    shared_ptr<ModuleInstance> current_module;
    unique_ptr<vector<Var>> module_vars;

public:
    ModelBuilder();
    virtual ~ModelBuilder();
    
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
