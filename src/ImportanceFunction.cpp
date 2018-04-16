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
#include <random>     // for random_sample()
#include <limits>
#include <numeric>    // std::numeric_limits<>()
#include <iterator>   // std::begin(), std::end()
#include <algorithm>  // std::find()
#include <functional>   // std::function<>, std::bind()
#include <type_traits>  // std::is_assignable<>
// FIG
#include <string_utils.h>
#include <ImportanceFunction.h>
#include <SimulationEngine.h>
#include <ThresholdsBuilder.h>
#include <ThresholdsBuilderAdaptive.h>
#include <FigException.h>
#include <Property.h>

// ADL
using std::find;
using std::begin;
using std::end;


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
    exprStr_ = exprtk_format(formula);
	NVARS_ = std::distance(begin(varnames), end(varnames));
	varsNames_.clear();
	varsNames_.reserve(NVARS_);
	varsPos_.clear();
	varsPos_.reserve(NVARS_);
    varsValues_.resize(NVARS_);
    auto pos_of_var = wrap_mapper(obj);
    NVARS_ = 0ul;
    for (const std::string& var: varnames) {
        if (find(begin(varsNames_),end(varsNames_),var) == end(varsNames_)
                && exprStr_.find(var) != std::string::npos) {
            varsNames_.emplace_back(var);                  // map var
            varsPos_.emplace_back(pos_of_var(var));        // map global pos
            NVARS_++;
        }
    }
	varsNames_.shrink_to_fit();
	varsPos_.shrink_to_fit();
	NVARS_ = varsNames_.size();
    varsValues_.resize(NVARS_);
    compile_expression();
	pinned_ = true;
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
	NVARS_ = 0ul;
	varsNames_.clear();
	varsPos_.clear();
	varsValues_.clear();
    pinned_ = false;
    compile_expression();
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
	return static_cast<ImportanceValue>(expr_.value());
}


