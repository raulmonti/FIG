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
#include <numeric>    // std::numeric_limits<>()
#include <iterator>   // std::begin(), std::end()
#include <algorithm>  // std::find()
#include <functional>   // std::function<>, std::bind()
#include <type_traits>  // std::is_assignable<>
// FIG
#include <ImportanceFunction.h>
#include <FigException.h>
#include <Property.h>

// ADL
using std::begin;
using std::end;
using std::find;


namespace
{

std::function<size_t(const std::string&)>
wrap_mapper(const fig::PositionsMap& obj)
{
    return [&obj](const std::string& varName) { return obj.at(varName); };
}

std::function<size_t(const std::string&)>
wrap_mapper(const fig::State<fig::STATE_INTERNAL_TYPE>& obj)
{
    return std::bind(&fig::State<fig::STATE_INTERNAL_TYPE>::position_of_var,
                     obj, std::placeholders::_1);
}

} // namespace



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


const std::array< std::string, 3 > ImportanceFunction::strategies =
{{
	// Flat importance, i.e. null ImportanceValue for all states
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


template< template< typename... > class Container,
                    typename... OtherArgs,
          typename Mapper
>
void
ImportanceFunction::Formula::set(
    const std::string& formula,
    const Container<std::string, OtherArgs...>& varnames,
    const Mapper& obj)
{
    static_assert(std::is_same<PositionsMap, Mapper>::value ||
                  std::is_convertible<State<STATE_INTERNAL_TYPE>, Mapper>::value,
                  "ERROR: type mismatch. ImportanceFunction::Formula::set() can"
                  " only be called with a State<...> object or a PositionsMap.");
    if ("" == formula || formula.length() == 0ul)
        throw_FigException("can't define an empty user function");

    empty_ = false;
    exprStr_ = formula;
    parse_our_expression();  // updates expr_
    varsMap_.clear();
    auto pos_of_var = wrap_mapper(obj);
    for (const auto& name: varnames)
        if (std::string::npos != formula.find(name))  // name occurs in formula
            varsMap_.emplace_back(std::make_pair(name, pos_of_var(name)));
    pinned_ = true;
    // Fake an evaluation to reveal parsing errors but now... I said NOW!
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

// ImportanceFunction::Formula::set() can only be invoked
// with the following Container and Mapper types
typedef State<STATE_INTERNAL_TYPE> state_t;
template void ImportanceFunction::Formula::set(
    const std::string&, const std::set<std::string>&, const PositionsMap&);
template void ImportanceFunction::Formula::set(
    const std::string&, const std::set<std::string>&, const state_t&);
template void ImportanceFunction::Formula::set(
    const std::string&, const std::list<std::string>&, const PositionsMap&);
template void ImportanceFunction::Formula::set(
    const std::string&, const std::list<std::string>&, const state_t&);
template void ImportanceFunction::Formula::set(
    const std::string&, const std::deque<std::string>&, const PositionsMap&);
template void ImportanceFunction::Formula::set(
    const std::string&, const std::deque<std::string>&, const state_t&);
template void ImportanceFunction::Formula::set(
    const std::string&, const std::vector<std::string>&, const PositionsMap&);
template void ImportanceFunction::Formula::set(
    const std::string&, const std::vector<std::string>&, const state_t&);
template void ImportanceFunction::Formula::set(
    const std::string&, const std::forward_list<std::string>&, const PositionsMap&);
template void ImportanceFunction::Formula::set(
    const std::string&, const std::forward_list<std::string>&, const state_t&);
template void ImportanceFunction::Formula::set(
    const std::string&, const std::unordered_set<std::string>&, const PositionsMap&);
template void ImportanceFunction::Formula::set(
    const std::string&, const std::unordered_set<std::string>&, const state_t&);


void
ImportanceFunction::Formula::reset() noexcept
{
    empty_ = true;
    exprStr_ = "1";
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
        expr_.DefineVar(pair.first, reinterpret_cast<STATE_INTERNAL_TYPE*>(
                                    const_cast<ImportanceValue*>(
                                    &localImportances[pair.second])));
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
	minValue_(static_cast<ImportanceValue>(0u)),
	maxValue_(static_cast<ImportanceValue>(0u)),
	minRareValue_(static_cast<ImportanceValue>(0u)),
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
	return has_importance_info() ? strategy_ : "";
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
ImportanceFunction::min_value() const noexcept
{
	return has_importance_info() ? minValue_ : static_cast<ImportanceValue>(0u);
}


ImportanceValue
ImportanceFunction::max_value() const noexcept
{
	return has_importance_info() ? maxValue_ : static_cast<ImportanceValue>(0u);
}


ImportanceValue
ImportanceFunction::min_rare_value() const noexcept
{
	return has_importance_info() ? minRareValue_ : static_cast<ImportanceValue>(0u);
}


void
ImportanceFunction::clear() noexcept
{
	hasImportanceInfo_ = false;
	readyForSims_ = false;
	strategy_ = "";
	thresholdsTechnique_ = "";
	minValue_ = static_cast<ImportanceValue>(0u);
	maxValue_ = static_cast<ImportanceValue>(0u);
	minRareValue_ = static_cast<ImportanceValue>(0u);
	numThresholds_ = 0u;
	userFun_.reset();
}


void
ImportanceFunction::find_extreme_values(State<STATE_INTERNAL_TYPE> state,
										const Property& property)
{
	minImportance_ = std::numeric_limits<ImportanceValue>::max();
	maxImportance_ = std::numeric_limits<ImportanceValue>::min();
	minRareImportance_ = std::numeric_limits<ImportanceValue>::max();

	/**
	 * @todo FIXME This brute force attack takes too long,
	 *       big state spaces kill it (e.g. Glasserman's ATM)
	 *
	 * Finding these extreme values affects two things:
	 * 1. RESTART oversampling correction in ConfidenceInterval
	 * 2. Thresholds builders stopping criterion
	 *
	 * Raúl's idea is to explore the "importance space" which is always
	 * a subset of the state space.
	 * Issues: a) how to find minRareImportance without looking the states
	 *         b) how to access each module's importance space
	 *
	 * Another workarounds regarding the stopping criterion are:
	 * 2.1- implement automatic (fixed) thresholds selection,
	 *      e.g. select one every two importance levels, or skip
	 *      first three and then use all the rest;
	 * 2.2- let AMS finish early, viz. when it fails to find new thresholds
	 *      'N' times, consider last threshold found as the final threshold
	 *      and return successfully.
	 */

//	#pragma omp parallel for default(shared) private(gStateCopy) reduction(min:imin,iminRare)
	for (size_t i = 0ul ; i < state.concrete_size() ; i++) {
		const StateInstance symbState = state.decode(i).to_state_instance();
		const ImportanceValue importance = importance_of(symbState);
		minImportance_ = importance < minImportance_ ? importance
													 : minImportance_;
		if (property.is_rare(symbState) && importance < minRareImportance_)
			minRareImportance_ = importance;
	}

//	#pragma omp parallel for default(shared) private(gStateCopy) reduction(max:imax)
	for (size_t i = 0ul ; i < state.concrete_size() ; i++) {
		const ImportanceValue importance =
                importance_of(state.decode(i).to_state_instance());
		maxImportance_ = importance > maxImportance_ ? importance
													 : maxImportance_;
	}
}

} // namespace fig
