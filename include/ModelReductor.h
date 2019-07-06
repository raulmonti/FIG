//==============================================================================
//
//  ModelReductor.h
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

#ifndef MODEL_REDUCTOR_H
#define MODEL_REDUCTOR_H

#include "ModelAST.h"
#include "ModuleScope.h"
#include "Operators.h"


/**
 * @brief This visitor traverses an AST reducing as much
 * as possible every expression that appear on it.
 * @note It will essentially call \ref ExpReductor on every expression on the AST.
 * @note This class also computes every array size and initializations.
 */

class ModelReductor : public Visitor {

	float probabilisticWeightAcc;
private:
    /// The scope of the module being reduced
    shared_ptr<ModuleScope> current_scope = nullptr;

    /// Call \ref ExpReductor on the given expression an return the result.
    shared_ptr<Exp> reduce(shared_ptr<Exp> node);

    /// Reduce a sequence of expressions
    void reduce_vector(shared_vector<Exp> &vector);

    /// Some auxiliary functions to compute arrays limits.
    //helpers to infer arrays data
    void compute_int(int &to, shared_ptr<Exp> size);
    //reduce array expressions
    void reduce_size(shared_ptr<ArrayDecl> decl);
    void reduce_range(shared_ptr<Ranged> decl);
    void reduce_init(shared_ptr<Initialized> decl);
    void reduce_multiple_init(shared_ptr<MultipleInitialized> decl);
    void check_data(const ArrayData &data);


public:
    ModelReductor() {}
	~ModelReductor() override {}
    void accept_cond(shared_ptr<ModelAST>);
	void visit(shared_ptr<Model>) override;
	void visit(shared_ptr<ModuleAST>) override;
	void visit(shared_ptr<TransientProp>) override;
	void visit(shared_ptr<RateProp>) override;
	void visit(shared_ptr<TBoundSSProp>) override;
	// Declarations;
	void visit(shared_ptr<InitializedDecl>) override;
	void visit(shared_ptr<RangedDecl>) override;
	void visit(shared_ptr<ClockDecl>) override;
	void visit(shared_ptr<ArrayDecl>) override;
	void visit(shared_ptr<InitializedArray>) override;
	void visit(shared_ptr<MultipleInitializedArray>) override;
	void visit(shared_ptr<RangedInitializedArray>) override;
	void visit(shared_ptr<RangedMultipleInitializedArray>) override;
	//Transitions;
	void visit(shared_ptr<TransitionAST>) override;
	//Effects
	void visit(shared_ptr<PBranch>) override;
	void visit(shared_ptr<Assignment>) override;
	void visit(shared_ptr<ClockReset>) override;
	//Distributions
	void visit(shared_ptr<SingleParameterDist>) override;
	void visit(shared_ptr<MultipleParameterDist>) override;
    //Locations
    //void visit(shared_ptr<ArrayPosition>);
};

#endif
