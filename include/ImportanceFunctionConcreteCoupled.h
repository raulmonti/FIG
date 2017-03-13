//==============================================================================
//
//  ImportanceFunctionConcreteCoupled.h
//
//  Copyleft 2015-
//  Authors:
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


#ifndef IMPORTANCEFUNCTIONCONCRETECOUPLED_H
#define IMPORTANCEFUNCTIONCONCRETECOUPLED_H

#include <ImportanceFunctionConcrete.h>
#include <State.h>
#include <FigException.h>


namespace fig
{

/**
 * @brief ImportanceFunction for the concrete importance assessment
 *        of a fully coupled ModuleNetwork.
 *
 *        Assesses the importance of the concrete state space resulting from
 *        the parallel composition of all the modules in the system,
 *        viz. the "coupled" view of the user model.
 *        This requires a ModuleNetwork with the global symbolic state,
 *        i.e. the memory-contiguous join of all the \ref State "states"
 *        of the \ref ModuleInstance "module instances" forming the network.
 *
 * @see ImportanceFunction
 * @see ImportanceFunctionConcrete
 * @see ImportanceFunctionConcreteSplit
 */
class ImportanceFunctionConcreteCoupled : public ImportanceFunctionConcrete
{
	/// User's \ref ModuleNetwork "system model", i.e. the network of modules,
	/// needed for the "auto" strategy
	const ModuleNetwork& model_;

	/// Single location used from ImportanceFunctionConcrete::
	const unsigned importanceInfoIndex_;

public:  // Ctor/Dtor

	/// @brief Data ctor
	/// @param model System model, whose current state is taken as initial
	ImportanceFunctionConcreteCoupled(const ModuleNetwork& model);

	/// Dtor
	virtual ~ImportanceFunctionConcreteCoupled();

	/// Avoid accidental copies
	ImportanceFunctionConcreteCoupled(const ImportanceFunctionConcreteCoupled&) = delete;

	/// Avoid accidental copies
	ImportanceFunctionConcreteCoupled&
	operator=(const ImportanceFunctionConcreteCoupled&) = delete;

public:  // Accessors

	inline bool concrete_simulation() const noexcept override final { return true; }

	/// @copydoc ImportanceFunctionConcrete::info_of()
	/// @note Attempted inline in a desperate need for speed
	/// @note <b>Complexity:</b> <i>O(size(state)<sup>2</sup>)</i>
	inline ImportanceValue info_of(const StateInstance& state) const override
		{
#       ifndef NDEBUG
			if (!has_importance_info())
				throw_FigException("importance function \"" + name() + "\" "
								   "doesn't hold importance information.");
			globalStateCopy.copy_from_state_instance(state, true);
#       else
			globalStateCopy.copy_from_state_instance(state, false);
#       endif
			auto info = modulesConcreteImportance[importanceInfoIndex_]
												 [globalStateCopy.encode()];
			return ready() ? (MASK(info) | level_of(UNMASK(info)))
						   : info;
		}

	/// @copydoc ImportanceFunction::importance_of()
	/// @note Attempted inline in a desperate need for speed
	/// @note <b>Complexity:</b> <i>O(size(state)<sup>2</sup>)</i>
	inline ImportanceValue importance_of(const StateInstance& state) const override
		{
#       ifndef NDEBUG
			if (!has_importance_info())
				throw_FigException("importance function \"" + name() + "\" "
								   "doesn't hold importance information.");
			globalStateCopy.copy_from_state_instance(state, true);
#       else
			globalStateCopy.copy_from_state_instance(state, false);
#       endif
			return UNMASK(modulesConcreteImportance[importanceInfoIndex_]
												   [globalStateCopy.encode()]);
		}

	void print_out(std::ostream& out,
	               State<STATE_INTERNAL_TYPE> s = State<STATE_INTERNAL_TYPE>()) const override;

public:  // Utils

	void assess_importance(const Property& prop,
						   const std::string& strategy = "flat",
						   const PostProcessing& postProc = PostProcessing()) override;

	void assess_importance(const Property& prop,
						   const std::string& formulaExprStr,
						   const std::vector<std::string>& varnames) override;
};

} // namespace fig

#endif // IMPORTANCEFUNCTIONCONCRETECOUPLED_H