ImportanceValue
ImportanceFunction::Formula::operator()(const ImportanceVec& localImportances) const
{
	if (!pinned())
		throw_FigException("this Formula is empty!");
	// Copy the values internally...
	for (size_t i = 0ul ; i < NVARS_ ; i++) {
		assert(!IS_SOME_EVENT(localImportances[varsPos_[i]]));
		varsValues_[i] = localImportances[varsPos_[i]];  // NOTE see other note
	}
	// ...and evaluate
    return static_cast<ImportanceValue>(expr_.value());
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
	simEngine_(""),
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


const std::string&
ImportanceFunction::sim_engine_bound() const noexcept
{
	return simEngine_;
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


unsigned
ImportanceFunction::num_thresholds() const
{
#ifndef NDEBUG
	if (!ready())
		throw_FigException("this ImportanceFunction hasn't "
		                   "any thresholds built in it yet");
#endif
	assert(threshold2importance_.size() > 1ul);
	return threshold2importance_.size() - 2ul;
}


const ThresholdsVec&
ImportanceFunction::thresholds() const
{
#ifndef NDEBUG
	if (!ready())
		throw_FigException("this ImportanceFunction hasn't "
						   "any thresholds built in it yet");
#endif
	return threshold2importance_;
}


unsigned long
ImportanceFunction::min_thresholds_effort(bool dryrun) const
{
#ifndef NDEBUG
	if (!ready() && !dryrun)
		throw_FigException("this ImportanceFunction hasn't "
		                   "any thresholds built in it yet");
	else
		assert(0ul < minThresholdsEffort_);
#endif
	return dryrun ? (1ul<<0ul) : minThresholdsEffort_;
}


unsigned long
ImportanceFunction::max_thresholds_effort(bool dryrun) const
{
#ifndef NDEBUG
	if (!ready() && !dryrun)
        throw_FigException("this ImportanceFunction hasn't "
                           "any thresholds built in it yet");
	else
		assert(0ul < maxThresholdsEffort_ || dryrun);
#endif
	return dryrun ? (1ul<<3ul) : maxThresholdsEffort_;
}


PostProcessing
ImportanceFunction::post_processing() const noexcept
{
	return PostProcessing();
}


ImportanceValue
ImportanceFunction::level_of(const ImportanceValue& imp) const
{
	if (!ready())
#ifndef NDEBUG
		throw_FigException("importance function \"" + name_ + "\" "
						   "doesn't hold thresholds information.");
#else
		return static_cast<ImportanceValue>(0u);
#endif
	assert(imp >= minValue_);
	assert(imp <= maxValue_);
	if (importance2threshold_.size() > 0ul)  // Do we have the direct map?
		return importance2threshold_[imp].first;
	ImportanceValue tlvl(threshold2importance_.size()/2ul), step(tlvl/2);
	while (imp <  threshold2importance_[tlvl].first ||
	       imp >= threshold2importance_[tlvl+1].first) {
		if (imp < threshold2importance_[tlvl].first)
			tlvl -= step;
		else
			tlvl += step;
		step = std::max(static_cast<ImportanceValue>(1), step/2);
	}
	return tlvl;
}


unsigned long
ImportanceFunction::effort_of(const ImportanceValue& lvl) const
{
	if (!ready())
#ifndef NDEBUG
		throw_FigException("importance function \"" + name_ + "\" "
		                   "doesn't hold thresholds information.");
#else
		return static_cast<ImportanceValue>(0u);
#endif
	assert(lvl < threshold2importance_.size());
	return threshold2importance_[lvl].second;
}


ImportanceValue
ImportanceFunction::min_value(bool returnImportance) const noexcept
{
	return ready() && !returnImportance
	        ? level_of(minValue_)
	        : has_importance_info() ? minValue_
	                                : static_cast<ImportanceValue>(0u);
}


ImportanceValue
ImportanceFunction::max_value(bool returnImportance) const noexcept
{
	return ready() && !returnImportance
	        ? level_of(maxValue_)
	        : has_importance_info() ? maxValue_
	                                : static_cast<ImportanceValue>(0u);
}


ImportanceValue
ImportanceFunction::min_rare_value(bool returnImportance) const noexcept
{
	return ready() && !returnImportance
	        ? level_of(minRareValue_)
	        : has_importance_info() ? minRareValue_
	                                : static_cast<ImportanceValue>(0u);
}


ImportanceValue
ImportanceFunction::initial_value(bool returnImportance) const noexcept
{
    return ready() && !returnImportance
	        ? level_of(initialValue_)
	        : has_importance_info() ? initialValue_
	                                : static_cast<ImportanceValue>(0u);
}


std::vector<ImportanceValue>
ImportanceFunction::random_sample(State<STATE_INTERNAL_TYPE> s,
                                  size_t numValues) const
{
	auto randomSamplePairs(random_sample2(s, numValues));
	std::vector<ImportanceValue> randomSample;
	randomSample.reserve(randomSamplePairs.size());
	for (auto& p: randomSamplePairs)
		randomSample.emplace_back(p.second);
	return randomSample;
}


std::set<std::pair<uint128_t, ImportanceValue>>
ImportanceFunction::random_sample2(State<STATE_INTERNAL_TYPE> s,
                                  size_t numValues) const
{
	static std::mt19937 RNG(std::mt19937::default_seed);
	if (!has_importance_info())
		throw_FigException("importance function \"" + name() + "\" "
		                   "has no importance information");
	static const uint128_t INI_CONCRETE_STATE(uint128::uint128_0);
	const uint128_t NUM_CONCRETE_STATES(s.concrete_size());
	assert(INI_CONCRETE_STATE < NUM_CONCRETE_STATES);
	std::uniform_int_distribution<size_t> anyCStateL(INI_CONCRETE_STATE, NUM_CONCRETE_STATES.lower());
	std::uniform_int_distribution<size_t> anyCStateU(INI_CONCRETE_STATE, NUM_CONCRETE_STATES.upper());
	std::set<std::pair<uint128_t,ImportanceValue>> randomSample;
	for (auto i = 0ul ; i < numValues ; i++) {
		uint128_t cs(anyCStateU(RNG), anyCStateL(RNG));
		randomSample.emplace(cs, importance_of(s.decode(cs)));
	}
	return randomSample;
}


void
ImportanceFunction::bind_sim_engine(const std::string& name) const
{
	assert(!name.empty());
	static std::vector< std::string > simulatorsNames;
	if (simulatorsNames.empty()) {
		simulatorsNames.reserve(SimulationEngine::NUM_NAMES);
		for (const auto& name: SimulationEngine::names())
			simulatorsNames.emplace_back(name);
	}
	assert(!simulatorsNames.empty());
	if (find(begin(simulatorsNames), end(simulatorsNames), name) == end(simulatorsNames))
		throw_FigException("cannot bind to invalid SimulationEgine name \"" + name + "\"");
	else
		simEngine_ = name;
}


void ImportanceFunction::unbind_sim_engine() const
{
	simEngine_.clear();
}


void
ImportanceFunction::build_thresholds(ThresholdsBuilder& tb)
{
	if (!has_importance_info())
		throw_FigException("importance function \"" + name() + "\" "
		                   "has no importance information");
	ThresholdsVec().swap(threshold2importance_);
	ThresholdsVec().swap(importance2threshold_);
	thresholdsTechnique_ = "";
    readyForSims_ = false;
    threshold2importance_ = tb.build_thresholds(shared_from_this());
    post_process_thresholds(tb);
}


void
ImportanceFunction::post_process_thresholds(const ThresholdsBuilder& tb)
{
	if (static_cast<size_t>(maxValue_-minValue_) < (MAX_MEM_REQ/8ul) &&
			strategy_ != "flat") {
		// Few importance values: we can afford direct "translator"
		importance2threshold_ = tb.invert_thresholds_map(threshold2importance_);
		// Check the consistency of the translator built
		assert(!importance2threshold_.empty());
		assert(importance2threshold_[0].first == static_cast<ImportanceValue>(0u));
		assert(importance2threshold_[0].first <= importance2threshold_.back().first);
		// As specified in ThresholdsBuilder::invert_thresholds_map() implementation:
		assert(0u == importance2threshold_[minValue_].first);
		assert(0u == importance2threshold_[initialValue_].first);
		assert(threshold2importance_.size() == importance2threshold_[maxValue_].first + 2u);
		// Threshold levels are a non-decreasing function of the importance:
		assert(importance2threshold_[minValue_].first <= importance2threshold_[initialValue_].first);
		assert(importance2threshold_[initialValue_].first <= importance2threshold_[minRareValue_].first);
		assert(importance2threshold_[minRareValue_].first <= importance2threshold_[maxValue_].first);
	}
	// Set relevant attributes
	using myLimits = std::numeric_limits<decltype(minThresholdsEffort_)>;  // I'm this close...
	minThresholdsEffort_ = myLimits::max();
	for (const auto& p: threshold2importance_)
		minThresholdsEffort_ = std::min(minThresholdsEffort_, p.second);
	maxThresholdsEffort_ = myLimits::min();
	for (const auto& p: threshold2importance_)
		maxThresholdsEffort_ = std::max(maxThresholdsEffort_, p.second);
	thresholdsTechnique_ = tb.name;
	readyForSims_ = true;
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
	ThresholdsVec().swap(threshold2importance_);
	ThresholdsVec().swap(importance2threshold_);
	userFun_.reset();
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
		minI = std::min(minI, importance);
		maxI = std::max(maxI, importance);
		if (property.is_rare(symbState) && importance < minrI) {
			minrI = importance;
			/// @bug FIXME This rare state may be unreachable; how to know?
		}
	}
//	NOTE: A separate loop for 'maxI' is only needed when OpenMP is used
//	#pragma omp parallel for default(shared) private(state) reduction(max:maxI)
//	for (size_t i = 0ul ; i < NUM_CONCRETE_STATES ; i++) {
//		const ImportanceValue importance =
//				importance_of(state.decode(i).to_state_instance());
//		maxI = importance > maxI ? importance : maxI;
//    }

	assert(minI <= minrI);
	assert(minrI <= maxI);

	minValue_ = minI;
	maxValue_ = maxI;
	minRareValue_ = minrI;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
