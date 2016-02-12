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

std::vector< std::string > emptyFormulaVarnames;


ImportanceFunction::Formula::Formula() : MathExpression("", emptyFormulaVarnames)
{ /* Not much to do around here */ }


void
ImportanceFunction::Formula::reset(const std::string& formula,
								   const std::vector< std::string >& varnames,
								   const State<STATE_INTERNAL_TYPE>& globalState)
{
	exprStr_ = formula;
	empty_ = "" == formula;
	parse_our_expression();  // updates expr_
	varsMap_.clear();
	for (const auto& name: varnames) {
#ifndef NRANGECHK
		if (std::string::npos == exprStr.find(name))
			throw std::out_of_range(std::string("invalid variable name: \"")
									.append(name).append("\""));
#endif
		varsMap_.emplace_back(       // copy elision
				std::make_pair(name, globalState.position_of_var(name)));
	}
	pinned_ = true;
}


STATE_INTERNAL_TYPE
ImportanceFunction::Formula::operator()(const StateInstance& state) const
{
	/// @todo TODO: implement!
}



// ImportanceFunction class member functions

ImportanceFunction::ImportanceFunction(const std::string& name) :
	name_(name),
	hasImportanceInfo_(false),
	readyForSims_(false),
	strategy_(""),
	thresholdsTechnique_(""),
	maxImportance_(0u)
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
