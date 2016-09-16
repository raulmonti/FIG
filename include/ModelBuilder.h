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
using Vars = fig::State<fig::STATE_INTERNAL_TYPE>;
using Var  = fig::VariableDefinition<fig::STATE_INTERNAL_TYPE>;
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

namespace fig { class JaniTranslator; }  // Fwd dec. for friendship

/**
 * @brief This class processes a ModelAST object and builds
 * a model using the ModelSuite API.
 */
class ModelBuilder : public Visitor {

	friend class fig::JaniTranslator;

private:
    /// Alias for the global instance of the model.
    ModelSuite &model_suite = ModelSuite::get_instance();

    /// Alias for the map that assigns
    shared_map<string, ModuleScope> &scopes  = ModuleScope::scopes;

    /// The current module in construction.
    shared_ptr<ModuleInstance> current_module;

    /// The variables of the current module in construction
    unique_ptr<vector<Var>> module_vars;

    /// The clocks declared locally in the current module
    unique_ptr<vector<Clock>> module_clocks;

    /// The transitions of the current module
    unique_ptr<vector<Transition>> module_transitions;

    /// Map of input-enabled preconditions.
    /// @note This map is used to construct implicit transitions
    /// to be added in the current module. This transitions are implicit
    /// since they were not declared by the user. Each of the input labels of
    /// each module have an associated implicit transition that is enabled
    /// when the negation of all the explicit preconditions is valid. The
    /// key of the map is the label, the pair has the string representation of the
    /// precondition and a vector of names that appear on it.
    map<string, pair<string, vector<string>>> module_ie_pre;

    /// The symbol table of the current module in construction.
    shared_ptr<ModuleScope> current_scope;

    /// Variables needed to compute the value of update expressions
    /// of the transition being constructed at the time.
    /// @example (q' = 1 + r + p & p' = 5 + s) will give [r,p,s]
    unique_ptr<vector<string>> transition_read_vars;

    /// Variables that change after an update
    /// @example the above example will give [q,p]
    unique_ptr<vector<string>> transition_write_vars;

    /// Comma separated expressions strings to be evaluated
    /// @example the above example will give "1 + r + p, 5 + s"
    stringstream transition_update;

    /// The clocks reseted by the transition in construction.
    unique_ptr<set<string>> transition_clocks;

    /// Accept only if there is no error messages.
    void accept_visitor(shared_ptr<ModelAST> node, Visitor& visitor);

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

    /// Accept only if there is no error messages.
    void accept_cond(shared_ptr<ModelAST> node);

    /// Build a clock with the given id.
    Clock build_clock(const string &clock_id);

    /// Update the "module_ie_pre" variable
    /// @see ModelBuilder::model_ie_pre
    void update_module_ie(shared_ptr<Action> action);

    /// Build and add and implicit transition
    /// @see ModelBuilder::model_ie_pre
    void build_input_enabled();

public:
    ModelBuilder();
    virtual ~ModelBuilder();
    /// @note Property id (see Property::get_id) -> ast that generated it.
    /// the ast is needed since the property is projected several times
    /// to different set of variables.
    static map<int, shared_ptr<Prop>> property_ast;

    void visit(shared_ptr<Model> node);
    void visit(shared_ptr<ModuleBody> node);
    void visit(shared_ptr<Decl> node);
    void visit(shared_ptr<Action> node);
    void visit(shared_ptr<Effect> node);
    void visit(shared_ptr<Prop> node);
};


#endif
