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
 * @brief Importance function for the concrete importance assessment
 *        of a ModuleNetwork.
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
	/// Copy of the global state of the model (i.e. the network of modules)
	mutable State< STATE_INTERNAL_TYPE > globalStateCopy_;

	/// For interaction with base class ImportanceFunctionConcrete
	const unsigned importanceInfoIndex_;

public:  // Ctor/Dtor

	ImportanceFunctionConcreteCoupled();

	virtual ~ImportanceFunctionConcreteCoupled();

public:  // Accessors

	/// @copydoc ImportanceFunctionConcrete::info_of()
	/// @note Attempted inline in a desperate need for speed
	inline virtual ImportanceValue info_of(const StateInstance& state) const
		{
#       ifndef NDEBUG
			if (!has_importance_info())
				throw_FigException(std::string("importance function \"")
								   .append(name()).append("\" doesn't ")
								   .append("hold importance information."));
			globalStateCopy_.copy_from_state_instance(state, true);
#       else
			globalStateCopy_.copy_from_state_instance(state, false);
#       endif
			return modulesConcreteImportance[importanceInfoIndex_]
											[globalStateCopy_.encode()];
		}

	/// @copydoc ImportanceFunction::importance_of()
	/// @note Attempted inline in a desperate need for speed
	inline virtual ImportanceValue importance_of(const StateInstance& state) const
		{
			return UNMASK(info_of(state));
		}

	/// @copydoc ImportanceFunction::level_of()
	/// @note Attempted inline in a desperate need for speed
	inline virtual ImportanceValue level_of(const StateInstance &state) const
		{
#       ifndef NDEBUG
			if (!ready())
				throw_FigException(std::string("importance function \"")
								   .append(name()).append("\" isn't ")
								   .append("ready for simulations."));
#		endif
			// Internal vector currently holds threshold levels
			return UNMASK(info_of(state));
		}

	virtual void print_out(std::ostream& out) const;

public:  // Utils

	virtual void assess_importance(const ModuleNetwork& net,
								   const Property& prop,
								   const std::string& strategy = "",
								   bool force = false);

	virtual void build_thresholds(ThresholdsBuilder& tb,
								  const unsigned& splitsPerThreshold);

	virtual void clear() noexcept;
};

} // namespace fig

#endif // IMPORTANCEFUNCTIONCONCRETECOUPLED_H

