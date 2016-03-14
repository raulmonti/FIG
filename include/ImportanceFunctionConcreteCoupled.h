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

public:  // Accessors

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
			return modulesConcreteImportance[importanceInfoIndex_]
											[globalStateCopy.encode()];
		}

	/// @copydoc ImportanceFunction::importance_of()
	/// @note Attempted inline in a desperate need for speed
	/// @note <b>Complexity:</b> <i>O(size(state)<sup>2</sup>)</i>
	inline ImportanceValue importance_of(const StateInstance& state) const override
		{
			return UNMASK(info_of(state));
		}

	/// @copydoc ImportanceFunction::level_of(const StateInstance&)
	/// @note Attempted inline in a desperate need for speed
	/// @note <b>Complexity:</b> same as ImportanceFunctionConcreteCoupled::info_of()
	inline ImportanceValue level_of(const StateInstance &state) const override
		{
#       ifndef NDEBUG
			if (!ready())
				throw_FigException("importance function \"" + name() + "\" "
								   + "isn't ready for simulations.");
#		endif
			// Internal vector currently holds threshold levels
			return UNMASK(info_of(state));
		}

	/// @copydoc ImportanceFunction::level_of(const ImportanceValue&)
	/// @note Attempted inline in a desperate need for speed
	/// @note <b>Complexity:</b> <i>O(1)</i>
	inline ImportanceValue level_of(const ImportanceValue& val) const override
		{
#       ifndef NDEBUG
			if (!ready())
				throw_FigException("importance function \"" + name() + "\" "
								   + "isn't ready for simulations.");
#		endif
			// Internal vector currently holds threshold levels
			assert(val >= min_value());
			assert(val <= max_value());
			return val;
		}

	void print_out(std::ostream& out, State<STATE_INTERNAL_TYPE>) const override;

public:  // Utils

	void assess_importance(const Property& prop,
						   const std::string& strategy = "flat") override;

	void assess_importance(const Property& prop,
						   const std::string& formulaExprStr,
						   const std::vector<std::string>& varnames) override;

	void build_thresholds(ThresholdsBuilder& tb, const unsigned& spt) override;

	void build_thresholds_adaptively(ThresholdsBuilderAdaptive& atb,
									 const unsigned& spt,
									 const float& p,
									 const unsigned& n) override;
private:  // Class utils

	/// Post-processing once the thresholds have been chosen
	/// @param tbName  Name of the ThresholdsBuilder used
	/// @param imp2thr Translator from ImportanceValue to threshold level
	/// @see build_thresholds()
	/// @see build_thresholds_adaptively()
	void post_process_thresholds(const std::string& tbName,
								 std::vector< ImportanceValue >& imp2thr);
};

} // namespace fig

#endif // IMPORTANCEFUNCTIONCONCRETECOUPLED_H

