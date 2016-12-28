/* Leonardo Rodríguez */

#ifndef MODELBUILDER_H
#define MODELBUILDER_H

#include <sstream>
#include <set>

#include "ModelTC.h"
#include "ModuleInstance.h"
#include "Util.h"
#include "State.h"
#include "Clock.h"
#include "Transition.h"
#include "Label.h"
#include "Property.h"
#include "PropertyTransient.h"
#include "PropertyRate.h"
#include "ModelSuite.h"

using std::string;
using std::shared_ptr;
using std::unique_ptr;
using fig::ModuleInstance;
using Vars  = fig::State<fig::STATE_INTERNAL_TYPE>;
using Var   = fig::VariableDefinition<fig::STATE_INTERNAL_TYPE>;
using ArrayEntries = fig::State<fig::STATE_INTERNAL_TYPE>;
using Array = std::pair<std::string, ArrayEntries>;
using fig::Clock;
using fig::Transition;
using fig::Label;
using fig::Precondition;
using fig::Postcondition;
using fig::Property;
using fig::PropertyRate;
using fig::PropertyTransient;
using fig::ModuleInstance;
using fig::ModelSuite;
using fig::State;
using std::set;
using std::pair;

/**
 * @brief This class processes a ModelAST object and builds
 * a model using the ModelSuite API.
 */
class ModelBuilder : public Visitor {
private:
    /// Alias for the global instance of the model.
    ModelSuite &model_suite = ModelSuite::get_instance();

    /// Alias for the module scopes map
    shared_map<string, ModuleScope> &scopes  = ModuleScope::scopes;

    /// The current module in construction.
    shared_ptr<ModuleInstance> current_module;

    /// The variables of the current module in construction
    unique_ptr<vector<Var>> module_vars;

    /// The arrays of the current module in construction
    unique_ptr<vector<Array>> module_arrays;

    /// The clocks declared locally in the current module
    unique_ptr<vector<Clock>> module_clocks;

    /// The transitions of the current module
    unique_ptr<vector<Transition>> module_transitions;

    /// The symbol table of the current module in construction.
    shared_ptr<ModuleScope> current_scope;

    /// The clocks reseted by the transition in construction.
    unique_ptr<set<string>> transition_clocks;

    /// Accept only if there is no error message.
    void accept_visitor(shared_ptr<ModelAST> node, Visitor& visitor);

    /// Accept only if there is no error message.
    void accept_cond(shared_ptr<ModelAST> node);

    /// Build a clock with the given id.
    Clock build_clock(const string &clock_id);

    /// Try to evaluate an expression or put an error message if it
    /// was not possible to reduce it.
    /// @note Some expressions are expected to depend only on global
    /// constants, not on state variables (e.g the range of a variable,
    /// the parameter of a distribution). If they depend on a state, then
    /// this function will store an error message in the inherited member
    /// "message" of class Visitor.
    int get_int_or_error(shared_ptr<Exp> exp, const string &msg);

    /// @copydoc ModelBuilder::get_int_or_error
    bool get_bool_or_error(shared_ptr<Exp> exp, const string &msg);

    /// @copydoc ModelBuilder::get_int_or_error
    float get_float_or_error(shared_ptr<Exp> exp, const string &msg);

public:
    ModelBuilder();
    virtual ~ModelBuilder();
    /// @note maps Property id (see Property::get_id) -> ast that generated it.
    /// the ast is needed since the property is projected several times
    /// to different set of variables.
    static map<int, shared_ptr<Prop>> property_ast;

    void visit(shared_ptr<Model> node);
    void visit(shared_ptr<ModuleAST> node);
    void visit(shared_ptr<RangedDecl> node);
    void visit(shared_ptr<ClockDecl> node);
    void visit(shared_ptr<InitializedDecl> node);
    void visit(shared_ptr<ArrayDecl> node);
    void visit(shared_ptr<TransitionAST> node);
    void visit(shared_ptr<Assignment> node);
    void visit(shared_ptr<ClockReset> node);
    void visit(shared_ptr<TransientProp> node);
    void visit(shared_ptr<RateProp> node);
};


#endif
