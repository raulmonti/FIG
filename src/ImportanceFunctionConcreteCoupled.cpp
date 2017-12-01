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


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
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
ImportanceFunctionConcreteCoupled::assess_importance(const Property& prop,
													 const std::string& strategy,
													 const PostProcessing& postProc)
{
    if (hasImportanceInfo_)
		ImportanceFunctionConcrete::clear();
	const bool importanceAssessed =
		ImportanceFunctionConcrete::assess_importance(model_,
													  prop,
													  strategy,
													  importanceInfoIndex_);
	assert(importanceAssessed);
	hasImportanceInfo_ = importanceAssessed;
	strategy_ = strategy;

	// Apply post processing (shift, exponentiation, etc.)
	auto extrVals = std::vector<ExtremeValues>(
	                    {std::tie(minValue_, maxValue_, minRareValue_)});
	if ("flat" != strategy)
		post_process(postProc, extrVals);
	minValue_ = std::get<0>(extrVals.front());
	maxValue_ = std::get<1>(extrVals.front());
	minRareValue_ = std::get<2>(extrVals.front());
	initialValue_ = minValue_;  // invariant for auto concrete coupled ifun

	assert(minValue_ <= initialValue_);
	assert(initialValue_ <= minRareValue_);
	assert(minRareValue_ <= maxValue_);
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
ImportanceFunctionConcreteCoupled::print_out(std::ostream& out,
                                             State<STATE_INTERNAL_TYPE>) const
{
	static constexpr auto MAX_PRINT_LEN((1ul)<<(10ul));
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
	if (impVec.size() > MAX_PRINT_LEN)
		out << " (printing only the first " << MAX_PRINT_LEN
		    << " out of a total of " << impVec.size() << " values)";
	for (size_t i = 0ul ; i < std::min(impVec.size(),MAX_PRINT_LEN) ; i++) {
        out << " (" << i;
		out << (IS_RARE_EVENT     (impVec[i]) ? "*" : "");
		out << (IS_STOP_EVENT     (impVec[i]) ? "~" : "");
		out << (IS_REFERENCE_EVENT(impVec[i]) ? "^" : "");
        out << "," << UNMASK(impVec[i]) << ")";
		out.flush();
	}
    out << std::endl;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
