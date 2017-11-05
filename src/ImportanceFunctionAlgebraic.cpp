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
#include <Property.h>

// ADL
using std::begin;
using std::end;


namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

using fig::ImportanceValue;
using Formula = fig::ImportanceFunction::Formula;
typedef fig::State< fig::STATE_INTERNAL_TYPE > State;
typedef std::shared_ptr< fig::Variable< fig::STATE_INTERNAL_TYPE > > VariablePtr;


/**
 * @brief Increment in 's' the value of the "next variable",
 *        among those declared in 'vars'
 *
 *        Following the order of the variables defined in vars, find the
 *        first one in 's' whose value isn't maximal and increment it.
 *        All bypassed variables (whose value was maximal) are reset
 *        to their minimal values. This represents a single increment in
 *        the concrete value of 's' but following 'vars' variables ordering.
 *
 * @return Whether a Variable could be incremented
 */
bool advance(const std::vector< std::string >& vars, State& s)
{
	unsigned vpos(0u);
	VariablePtr var_p(nullptr);// = s[vars[vpos]];
	// Find next variable whose value can be incremented (following 'vars' order)
	do {
		var_p = s[vars[vpos]];
	} while (var_p->val() == var_p->max() && ++vpos < vars.size());
	if (vpos >= vars.size())
		return false;
	// Increment its value
	var_p->inc();
	// Reset previous variables (according to 'vars' order) to their minimal values
	for (unsigned i = 0ul ; i < vpos ; i++) {
		var_p = s[vars[i]];
		(*var_p) = var_p->min();
	}
	return true;
}


/**
 * @brief Find extreme values of given \ref fig::ImportanceFunction::Formula
 *        "algebraic formula"
 *
 *        All values combinations of the variables used in the formula
 *        are tested, for the variable ranges specified in State 's'.
 *        From all the resulting evaluations of the Formula 'f' the minimal
 *        value is returned as the first component of the pair and the maximal
 *        value as the second component.
 *
 * @param f  User defined algebraic formula (aka: ad hoc function)
 * @param s  Global state of the system model
 *
 * @return (min,max) evaluations of 'f' for all possible values combinations
 *         of the variables ocurring in it
 *
 * @note <b>Complexity:</b> <i>O((state|<sub>{f.get_free_vars()}</sub>).concrete_size()
 *                               * state.size())</i>
 *
 * @throw FigException if some variable used in 'f' isn't found in State 's'
 */
std::pair< ImportanceValue, ImportanceValue >
find_extreme_values(const Formula& f, State s)
{
	ImportanceValue min(std::numeric_limits<ImportanceValue>::max());
	ImportanceValue max(std::numeric_limits<ImportanceValue>::min());
	const std::vector< std::string > vars(f.get_free_vars());

	// Initialize (in 's') all relevant variables to their minimal values
	for (const auto& var: vars) {
		VariablePtr var_p = s[var];
		if (nullptr == var_p)
			throw_FigException("variable \"" + var + "\" not found in state");
		(*var_p) = var_p->min();
	}

	// Test all values combinations for the relevant variables
	do {
		ImportanceValue imp = f(s.to_state_instance());
		min = std::min(min, imp);
		max = std::max(max, imp);
	} while (advance(vars, s));

	return std::make_pair(min, max);
}

} // namespace  // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ImportanceFunctionAlgebraic::ImportanceFunctionAlgebraic() :
	ImportanceFunction("algebraic")
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


template< template< typename... > class Container, typename... OtherArgs >
void
ImportanceFunctionAlgebraic::set_formula(
	const std::string& strategy,
	const std::string& formulaExprStr,
	const Container<std::string, OtherArgs...>& varnames,
	const State<STATE_INTERNAL_TYPE>& gState,
	const Property& property,
	const ImportanceValue& minVal,
	const ImportanceValue& maxVal)
{
    if ("auto" == strategy)
		throw_FigException("importance strategy \"auto\" can only be used "
						   "with concrete importance functions");
	else if (std::find(begin(ImportanceFunction::strategies()),
					   end(ImportanceFunction::strategies()),
					   strategy) == end(ImportanceFunction::strategies()))
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
	initialValue_ = importance_of(gState.to_state_instance());
	if ("flat" == strategy) {
		// Little to find out
		minValue_ = initialValue_;
		maxValue_ = initialValue_;
		minRareValue_ = initialValue_;

	} else if (minVal < maxVal) {
		// Trust blindly in the user-defined extreme values
		minValue_ = minVal;
		maxValue_ = maxVal;
		minRareValue_ = std::max(initialValue_, minValue_);  // play it safe

	} else if (gState.concrete_size() < (1ul<<20ul)) {
		// We can afford a full-state-space scan
		find_extreme_values(gState, property);

	} else {
		// Concrete state space is too big, resort to faster ways
		std::tie(minValue_, maxValue_) = ::find_extreme_values(userFun_, gState);
		minRareValue_ = std::max(initialValue_, minValue_);  // play it safe
		/// @todo TODO The general solution is to use ILP on userFun_
		///            That'd also compute the real minRareValue_ (and fast!)
		///            Use <a href="http://dlib.net/">dlib</a> maybe?
	}

	assert(minValue_ <= initialValue_);
//	assert(initialValue_ <= minRareValue_);  // may be false for functions
											 // defined ad hoc by the user
	assert(initialValue_ <= maxValue_);
	assert(minRareValue_ <= maxValue_);
}

// ImportanceFunctionAlgebraic::set_formula() can only be invoked
// with the following containers
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::set<std::string>&,
	const State<STATE_INTERNAL_TYPE>&, const Property&,
	const ImportanceValue&, const ImportanceValue&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::list<std::string>&,
	const State<STATE_INTERNAL_TYPE>&, const Property&,
	const ImportanceValue&, const ImportanceValue&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::deque<std::string>&,
	const State<STATE_INTERNAL_TYPE>&, const Property&,
	const ImportanceValue&, const ImportanceValue&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::vector<std::string>&,
	const State<STATE_INTERNAL_TYPE>&, const Property&,
	const ImportanceValue&, const ImportanceValue&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::forward_list<std::string>&,
	const State<STATE_INTERNAL_TYPE>&, const Property&,
	const ImportanceValue&, const ImportanceValue&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::unordered_set<std::string>&,
	const State<STATE_INTERNAL_TYPE>&, const Property&,
	const ImportanceValue&, const ImportanceValue&);


void
ImportanceFunctionAlgebraic::print_out(std::ostream& out,
									   State<STATE_INTERNAL_TYPE> s) const
{
	if (!has_importance_info()) {
		out << "\nImportance function \"" << name() << "\" doesn't yet have "
			   "any importance information to print." << std::endl;
		return;
	} else if (s.size() < 1ul) {
		out << "\n[WARNING] Importance function \"" << name() << "\" "
			   "print_out() method needs a valid virtual state as input; "
			   "skipping importance information printing." << std::endl;
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
	for (uint128_t i = uint128::uint128_0 ; i < s.concrete_size() ; i++) {
		const StateInstance symbState = s.decode(i).to_state_instance();
        out << " (";
		print_state(symbState);
        out << " " << importance_of(symbState) << ")";
		out.flush();
	}
    out << std::endl;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
