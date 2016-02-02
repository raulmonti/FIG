//==============================================================================
//
//  ImportanceFunctionConcreteCoupled.cpp
//
//  Copyleft 2016-
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


#include <ImportanceFunctionConcreteCoupled.h>
#include <ModuleNetwork.h>
#include <ThresholdsBuilder.h>


namespace
{

using fig::ImportanceValue;
using ImportanceVec = fig::ImportanceFunctionConcrete::ImportanceVec;

ImportanceValue find_min_rare_importance(const ImportanceVec& impVec)
{
	ImportanceValue minImp(std::numeric_limits<ImportanceValue>::max);
	for (size_t i = 0u ; i < impVec.size() ; i++)
		if (fig::IS_RARE_EVENT(impVec[i]) && fig::UNMASK(impVec[i]) < minImp)
			minImp = fig::UNMASK(impVec[i]);
	return minImp;
}

} // namespace


namespace fig
{

// Available function names in ImportanceFunction::names
ImportanceFunctionConcreteCoupled::ImportanceFunctionConcreteCoupled() :
	ImportanceFunctionConcrete("concrete_coupled"),
	globalStateCopy_(),
	importanceInfoIndex_(0u)
{ /* Not much to do around here */ }


ImportanceFunctionConcreteCoupled::~ImportanceFunctionConcreteCoupled()
{
	clear();
}


void
ImportanceFunctionConcreteCoupled::assess_importance(
	const ModuleNetwork& net,
	const Property& prop,
	const std::string& strategy,
	bool force)
{
	if (force || strategy_ != strategy) {
		globalStateCopy_ = net.global_state();
		ImportanceFunctionConcrete::assess_importance(net.gState,
													  net.transitions_,
													  prop,
													  strategy,
													  importanceInfoIndex_);
	}
	hasImportanceInfo_ = true;
	strategy_ = strategy;
}


void
ImportanceFunctionConcreteCoupled::build_thresholds(
	const ThresholdsBuilder& tb,
	const unsigned& splitsPerThreshold)
{
	if (!has_importance_info())
		throw_FigException(std::string("importance function \"").append(name())
						   .append("\" doesn't yet have importance information"));

	ImportanceVec& impVec = modulesConcreteImportance[importanceInfoIndex_];
	unsigned numThresholds = tb.build_thresholds_concrete(splitsPerThreshold,
														  maxImportance_,
														  impVec);
	thresholdsTechnique_ = tb.name;
	maxImportance_ = numThresholds;
	// Find lowest threshold level where we can find a rare state
	minRareImportance_ = std::numeric_limits<ImportanceValue>::max;
	for (size_t i = 0u ; i < impVec.size() ; i++)
		if (IS_RARE_EVENT(impVec[i]) && UNMASK(impVec[i]) < minRareImportance_)
			minRareImportance_ = UNMASK(impVec[i]);
	readyForSims_ = true;
}


ImportanceValue
ImportanceFunctionConcreteCoupled::importance_of(const StateInstance& state) const
{
#ifndef NDEBUG
	assert(has_importance_info());
	globalStateCopy_.copy_from_state_instance(state, true);
#else
	globalStateCopy_.copy_from_state_instance(state, false);
#endif
	return UNMASK(modulesConcreteImportance[importanceInfoIndex_]
										   [globalStateCopy_.encode()]);
}


void
ImportanceFunctionConcreteCoupled::clear() noexcept
{
	ImportanceFunctionConcrete::clear();
	globalStateCopy_ = State< STATE_INTERNAL_TYPE >();
	hasImportanceInfo_ = false;
	readyForSims_ = false;
	strategy_ = "";
	thresholdsTechnique_ = "";
	maxImportance_ = static_cast<ImportanceValue>(0u);
	minRareImportance_ = static_cast<ImportanceValue>(0u);
}

} // namespace fig
