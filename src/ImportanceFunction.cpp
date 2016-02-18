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
#include <vector>
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

const std::array< std::string, 3 > ImportanceFunction::names =
{{
	// See ImportanceFunctionConcreteCoupled class
	"concrete_coupled",

	// See ImportanceFunctionConcreteSplit class
	"concrete_split",

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


ImportanceValue
ImportanceFunction::Formula::operator()(const ImportanceVec& localImportances) const
{
	// Bind symbolic state variables to the current expression...
	for (const auto& pair: varsMap_)
		expr_.DefineVar(pair.first,  const_cast<ImportanceValue*>(
						&localImportances[pair.second]));
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
	numThresholds_(0u),
	userFun_()
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
ImportanceFunction::adhoc_fun() const noexcept
{
	return has_importance_info() && "adhoc" == strategy_ ? userFun_.expression()
														 : "";
}


const std::string
ImportanceFunction::thresholds_technique() const noexcept
{
	return ready() ? thresholdsTechnique_ : "";
}


const unsigned&
ImportanceFunction::num_thresholds() const
{
    if (!ready())
		throw_FigException("this ImportanceFunction hasn't "
						   "any thresholds built in it yet");
	return numThresholds_;
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


void
ImportanceFunction::clear() noexcept
{
	hasImportanceInfo_ = false;
	readyForSims_ = false;
	strategy_ = "";
	thresholdsTechnique_ = "";
	minImportance_ = static_cast<ImportanceValue>(0u);
	maxImportance_ = static_cast<ImportanceValue>(0u);
	minRareImportance_ = static_cast<ImportanceValue>(0u);
	numThresholds_ = 0u;
	userFun_.reset();
}

} // namespace fig
