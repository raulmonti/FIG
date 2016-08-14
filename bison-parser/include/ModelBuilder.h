#include <sstream>
#include <set>

#include "ModelTC.h"
#include "ModuleInstance.h"
#include "Util.h"
#include "State.h"
#include "Clock.h"
#include "Transition.h"
#include "Label.h"

using std::string;
using std::shared_ptr;
using std::unique_ptr;
using fig::ModuleInstance;
using Vars = fig::State<fig::STATE_INTERNAL_TYPE>;
using Var  = fig::VariableDefinition<fig::STATE_INTERNAL_TYPE>;
using fig::Clock;
using fig::Transition;
using fig::Label;
using fig::Precondition;
using fig::Postcondition;
using std::set;

class ModelBuilder : public Visitor {
private:
    shared_map<string, ModuleScope> &scopes  = ModuleScope::scopes;
    shared_ptr<ModuleInstance> current_module;
    unique_ptr<vector<Var>> module_vars;
    unique_ptr<vector<Clock>> module_clocks;
    unique_ptr<vector<Transition>> module_transitions;
    shared_ptr<ModuleScope> current_scope;
    //variables needed to compute the value of update expressions:
    unique_ptr<set<string>> transition_read_vars;
    //variables that change after an update
    unique_ptr<vector<string>> transition_write_vars;
    //comma separated expressions strings to be evaluated
    stringstream transition_update;
    //clocks reseted during update
    unique_ptr<set<string>> transition_clocks;
    void accept_visitor(shared_ptr<ModelAST> node, Visitor& visitor);
    int get_int_or_error(shared_ptr<Exp> exp, const string &msg);
    bool get_bool_or_error(shared_ptr<Exp> exp, const string &msg);
    float get_float_or_error(shared_ptr<Exp> exp, const string &msg);
    void accept_cond(shared_ptr<ModelAST> node);
    Clock build_clock(const string &clock_id);
public:
    ModelBuilder();
    virtual ~ModelBuilder();
    
    void visit(shared_ptr<Model> node);
    void visit(shared_ptr<ModuleBody> node);
    void visit(shared_ptr<Decl> node);
    void visit(shared_ptr<Action> node);
    void visit(shared_ptr<Effect> node);
};

class ExpStringBuilder : public Visitor {
    set<string> names;
    bool should_enclose;
    std::string result;
public:
    ExpStringBuilder() : should_enclose {false}, result {""} {};
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
    const set<string>& get_names();
    string str();
};
