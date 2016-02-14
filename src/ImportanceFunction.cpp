//==============================================================================
//
//  ImportanceFunction.cpp
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
#include <sstream>
#include <iterator>   // std::begin, std::end
#include <algorithm>  // std::find()
// FIG
#include <ImportanceFunction.h>
#include <FigException.h>

// ADL
using std::begin;
using std::end;
using std::find;


namespace fig
{

// Static variables initialization

const std::array< std::string, 2 > ImportanceFunction::names =
{{
	// See ImportanceFunctionConcreteCoupled class
	"concrete_coupled",

//	// See ImportanceFunctionConcreteSplit class
//	"concrete_split"

	 // See ImportanceFunctionAlgebraic class
	 "algebraic"
}};


const std::array< std::string, 4 > ImportanceFunction::strategies =
{{
	// Flat importance, i.e. null ImportanceValue for all states
	"",
	"flat",

	// Automatically built importance, with backwards BFS
	"auto",

	// User defined importance, by means of some function over the states
	"adhoc"
}};



// ImportanceFunction internal "Formula" class

ImportanceFunction::Formula::Formula() :
    MathExpression("", std::vector<std::string>() )
{ /* Not much to do around here */ }


template< template< typename... > class Container, typename... OtherArgs >
void
ImportanceFunction::Formula::set(
    const std::string& formula,
    const Container<std::string, OtherArgs...>& varnames,
    const State<STATE_INTERNAL_TYPE>& globalState)
{
	empty_ = "" == formula;
    exprStr_ = empty_ ? MathExpression::emptyExpressionString : formula;
    parse_our_expression();  // updates expr_
	varsMap_.clear();
	for (const auto& name: varnames) {
#ifndef NRANGECHK
        if (std::string::npos == exprStr_.find(name))
			throw std::out_of_range(std::string("invalid variable name: \"")
									.append(name).append("\""));
#endif
		varsMap_.emplace_back(       // copy elision
				std::make_pair(name, globalState.position_of_var(name)));
	}
	pinned_ = true;
    // Fake an evaluation to reveal parsing errors now
    STATE_INTERNAL_TYPE dummy(static_cast<STATE_INTERNAL_TYPE>(1.1));
    try {
        for (const auto& pair: varsMap_)
            expr_.DefineVar(pair.first, &dummy);
        dummy = expr_.Eval();
    } catch (mu::Parser::exception_type &e) {
        std::cerr << "Failed parsing expression" << std::endl;
        std::cerr << "    message:  " << e.GetMsg()   << std::endl;
        std::cerr << "    formula:  " << e.GetExpr()  << std::endl;
        std::cerr << "    token:    " << e.GetToken() << std::endl;
        std::cerr << "    position: " << e.GetPos()   << std::endl;
        std::cerr << "    errc:     " << e.GetCode()  << std::endl;
        throw_FigException("bad expression for ImportanceFunction::Formula, "
                           "did you remember to map all the variables?");
    }
}

// ImportanceFunction::Formula::set() can only be invoked with the following containers
template void ImportanceFunction::Formula::set(const std::string&,
                                               const std::set<std::string>&,
                                               const State<STATE_INTERNAL_TYPE>&);
template void ImportanceFunction::Formula::set(const std::string&,
                                               const std::list<std::string>&,
                                               const State<STATE_INTERNAL_TYPE>&);
template void ImportanceFunction::Formula::set(const std::string&,
                                               const std::deque<std::string>&,
                                               const State<STATE_INTERNAL_TYPE>&);
template void ImportanceFunction::Formula::set(const std::string&,
                                               const std::vector<std::string>&,
                                               const State<STATE_INTERNAL_TYPE>&);
template void ImportanceFunction::Formula::set(const std::string&,
                                               const std::forward_list<std::string>&,
                                               const State<STATE_INTERNAL_TYPE>&);
template void ImportanceFunction::Formula::set(const std::string&,
                                               const std::unordered_set<std::string>&,
                                               const State<STATE_INTERNAL_TYPE>&);


void
ImportanceFunction::Formula::reset() noexcept
{
    empty_ = true;
    exprStr_ = MathExpression::emptyExpressionString;
    try { parse_our_expression(); } catch (FigException&) {}
    varsMap_.clear();
    pinned_ = false;
}


ImportanceValue
ImportanceFunction::Formula::operator()(const StateInstance& state) const
{
    // Bind symbolic state variables to the current expression...
    for (const auto& pair: varsMap_)
        expr_.DefineVar(pair.first,  const_cast<STATE_INTERNAL_TYPE*>(
                        &state[pair.second]));
    // ...and evaluate
    return static_cast<ImportanceValue>(expr_.Eval());
}



// ImportanceFunction class member functions

ImportanceFunction::ImportanceFunction(const std::string& name) :
	name_(name),
	hasImportanceInfo_(false),
	readyForSims_(false),
	strategy_(""),
	thresholdsTechnique_(""),
    minImportance_(static_cast<ImportanceValue>(0u)),
    maxImportance_(static_cast<ImportanceValue>(0u)),
    minRareImportance_(static_cast<ImportanceValue>(0u)),
    adhocFun_()
{
	if (find(begin(names), end(names), name) == end(names)) {
		std::stringstream errMsg;
		errMsg << "invalid importance function name \"" << name << "\". ";
		errMsg << "Available importance functions are";
		for (const auto& name: names)
			errMsg << " \"" << name << "\"";
		errMsg << "\n";
		throw_FigException(errMsg.str());
	}
}


const std::string&
ImportanceFunction::name() const noexcept
{
	return name_;
}


bool
ImportanceFunction::has_importance_info() const noexcept
{
	return hasImportanceInfo_;
}


bool
ImportanceFunction::ready() const noexcept
{
	return readyForSims_;
}


const std::string
ImportanceFunction::strategy() const noexcept
{
	return has_importance_info() ? ("" == strategy_ ? "flat" : strategy_)
								 : "";
}


const std::string
ImportanceFunction::thresholds_technique() const noexcept
{
	return ready() ? thresholdsTechnique_ : "";
}


unsigned
ImportanceFunction::num_thresholds() const
{
    if (!ready())
		throw_FigException("this ImportanceFunction hasn't "
						   "any thresholds built in it yet");
	return maxImportance_;
}


ImportanceValue
ImportanceFunction::min_importance() const noexcept
{
    return has_importance_info() ? minImportance_
                                 : static_cast<ImportanceValue>(0u);
}


ImportanceValue
ImportanceFunction::max_importance() const noexcept
{
	return has_importance_info() ? maxImportance_
								 : static_cast<ImportanceValue>(0u);
}


ImportanceValue
ImportanceFunction::min_rare_importance() const noexcept
{
	return has_importance_info() ? minRareImportance_
								 : static_cast<ImportanceValue>(0u);
}

} // namespace fig
