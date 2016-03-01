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
		ImportanceFunctionConcrete("concrete_coupled", model.global_state()),
		model_(model),
        importanceInfoIndex_(0u)
{ /* Not much to do around here */ }


ImportanceFunctionConcreteCoupled::~ImportanceFunctionConcreteCoupled()
{
	ImportanceFunctionConcrete::clear();
}


void
ImportanceFunctionConcreteCoupled::assess_importance(
	const Property& prop,
    const std::string& strategy)
{
    if (hasImportanceInfo_)
		ImportanceFunctionConcrete::clear();
	ImportanceFunctionConcrete::assess_importance(model_,
                                                  prop,
                                                  strategy,
                                                  importanceInfoIndex_);
	assert(minImportance_ <= minRareImportance_);
	assert(minRareImportance_ <= maxImportance_);
	hasImportanceInfo_ = true;
	strategy_ = strategy;
}


void
ImportanceFunctionConcreteCoupled::assess_importance(
	const Property&,
	const std::string&,
	const std::vector<std::string>&)
{
	throw_FigException("TODO: ad hoc assessment and coupled concrete storage");
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

    // Build translator from importance to threshold-level
    std::vector< ImportanceValue > imp2thr =
            tb.build_thresholds(splitsPerThreshold, *this);
    assert(!imp2thr.empty());
	assert(imp2thr[0] == static_cast<ImportanceValue>(0u));
	assert(imp2thr[0] <= imp2thr.back());

    // Replace importance info with the new thresholds info
	ImportanceVec& impVec = modulesConcreteImportance[importanceInfoIndex_];
	#pragma omp parallel for default(shared)
    for (size_t i = 0ul ; i < impVec.size() ; i++) {
        ImportanceValue imp = impVec[i];
        impVec[i] = MASK(imp) | imp2thr[UNMASK(imp)];
    }

	// Update limits since old importance values were discarded
	minImportance_ = imp2thr[0];
	maxImportance_ = imp2thr.back();
	minRareImportance_ = std::numeric_limits<ImportanceValue>::max();
    for (size_t i = 0ul ; i < impVec.size() ; i++)
		if (IS_RARE_EVENT(impVec[i]) && UNMASK(impVec[i]) < minRareImportance_)
			minRareImportance_ = UNMASK(impVec[i]);
	assert(minImportance_ <= minRareImportance_);
	assert(minRareImportance_ <= maxImportance_);

	numThresholds_ = imp2thr.back();
	thresholdsTechnique_ = tb.name;
    readyForSims_ = true;
}


void
ImportanceFunctionConcreteCoupled::print_out(std::ostream& out,
                                             State<STATE_INTERNAL_TYPE>) const
{
	if (!has_importance_info()) {
		out << "\nImportance function \"" << name() << "\" doesn't yet have "
			   "any importance information to print." << std::endl;
		return;
	}
    out << "\nPrinting importance function \"" << name() << "\" values.";
    out << "\nImportance assessment strategy: " << strategy();
    if (ready())
        out << "\nLegend: ( concrete_state[*~^] , threshold_level )";
    else
        out << "\nLegend: ( concrete_state[*~^] , importance_value )";
    out << "\nwhere"
        << "\n      *  denotes a state is RARE,"
        << "\n      ~  denotes a state is STOP,"
        << "\n      ^  denotes a state is REFERENCE.";
    out << "\nValues for coupled model:";
    const ImportanceVec& impVec = modulesConcreteImportance[importanceInfoIndex_];
	for (size_t i = 0ul ; i < impVec.size() ; i++) {
        out << " (" << i;
		out << (IS_RARE_EVENT     (impVec[i]) ? "*" : "");
		out << (IS_STOP_EVENT     (impVec[i]) ? "~" : "");
		out << (IS_REFERENCE_EVENT(impVec[i]) ? "^" : "");
        out << "," << UNMASK(impVec[i]) << ")";
		out.flush();
	}
    out << std::endl;
}

} // namespace fig
