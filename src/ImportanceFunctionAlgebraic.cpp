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
#include <tuple>  // std::tie()
#include <vector>
#include <forward_list>
#include <unordered_set>
#include <algorithm>  // std::find()
#include <iterator>   // std::advance(), std::begin(), std::end()
#include <numeric>    // std::numeric_limits<>
// FIG
#include <ImportanceFunctionAlgebraic.h>
#include <ThresholdsBuilder.h>
#include <Property.h>

// ADL
using std::begin;
using std::end;


namespace
{

using fig::ImportanceValue;
using fig::ImportanceFunction::Formula;
using State = fig::State< fig::STATE_INTERNAL_TYPE >;
using Variable = fig::Variable< fig::STATE_INTERNAL_TYPE >;


/**
 * @brief Increment in 's' the value of the "next variable",
 *        among those declared in 'vars'
 *
 *        Following the order of the variables defined in vars, find the
 *        first one in 's' whose value isn't maximal and increment it.
 *        All variables bypassed (whose value was maximal) are reset
 *        to their minimal values. This represents an increase in the
 *        concrete value of 's' but following 'vars' variables ordering.
 *
 * @throw FigException if no variable from 'vars' can be incremented in 's'
 */
void advance(const std::vector< std::string >& vars, State& s)
{
	unsigned vpos(0u);
	std::shared_ptr< Variable > var_p = s[vars[vpos]];
	// Find next variable whose value can be incremented (following 'vars' order)
	while (var_p->val() == var_p->max() && vpos < vars.size())
		var_p = s[vars[++vpos]];
	if (vpos >= vars.size())
		throw_FigException("couldn't increment any variable in given state");
	// Increment its value
	var_p->inc();
	// Reset previous variables (according to 'vars' order) to their minimal values
	for (unsigned i = 0ul ; i < vpos ; i++) {
		var_p = s[vars[i]];
		var_p->val() = var_p->min();
	}
}


/**
 * @brief Find extreme values of given \ref fig::ImportanceFunction::Formula
 *        "algebraic formula"
 *
 *        All values combinations of the variables used in the formula
 *        are tested, for the possibilities specified in State 's'.
 *        From all the resulting evaluations of the Formula 'f'
 *        the minimal value is returned as the first component of the tuple
 *        and the maximal value is returned as the second component.
 *
 * @param f  User defined algebraic formula (aka: ad hoc function)
 * @param s  Global state of the system model
 *
 * @return (min,max) evaluations of 'f' for all possible values combinations
 *         of the variables ocurring in it
 *
 * @note <b>Complexity:</b> <i>O((state|<sub>{f.free_vars()}</sub>).concrete_size()
 *                               * state.size())</i>
 *
 * @throw FigException if some variable used in 'f' isn't found in State 's'
 */
std::pair< ImportanceValue, ImportanceValue >
find_extreme_values(const Formula& f, State s)
{
	ImportanceValue min(std::numeric_limits<ImportanceValue>::max());
	ImportanceValue max(std::numeric_limits<ImportanceValue>::min());
	const std::vector< std::string > vars(f.free_vars());
	size_t checked(0ul), last(1ul);

	// Initialize (in 's') all relevant variables to their minimal values
	for (const auto& var: vars) {
		std::shared_ptr< Variable > var_p = s[var];
		if (nullptr == var_p)
			throw_FigException("variable \"" + var + "\" not found in state");
		(*var_p) = var_p->min();
		last *= var_p->range();
	}

	// Test all values combinations for the relevant variables
	do {
		ImportanceValue imp = f(s.to_state_instance());
		min = std::min(min, imp);
		max = std::max(max, imp);
		advance(vars, s);
	} while (++checked < last);

	return std::make_pair(min, max);
}

} // namespace



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
		throw_FigException("importance function \"" + name() + "\" "
						   "doesn't hold importance information.");
#endif
	return userFun_(state);
}


ImportanceValue
ImportanceFunctionAlgebraic::level_of(const StateInstance& state) const
{
#ifndef NDEBUG
	if (!ready())
		throw_FigException("importance function \"" + name() + "\" "
						   "isn't ready for simulations.");
#endif
	return importance2threshold_[userFun_(state)];
}


ImportanceValue
ImportanceFunctionAlgebraic::level_of(const ImportanceValue& val) const
{
#ifndef NDEBUG
	if (!ready())
		throw_FigException("importance function \"" + name() + "\" "
						   "isn't ready for simulations.");
#endif
	assert(val < importance2threshold_.size());
	return importance2threshold_[val];
}


template< template< typename... > class Container, typename... OtherArgs >
void
ImportanceFunctionAlgebraic::set_formula(
	const std::string& strategy,
	const std::string& formulaExprStr,
	const Container<std::string, OtherArgs...>& varnames,
	const State<STATE_INTERNAL_TYPE>& gState,
	const Property&)
{
    if ("auto" == strategy)
		throw_FigException("importance strategy \"auto\" can only be used "
						   "with concrete importance functions");
	else if (std::find(begin(ImportanceFunction::strategies),
					   end(ImportanceFunction::strategies),
					   strategy) == end(ImportanceFunction::strategies))
		throw_FigException("unrecognized importance assessment strategy \""
						   + strategy + "\". See available options with "
						   "ModelSuite::available_importance_strategies()");
    try {
		userFun_.set(formulaExprStr, varnames, gState);
	} catch (std::out_of_range& e) {
		throw_FigException("something went wrong while setting the function \""
						   + formulaExprStr + "\" for ad hoc importance "
						   "assessment: " + e.what());
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
		std::vector< std::string > varnamesVec;
		varnamesVec.insert(begin(varnamesVec), begin(varnames), end(varnames));
		std::tie(minValue_, maxValue_) = ::find_extreme_values(userFun_, gState);
		// Assume minRareValue_ == minValue_ to avoid exploring whole state space
		minRareValue_ = minValue_;

		/// @todo TODO The general solution is to use ILP on userFun_
		///            That'd also compute the real minRareValue_ (and fast!)
		///            Use <a href="http://dlib.net/">dlib</a> maybe?
	}
	assert(minValue_ <= minRareValue_);
	assert(minRareValue_ <= maxValue_);
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
