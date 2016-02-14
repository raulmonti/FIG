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
// FIG
#include <ImportanceFunctionAlgebraic.h>


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
	return adhocFormula_(state);
}


template< template< typename... > class Container, typename... OtherArgs >
void
ImportanceFunctionAlgebraic::set_formula(
	const std::string& strategy,
	const std::string& formulaExprStr,
	const Container<std::string, OtherArgs...>& varnames,
	const State<STATE_INTERNAL_TYPE>& gState)
{
	try {
		adhocFormula_.reset(formulaExprStr, varnames, gState);
	} catch (std::out_of_range& e) {
		throw_FigException(std::string("something went wrong while setting ")
						   .append(" the formula \"").append(formulaExprStr)
						   .append("\" for ad hoc importance assessment: ")
						   .append(e.what()));
	}
	hasImportanceInfo_ = true;
	strategy_ = strategy;
}

// ImportanceFunctionAlgebraic::set_formula() can only be invoked
// with the following containers
template<> void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::set<std::string>&,
    const State<STATE_INTERNAL_TYPE>&);
template<> void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::list<std::string>&,
    const State<STATE_INTERNAL_TYPE>&);
template<> void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::deque<std::string>&,
    const State<STATE_INTERNAL_TYPE>&);
template<> void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::vector<std::string>&,
    const State<STATE_INTERNAL_TYPE>&);
template<> void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::forward_list<std::string>&,
    const State<STATE_INTERNAL_TYPE>&);
template<> void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::unordered_set<std::string>&,
    const State<STATE_INTERNAL_TYPE>&);


} // namespace fig
