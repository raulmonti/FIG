#include "ModelTC.h"
#include "ModuleInstance.h"
#include "State.h"
#include "Clock.h"

using std::shared_ptr;
using std::unique_ptr;
using fig::ModuleInstance;
using Vars = fig::State<fig::STATE_INTERNAL_TYPE>;
using Var  = fig::VariableDefinition<fig::STATE_INTERNAL_TYPE>;
using fig::Clock;

class ModelBuilder : public Visitor {
private:
    shared_map<string, ModuleScope> &scopes  = ModuleScope::scopes;
    void accept_visitor(shared_ptr<ModelAST> node, Visitor& visitor);
    int get_int_or_error(shared_ptr<Exp> exp, const string &msg);
    bool get_bool_or_error(shared_ptr<Exp> exp, const string &msg);
    float get_float_or_error(shared_ptr<Exp> exp, const string &msg);
    void accept_cond(shared_ptr<ModelAST> node);
    Clock build_clock(const string &clock_id);
    shared_ptr<ModuleInstance> current_module;
    unique_ptr<vector<Var>> module_vars;
    unique_ptr<vector<Clock>> module_clocks;
    shared_ptr<ModuleScope> current_scope;

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