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


namespace fig
{

// Available function names in ImportanceFunction::names
ImportanceFunctionConcreteCoupled::ImportanceFunctionConcreteCoupled(
    const ModuleNetwork &model) :
        ImportanceFunctionConcrete("concrete_coupled"),
        globalStateCopy_(model.global_state()),
        globalInitialValuation_(model.global_state().to_state_instance()),
        globalTransitions_(model.transitions_),
        importanceInfoIndex_(0u)
{ /* Not much to do around here */ }


ImportanceFunctionConcreteCoupled::~ImportanceFunctionConcreteCoupled()
{
	clear();
}


void
ImportanceFunctionConcreteCoupled::assess_importance(
	const Property& prop,
    const std::string& strategy)
{
    if (hasImportanceInfo_)
        clear();
    globalStateCopy_.copy_from_state_instance(globalInitialValuation_);
    ImportanceFunctionConcrete::assess_importance(globalStateCopy_,
                                                  globalTransitions_,
                                                  prop,
                                                  strategy,
                                                  importanceInfoIndex_);
	hasImportanceInfo_ = true;
	strategy_ = strategy;
}


template< template< typename... > class Container, typename... OtherArgs >
virtual void
ImportanceFunctionConcreteCoupled::assess_importance(
    const Property& prop,
    const std::string& formulaExprStr,
    const Container<std::string,OtherArgs...>& varnames)
{
    /// @todo TODO: implement concrete ifun with ad hoc importance assessment
}


void
ImportanceFunctionConcreteCoupled::build_thresholds(
	ThresholdsBuilder& tb,
	const unsigned& splitsPerThreshold)
{
	if (!has_importance_info())
		throw_FigException(std::string("importance function \"").append(name())
						   .append("\" doesn't yet have importance information"));
    ImportanceVec& impVec = modulesConcreteImportance[importanceInfoIndex_];

    // Build translator from importance to threshold-level
    std::vector< ImportanceValue > imp2thr =
            tb.build_thresholds(splitsPerThreshold, *this);
    maxImportance_ = imp2thr.back();
    thresholdsTechnique_ = tb.name;

    // Replace importance info with the new thresholds info
    #pragma omp parallel for default(shared)
    for (size_t s = 0u ; s < impVec.size() ; s++) {
        fig::Event mask = fig::MASK(impVec[s]);
        impVec[s] = mask | imp2thr[impVec[s]];
    }

	// Find lowest threshold level where we can find a rare state
	minRareImportance_ = std::numeric_limits<ImportanceValue>::max();
	for (size_t i = 0u ; i < impVec.size() ; i++)
		if (IS_RARE_EVENT(impVec[i]) && UNMASK(impVec[i]) < minRareImportance_)
			minRareImportance_ = UNMASK(impVec[i]);

    readyForSims_ = true;
}


void
ImportanceFunctionConcreteCoupled::print_out(std::ostream& out) const
{
	if (!has_importance_info()) {
		out << "\nImportance function \"" << name() << "\" doesn't yet have "
			   "any importance information to print." << std::endl;
		return;
	}
	out << "\nPrinting importance function \"" << name() << "\" values.\n";
	if (ready())
		out << "Legend: ( concrete_state_value[*~^] , threshold_level )\n";
	else
		out << "Legend: ( concrete_state_value[*~^] , importance_value )\n";
	out << "where\n"
		<< "      *  denotes a state is RARE,\n"
		<< "      ~  denotes a state is STOP,\n"
		<< "      ^  denotes a state is REFERENCE.\n";
	out << "Values:" << std::endl;
	const ImportanceVec& impVec = modulesConcreteImportance[importanceInfoIndex_];
	for (size_t i = 0ul ; i < impVec.size() ; i++) {
		out << "(" << i;
		out << (IS_RARE_EVENT     (impVec[i]) ? "*" : "");
		out << (IS_STOP_EVENT     (impVec[i]) ? "~" : "");
		out << (IS_REFERENCE_EVENT(impVec[i]) ? "^" : "");
		out << "," << UNMASK(impVec[i]) << ") ";
		out.flush();
	}
	out << "\b" << std::endl;
}


void
ImportanceFunctionConcreteCoupled::clear() noexcept
{
	ImportanceFunctionConcrete::clear();
	hasImportanceInfo_ = false;
	readyForSims_ = false;
	strategy_ = "";
	thresholdsTechnique_ = "";
	maxImportance_ = static_cast<ImportanceValue>(0u);
	minRareImportance_ = static_cast<ImportanceValue>(0u);
}

} // namespace fig
