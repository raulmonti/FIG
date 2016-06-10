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
#include <string_utils.h>
#include <ImportanceFunction.h>
#include <ThresholdsBuilder.h>
#include <ThresholdsBuilderAdaptive.h>
#include <FigException.h>
#include <Property.h>

// ADL
using std::begin;
using std::end;
using std::find;


namespace  // // // // // // // // // // // // // // // // // // // // // // //
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

} // namespace  // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

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
	exprStr_ = muparser_format(formula);
	parse_our_expression();  // updates expr_
	NVARS_ = std::distance(begin(varnames), end(varnames));
	varsNames_.clear();
	varsNames_.reserve(NVARS_);
	varsPos_.clear();
	varsPos_.reserve(NVARS_);
	varsValues_.resize(NVARS_);
	try {
		auto pos_of_var = wrap_mapper(obj);
		NVARS_ = 0ul;
		for (const std::string& var: varnames) {
			if (find(begin(varsNames_),end(varsNames_),var) == end(varsNames_)
					&& exprStr_.find(var) != std::string::npos) {
				varsNames_.emplace_back(var);                  // map var
				varsPos_.emplace_back(pos_of_var(var));        // map global pos
				expr_.DefineVar(var, &varsValues_[NVARS_++]);  // hook to expr_
			}
		}
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
	varsNames_.shrink_to_fit();
	varsPos_.shrink_to_fit();
	NVARS_ = varsNames_.size();
	varsValues_.resize(NVARS_);  // breaks references in expr_?
	pinned_ = true;

    // Fake an evaluation to reveal parsing errors but now... I said NOW!
    try {
		STATE_INTERNAL_TYPE dummy(static_cast<STATE_INTERNAL_TYPE>(1.1));
		for (STATE_INTERNAL_TYPE& val: varsValues_)
			val = dummy;
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
	NVARS_ = 0ul;
	varsNames_.clear();
	varsPos_.clear();
	varsValues_.clear();
    pinned_ = false;
}


ImportanceValue
ImportanceFunction::Formula::operator()(const StateInstance& state) const
{
	if (!pinned())
		throw_FigException("this Formula is empty!");
	// Copy the useful part of 'state'...
	for (size_t i = 0ul ; i < NVARS_ ; i++)
		varsValues_[i] = state[varsPos_[i]];  // ugly motherfucker
	/// @todo NOTE As an alternative we could use memcpy() to copy the values,
	///            but that means bringing a whole chunk of memory of which
	///            only a few variables will be used. To lighten that we could
	///            impose an upper bound on the number of variables/modules,
	///            but then the language's flexibility will be compromised.
	// ...and evaluate
    return static_cast<ImportanceValue>(expr_.Eval());
}


ImportanceValue
ImportanceFunction::Formula::operator()(const ImportanceVec& localImportances) const
{
	if (!pinned())
		throw_FigException("this Formula is empty!");
	// Copy the values internally...
	for (size_t i = 0ul ; i < NVARS_ ; i++)
		varsValues_[i] = static_cast<STATE_INTERNAL_TYPE>(
							 localImportances[varsPos_[i]]);  // NOTE see other note
	// ...and evaluate
	return static_cast<ImportanceValue>(expr_.Eval());
}


const std::vector<std::string>&
ImportanceFunction::Formula::get_free_vars() const noexcept
{
	return varsNames_;
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
	importance2threshold_(),
	userFun_()
{
	if (find(begin(names()), end(names()), name) == end(names())) {
		std::stringstream errMsg;
		errMsg << "invalid importance function name \"" << name << "\". ";
		errMsg << "Available importance functions are";
		for (const auto& name: names())
			errMsg << " \"" << name << "\"";
		errMsg << "\n";
		throw_FigException(errMsg.str());
	}
}


const std::array<std::string, ImportanceFunction::NUM_NAMES>&
ImportanceFunction::names() noexcept
{
	static const std::array< std::string, NUM_NAMES > names =
	{{
		// See ImportanceFunctionConcreteCoupled class
		"concrete_coupled",

		// See ImportanceFunctionConcreteSplit class
		"concrete_split",

		// See ImportanceFunctionAlgebraic class
		"algebraic"
	}};
	return names;
}


const std::array< std::string, ImportanceFunction::NUM_STRATEGIES>&
ImportanceFunction::strategies() noexcept
{
	static const std::array< std::string, NUM_NAMES > strategies=
	{{
		// Flat importance, i.e. null ImportanceValue for all states
		"flat",

		// Automatically built importance, by means of a backwards BFS analysis
		"auto",

		// User defined importance, by means of an algebraic expression on the states
		"adhoc"
	}};
	return strategies;
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
	if (has_importance_info() &&
			("adhoc" == strategy_ || "concrete_split" == name_))
		return userFun_.expression();
	else
		return "";
}


const std::string
ImportanceFunction::thresholds_technique() const noexcept
{
	return ready() ? thresholdsTechnique_ : "";
}


const unsigned&
ImportanceFunction::num_thresholds() const
{
#ifndef NDEBUG
	if (!ready())
		throw_FigException("this ImportanceFunction hasn't "
						   "any thresholds built in it yet");
#endif
	assert(thresholds2importance_.size() > 1ul);
	return thresholds2importance_.size() - 2ul;
}


std::string
ImportanceFunction::post_processing() const noexcept
{
	return "";
}


ImportanceValue
ImportanceFunction::level_of(const ImportanceValue& val) const
{
	if (!ready())
#ifndef NDEBUG
		throw_FigException("importance function \"" + name_ + "\" "
						   "doesn't hold thresholds information.");
#else
		return static_cast<ImportanceValue>(0u);
#endif
	assert(val >= minValue_);
	assert(val <= maxValue_);
	ImportanceValue tlvl(thresholds2importance_.size()/2ul);
	if (importance2threshold_.size() > 0ul) {
		// We have the direct map
		tlvl = importance2threshold_[val];
	} else {
		// Search the corresponding threshold level
		while (val <  thresholds2importance_[tlvl] ||
			   val >= thresholds2importance_[tlvl+1]) {
			if (val < thresholds2importance_[tlvl])
				tlvl *= 2;
			else
				tlvl /= 2;
		}
	}
	return tlvl;
}


ImportanceValue
ImportanceFunction::min_value() const noexcept
{
	return ready() ? level_of(minValue_)
				   : has_importance_info() ? minValue_
										   : static_cast<ImportanceValue>(0u);
}


ImportanceValue
ImportanceFunction::max_value() const noexcept
{
	return ready() ? level_of(maxValue_)
				   : has_importance_info() ? maxValue_
										   : static_cast<ImportanceValue>(0u);
}


ImportanceValue
ImportanceFunction::min_rare_value() const noexcept
{
	return ready() ? level_of(minRareValue_)
				   : has_importance_info() ? minRareValue_
										   : static_cast<ImportanceValue>(0u);
}


ImportanceValue
ImportanceFunction::initial_value() const noexcept
{
	return ready() ? level_of(initialValue_)
				   : has_importance_info() ? initialValue_
										   : static_cast<ImportanceValue>(0u);
}


void
ImportanceFunction::build_thresholds(
	ThresholdsBuilder& tb,
	const unsigned& spt)
{
	if (!has_importance_info())
		throw_FigException("importance function \"" + name() + "\" "
						   "doesn't yet have importance information");
	std::vector< ImportanceValue >().swap(importance2threshold_);
	readyForSims_ = false;
	thresholds2importance_ = tb.build_thresholds(spt, *this, post_processing());
	post_process_thresholds(tb.name);
}


void
ImportanceFunction::build_thresholds_adaptively(
	ThresholdsBuilderAdaptive& atb,
	const unsigned& spt,
	const float& p,
	const unsigned& n)
{
	if (!has_importance_info())
		throw_FigException("importance function \"" + name() + "\" "
						   "doesn't yet have importance information");
	std::vector< ImportanceValue >().swap(importance2threshold_);
	readyForSims_ = false;
	importance2threshold_ = atb.build_thresholds(spt, *this, p, n);
	post_process_thresholds(atb.name);
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
	std::vector< ImportanceValue >().swap(importance2threshold_);
	userFun_.reset();
}


void
ImportanceFunction::post_process_thresholds(const std::string& tbName)
{
	// Check the consistency of the "translator" built
	assert(!importance2threshold_.empty());
	assert(importance2threshold_[0] == static_cast<ImportanceValue>(0u));
	assert(importance2threshold_[0] <= importance2threshold_.back());
	// (threshold levels are a non-decreasing function of the importance)
	assert(importance2threshold_[minValue_] <= importance2threshold_[initialValue_]);
	assert(importance2threshold_[initialValue_] <= importance2threshold_[minRareValue_]);
	assert(importance2threshold_[minRareValue_] <= importance2threshold_[maxValue_]);

	// Set relevant attributes
	thresholdsTechnique_ = tbName;
	readyForSims_ = true;
}


void
ImportanceFunction::find_extreme_values(State<STATE_INTERNAL_TYPE> state,
										const Property& property)
{
	if (state.concrete_size().upper() > 0ul)
		throw_FigException("state is too big to perform this search; "
						   "cowardly aborting");

	const size_t NUM_CONCRETE_STATES(state.concrete_size().lower());
	ImportanceValue minI = std::numeric_limits<ImportanceValue>::max();
	ImportanceValue maxI = std::numeric_limits<ImportanceValue>::min();
	ImportanceValue minrI = std::numeric_limits<ImportanceValue>::max();

//	#pragma omp parallel for default(shared) private(state) reduction(min:minI,minrI)
	for (size_t i = 0ul ; i < NUM_CONCRETE_STATES ; i++) {
		const StateInstance symbState = state.decode(i).to_state_instance();
		const ImportanceValue importance = importance_of(symbState);
		minI = importance < minI ? importance : minI;
		if (property.is_rare(symbState) && importance < minrI) {
			minrI = importance;
			/// @bug FIXME This rare state may be unreachable; how to know?
		}
	}
    assert(minI <= minrI);

//	#pragma omp parallel for default(shared) private(state) reduction(max:maxI)
	for (size_t i = 0ul ; i < NUM_CONCRETE_STATES ; i++) {
		const ImportanceValue importance =
                importance_of(state.decode(i).to_state_instance());
		maxI = importance > maxI ? importance : maxI;
    }
    assert(minrI <= maxI);

	minValue_ = minI;
	maxValue_ = maxI;
	minRareValue_ = minrI;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
