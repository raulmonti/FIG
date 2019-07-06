//==============================================================================
//
//  ModelTC.h
//
//  Copyleft 2016-
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

#ifndef MODEL_TC_H
#define MODEL_TC_H

#include "Util.h"
#include "ModuleScope.h"
#include <utility>

using std::string;

/**
 * @brief A visitor to typecheck the AST. It will traverse the
 * AST finding type-errors, identifiers out of scope, duplicate
 * identifiers, etc.
 */
class ModelTC : public Visitor {
private:
    /// Shortcut for the static ModuleScope member.
    shared_map<string, ModuleScope> &scopes  = ModuleScope::scopes;

    /// Shortcut for the map of global variables.
    shared_map<string, Decl> &globals = ModuleScope::globals;

    /// The current module scope.
    // when traversing the AST it is necessary to keep track
    // of the current module scope to find idenfiers declarations
    shared_ptr<ModuleScope> current_scope;

    /// The type of the last node visited.
    Type last_type;

    /// Since expressions may have different types depending on
    /// the context, we need to check expressions according to
    /// the expected type.
    Type expected_exp_type;

    /// Is this visitor checking a property?
    /// This allows identifiers of expressions to occur on any module
    /// and not just in the current one.
    bool checking_property = false;

    /// Accepts if no errors
    void accept_cond(shared_ptr<ModelAST> module);

    /// Accept a expression with an expected type
    void accept_exp(Type expected, shared_ptr<Exp> exp);

    /// If the last inferred type is not "type", put an error
    void check_type(Type type, const string &msg);

    /// Check if every clock has a distribution type
    void check_clocks(shared_ptr<ModuleScope> scope);

    /// Check if the given expression is in DNF
    void check_dnf(PropType type, shared_ptr<Exp> exp);

    /// Find the type of the given identifier
    Type identifier_type(const string &id);

    /// Checking the global declarations (constants)
    bool is_global_scope() {
        return (current_scope == nullptr);
    }

    /// Check range and initialization of the declaration
    void check_ranges(shared_ptr<Decl> decl);
    void check_ranged_all(shared_ptr<ModuleScope> scope);

    /// Check if an identifier is in scope or redefined.
    void check_scope(shared_ptr<Decl> decl);

    /// Used to evaluate range bounds and distributions parameters
    int eval_int_or_put(shared_ptr<Exp> exp);
    float eval_float_or_put(shared_ptr<Exp> exp);


    /// Check array's declaration
    void check_array_size(shared_ptr<ArrayDecl> decl);
    void check_array_range(const std::string& id, shared_ptr<Ranged> decl);
    void check_array_init(const std::string& id,
                          Type expected, shared_ptr<Initialized> decl);
    void check_array_multiple_init(const std::string& id, Type expected,
                                   shared_ptr<MultipleInitialized> decl);

    /// Check an expression on its own instance of ModelTC but copy
    /// the inferred type to this instance
    /// @note used to typecheck expressions that are expected
    /// to fail on several ocations, and hence error messages
    /// should be ignored.
    bool dummy_check(Type expected_type, shared_ptr<Exp> exp);

public:
	ModelTC() : scopes(ModuleScope::scopes),
				globals(ModuleScope::globals),
				current_scope(nullptr),
				last_type(Type::tunknown),
				expected_exp_type(Type::tunknown),
				checking_property(false)
		{ /* Not much to do around here... */ }

    ModelTC(const ModelTC& instance) = default;

	~ModelTC() override {}

    /// Visitor functions
    void visit(shared_ptr<Model> node) override;
    void visit(shared_ptr<ModuleAST> node) override;
    void visit(shared_ptr<RangedDecl> node) override;
    void visit(shared_ptr<InitializedDecl> node) override;
    void visit(shared_ptr<RangedInitializedArray> node) override;
    void visit(shared_ptr<RangedMultipleInitializedArray> node) override;
    void visit(shared_ptr<InitializedArray> node) override;
    void visit(shared_ptr<MultipleInitializedArray> node) override;
    void visit(shared_ptr<ClockDecl> node) override;
    void visit(shared_ptr<TransitionAST> node) override;
	void visit(shared_ptr<PBranch> node) override;
	void visit(shared_ptr<Assignment> node) override;
    void visit(shared_ptr<ClockReset> node) override;
    void visit(shared_ptr<MultipleParameterDist> node) override;
    void visit(shared_ptr<SingleParameterDist> node) override;
    void visit(shared_ptr<Location> node) override;
    void visit(shared_ptr<ArrayPosition> node) override;
    void visit(shared_ptr<IConst> node) override;
    void visit(shared_ptr<BConst> node) override;
    void visit(shared_ptr<FConst> node) override;
    void visit(shared_ptr<LocExp> node) override;
    void visit(shared_ptr<BinOpExp> node) override;
    void visit(shared_ptr<UnOpExp> node) override;
    void visit(shared_ptr<TransientProp> node) override;
    void visit(shared_ptr<RateProp> node) override;
	void visit(shared_ptr<TBoundSSProp> node) override;
};


#endif
