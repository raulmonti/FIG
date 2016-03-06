//==============================================================================
//
//  ImportanceFunctionAlgebraic.cpp
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


// C++
#include <set>
#include <list>
#include <deque>
#include <vector>
#include <forward_list>
#include <unordered_set>
#include <algorithm>  // std::find()
#include <iterator>   // std::begin(), std::end()
// FIG
#include <ImportanceFunctionAlgebraic.h>
#include <ThresholdsBuilder.h>
#include <Property.h>

// ADL
using std::begin;
using std::end;

namespace fig
{

ImportanceFunctionAlgebraic::ImportanceFunctionAlgebraic() :
	ImportanceFunction("algebraic"),
	importance2threshold_()
{ /* Not much to do around here */ }


ImportanceFunctionAlgebraic::~ImportanceFunctionAlgebraic()
{
	clear();
}


ImportanceValue
ImportanceFunctionAlgebraic::importance_of(const StateInstance& state) const
{
#ifndef NDEBUG
	if (!has_importance_info())
		throw_FigException(std::string("importance function \"")
						   .append(name()).append("\" doesn't ")
						   .append("hold importance information."));
#endif
	return userFun_(state);
}


ImportanceValue
ImportanceFunctionAlgebraic::level_of(const StateInstance& state) const
{
#ifndef NDEBUG
	if (!ready())
		throw_FigException(std::string("importance function \"")
						   .append(name()).append("\" isn't ")
						   .append("ready for simulations."));
#endif
	return importance2threshold_[userFun_(state)];
}


ImportanceValue
ImportanceFunctionAlgebraic::level_of(const ImportanceValue& val) const
{
#ifndef NDEBUG
	if (!ready())
		throw_FigException(std::string("importance function \"")
						   .append(name()).append("\" isn't ")
						   .append("ready for simulations."));
#endif
	assert(val >= min_importance());
	assert(val <= max_importance());
	return importance2threshold_[val];
}


template< template< typename... > class Container, typename... OtherArgs >
void
ImportanceFunctionAlgebraic::set_formula(
	const std::string& strategy,
	const std::string& formulaExprStr,
	const Container<std::string, OtherArgs...>& varnames,
	const State<STATE_INTERNAL_TYPE>& gState,
	const Property& property)
{
    if ("auto" == strategy)
		throw_FigException("importance strategy \"auto\" can only be used "
						   "with concrete importance functions");
	else if (std::find(begin(ImportanceFunction::strategies),
					   end(ImportanceFunction::strategies),
					   strategy) == end(ImportanceFunction::strategies))
		throw_FigException(std::string("unrecognized importance assessment ")
						   .append("strategy \"").append(strategy).append("\"")
						   .append(". See available options with ModelSuite::")
						   .append("available_importance_strategies()"));
    try {
		userFun_.set(formulaExprStr, varnames, gState);
	} catch (std::out_of_range& e) {
		throw_FigException(std::string("something went wrong while setting ")
						   .append(" the function \"").append(formulaExprStr)
						   .append("\" for ad hoc importance assessment: ")
						   .append(e.what()));
	}
	hasImportanceInfo_ = true;
	strategy_ = strategy;

	// Find extreme importance values for this ad hoc function
	if ("flat" == strategy) {
		const ImportanceValue importance =
				importance_of(gState.to_state_instance());
		minValue_ = importance;
		maxValue_ = importance;
		minRareValue_ = importance;
	} else {
		/// @todo FIXME compute extreme values using LP from z3
		///             for current formula and with variable's restrictions.
		///             Assume minRareValue_ == minValue_ to avoid exploration
		///             of the whole state space
		throw_FigException("compute extreme values using LP from z3");
	}
	assert(minImportance_ <= minRareImportance_);
	assert(minRareImportance_ <= maxImportance_);
}

// ImportanceFunctionAlgebraic::set_formula() can only be invoked
// with the following containers
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::set<std::string>&,
    const State<STATE_INTERNAL_TYPE>&, const Property&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::list<std::string>&,
    const State<STATE_INTERNAL_TYPE>&, const Property&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::deque<std::string>&,
    const State<STATE_INTERNAL_TYPE>&, const Property&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::vector<std::string>&,
    const State<STATE_INTERNAL_TYPE>&, const Property&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::forward_list<std::string>&,
    const State<STATE_INTERNAL_TYPE>&, const Property&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::unordered_set<std::string>&,
    const State<STATE_INTERNAL_TYPE>&, const Property&);


void
ImportanceFunctionAlgebraic::build_thresholds(
    ThresholdsBuilder& tb,
    const unsigned& splitsPerThreshold)
{
	if (!has_importance_info())
		throw_FigException(std::string("importance function \"").append(name())
						   .append("\" doesn't yet have importance information"));

	// Build translator from ImportanceValue to threshold level
	std::vector< ImportanceValue >().swap(importance2threshold_);
	importance2threshold_ = tb.build_thresholds(splitsPerThreshold, *this);
	assert(!importance2threshold_.empty());
	assert(importance2threshold_[0] == static_cast<ImportanceValue>(0u));
	assert(importance2threshold_[0] <= importance2threshold_.back());

	// Update extreme values info
	// (threshold levels are a non-decreasing function of the importance)
	minValue_ = importance2threshold_[minValue_];
	maxValue_ = importance2threshold_[maxValue_];
	minRareValue_ = importance2threshold_[minRareValue_];
	assert(minValue_ <= minRareValue_);
	assert(minRareValue_ <= maxValue_);

	numThresholds_ = importance2threshold_.back();
	thresholdsTechnique_ = tb.name;
	readyForSims_ = true;
}


void
ImportanceFunctionAlgebraic::print_out(std::ostream& out,
									   State<STATE_INTERNAL_TYPE> s) const
{
	if (!has_importance_info()) {
		out << "\nImportance function \"" << name() << "\" doesn't yet have "
			   "any importance information to print." << std::endl;
		return;
	}
	auto print_state = [&out](const StateInstance& symbState) {
		out << "/";
		for (const auto& val: symbState)
			out << val << ",";
		out << "\b\\";
	};
    out << "\nPrinting importance function \"" << name() << "\" values.";
    out << "\nImportance assessment strategy: " << strategy();
    out << "\nLegend: (/symbolic,state,valuation\\ importance_value)";
    out << "\nValues for coupled model:";
	for (size_t i = 0ul ; i < s.concrete_size() ; i++) {
		const StateInstance symbState = s.decode(i).to_state_instance();
        out << " (";
		print_state(symbState);
        out << " " << importance_of(symbState) << ")";
		out.flush();
	}
    out << std::endl;
}


void
ImportanceFunctionAlgebraic::clear() noexcept
{
	std::vector< ImportanceValue >().swap(importance2threshold_);
	ImportanceFunction::clear();
}

} // namespace fig
