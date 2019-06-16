//==============================================================================
//
//  ModelAST.h
//
//  Copyright 2016-
//  Authors:
//  - Leonardo Rodríguez (Universidad Nacional de Córdoba)
//  - Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
//
//------------------------------------------------------------------------------
//
//  This file is part of FIG.
//
//  The Finite Improbability Generator (FIG) project is free software;
//  you can redistribute it and/or modify it under the terms of the GNU
//  General Public License as published by the Free Software Foundation;
//  either version 3 of the License, or (at your option) any later version.
//
//  FIG is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with FIG; if not, write to the Free Software Foundation,
//	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//==============================================================================

#ifndef MODELBUILDER_H
#define MODELBUILDER_H

#include <sstream>
#include <set>

#include <ModelTC.h>
#include <ModelSuite.h>

using std::string;
using Vars  = fig::State<fig::STATE_INTERNAL_TYPE>;
using Var   = fig::VariableDefinition<fig::STATE_INTERNAL_TYPE>;
using ArrayEntries = fig::State<fig::STATE_INTERNAL_TYPE>;
using Array = std::pair<std::string, ArrayEntries>;
using fig::Clock;
using fig::Transition;

/**
 * @brief This class processes a ModelAST object and builds
 * a model using the ModelSuite API.
 */
class ModelBuilder : public Visitor {
private:
    /// Alias for the global instance of the model.
	fig::ModelSuite &model_suite = fig::ModelSuite::get_instance();

    /// Alias for the module scopes map
    shared_map<string, ModuleScope> &scopes  = ModuleScope::scopes;

    /// The current module in construction.
	std::shared_ptr<fig::ModuleInstance> current_module;

    /// The variables of the current module in construction
	std::unique_ptr<vector<Var>> module_vars;

    /// The arrays of the current module in construction
	std::unique_ptr<vector<Array>> module_arrays;

    /// The clocks declared locally in the current module
	std::unique_ptr<vector<Clock>> module_clocks;

    /// The transitions of the current module
	std::unique_ptr<vector<Transition>> module_transitions;

    /// The symbol table of the current module in construction.
	std::shared_ptr<ModuleScope> current_scope;

    /// The clocks reseted by the transition in construction.
	std::unique_ptr<std::set<string>> transition_clocks;

    /// Accept only if there is no error message.
	void accept_visitor(std::shared_ptr<ModelAST> node, Visitor& visitor);

    /// Accept only if there is no error message.
	void accept_cond(std::shared_ptr<ModelAST> node);

    /// Build a clock with the given id.
    Clock build_clock(const string &clock_id);

    /// Try to evaluate an expression or put an error message if it
    /// was not possible to reduce it.
    /// @note Some expressions are expected to depend only on global
    /// constants, not on state variables (e.g the range of a variable,
    /// the parameter of a distribution). If they depend on a state, then
    /// this function will store an error message in the inherited member
    /// "message" of class Visitor.
	int get_int_or_error(std::shared_ptr<Exp> exp, const string &msg);

    /// @copydoc ModelBuilder::get_int_or_error
	bool get_bool_or_error(std::shared_ptr<Exp> exp, const string &msg);

    /// @copydoc ModelBuilder::get_int_or_error
	float get_float_or_error(std::shared_ptr<Exp> exp, const string &msg);

public:

	ModelBuilder();
    virtual ~ModelBuilder();

    /// @note maps Property id (see Property::get_id) -> ast that generated it.
    /// the ast is needed since the property is projected several times
    /// to different set of variables.
	static map<int, std::shared_ptr<Prop>> property_ast;

	void visit(std::shared_ptr<Model>           node);
	void visit(std::shared_ptr<ModuleAST>       node);
	void visit(std::shared_ptr<RangedDecl>      node);
	void visit(std::shared_ptr<ClockDecl>       node);
	void visit(std::shared_ptr<InitializedDecl> node);
	void visit(std::shared_ptr<ArrayDecl>       node);
	void visit(std::shared_ptr<TransitionAST>   node);
	void visit(std::shared_ptr<Assignment>      node);
	void visit(std::shared_ptr<ClockReset>      node);
	void visit(std::shared_ptr<TransientProp>   node);
	void visit(std::shared_ptr<RateProp>        node);
	void visit(std::shared_ptr<TBoundSSProp>    node);
};


#endif
