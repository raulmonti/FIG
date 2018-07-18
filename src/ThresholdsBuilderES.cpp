//==============================================================================
//
//  ThresholdsBuilderES.cpp
//
//  Copyleft 2017-
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

// C
#include <cmath>  // std::log(), std::round(), etc.
#include <cassert>
// C++
#include <vector>
#include <deque>
#include <memory>      // std::make_unique<>
#include <algorithm>   // std::fill()
#include <functional>  // std::bind()
#include <iomanip>     // std::setprecision()
// FIG
#include <ThresholdsBuilderES.h>
#include <SimulationEngineSFE.h>
#include <ImportanceFunction.h>
#include <PropertyTransient.h>
#include <PropertyRate.h>
#include <ModuleNetwork.h>
#include <ModelSuite.h>
#include <TraialPool.h>
#include <Traial.h>
#include <FigLog.h>
#include <Util.h>

// ADL
using std::begin;
using std::end;

using PathCandidate = fig::SimulationEngineFixedEffort::ThresholdsPathCandidates;


namespace   // // // // // // // // // // // // // // // // // // // // // // //
{

/// Show in tech log the probability of level-up for each level
void
print_lvlup(const std::vector< float >& Pup)
{
	float est(1.0f);
	const auto defaultLogFlags(fig::figTechLog.flags());
	fig::figTechLog << std::setprecision(2) << std::scientific;
	fig::figTechLog << "\nLvl-up probabilities:";
	for (auto p: Pup) {
		fig::figTechLog << " " << p;
		est *= p;
	}
	fig::figTechLog << "\nEstimate spoiler (transient properties only!): "
			   << est << std::endl;
	fig:: figTechLog.flags(defaultLogFlags);
}


/// Retrieve initial importance of the ImportanceFunction instance,
/// taking care to ask for <i>importance</i> and not for <i>threshold-level</i>.
fig::ImportanceValue
initial_importance(const fig::ImportanceFunction& ifun)
{
    return ifun.initial_value(true);
}


/// Return the element from \p paths that reached the highest ImportanceValue;
/// if there are N>1 such paths then average their probability values.
/// @warning Side effect: \p paths is deleted
PathCandidate
choose_best_path_to_rare(std::vector< PathCandidate >& paths)
{
	assert(!paths.empty());
	using Importance = fig::ImportanceValue;
	PathCandidate bestPath(paths.front());
	Importance bestPathImportance(bestPath.front().back().first);
	auto importance_of_at = [] (const PathCandidate& path, const size_t i) -> Importance
		{
			return path.front().at(i).first;
		};
	auto top_importance = [&paths] (size_t i) -> Importance
		{
			return paths[i].front().back().first;
		};
	auto is_better_path = [&paths,&bestPath,bestPathImportance,&top_importance](size_t i) -> bool
		{
			return top_importance(i) > bestPathImportance
					|| (top_importance(i) == bestPathImportance
						&& paths[i].front().size() > bestPath.front().size());
		};
	//\/////////////////////////////////////////////////////////////////////
	for (size_t i=0ul ; i < paths.size() ; i++) {
		if (is_better_path(i)) {
			bestPath = paths[i];
			bestPathImportance = top_importance(i);
		}
	}
	std::vector< PathCandidate > bestPaths;
	for (auto path: paths) {
		if (path.front().size() == bestPath.front().size()) {
			for (auto j=0ul ; j < path.front().size() ; j++) {
				if (importance_of_at(path,j) != importance_of_at(bestPath,j))
					goto discard_this_path;
			}
			bestPaths.emplace_back(path);  // same as bestPath: store
		}
		discard_this_path:
		;
	}
	assert(!bestPaths.empty());  // at least bestPath should've been stored
	const auto NUM_BEST_PATHS(static_cast<float>(bestPaths.size()));
	PathCandidate result(1);
	auto& resultContent = result.front();
	resultContent.reserve(bestPath.front().size());
	for (auto i=0ul ; i < bestPath.front().size() ; i++) {
		float acc(0.0f);
		for (auto& path: bestPaths)            // average over all bestPaths
			acc += path.front().at(i).second;  // probability of i-th jump in this path
		resultContent.emplace_back(importance_of_at(bestPath,i), acc/NUM_BEST_PATHS);
	}
	std::vector<PathCandidate>().swap(paths);
	assert (!result.empty());
	assert (!resultContent.empty());
	return result;
}


/// Get the probability of going up the \p i-th level according to \p path.
/// If it is \p null, i.e. level i+1 was no reached, then return the trend
/// of the previous two levels.
double
probability_next_lvl_up(const PathCandidate::value_type& path, const size_t& i)
{
	assert(path.size() > i);
	if (path[i].second > 0.0)
		return path[i].second;  // path has the desired probability
	else if (1 == i)
		return path[0].second;  // only one point of information available
	else
		return (path[i-1].second*path[i-1].second)/path[i-2].second;
}

} // namespace   // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

using EventWatcher = SimulationEngine::EventWatcher;


ThresholdsBuilderES::ThresholdsBuilderES(
		std::shared_ptr<const ModuleNetwork> model,
		const size_t& n) :
    ThresholdsBuilder("es"),  // virtual inheritance forces this...
	ThresholdsBuilderAdaptive(n),
    nSims_(0ul),
    maxSimLen_(0ul),
    property_(nullptr),
    model_(nullptr),
	impFun_(nullptr),
	internalSimulator_(make_unique<SimulationEngineSFE>(model, true))
{ /* Not much to do around here */ }


void
ThresholdsBuilderES::setup(std::shared_ptr<const Property> property,
                           const unsigned)
{
	property_ = property;
}


ThresholdsVec
ThresholdsBuilderES::build_thresholds(std::shared_ptr<const ImportanceFunction> impFun)
{
	model_ = ModelSuite::get_instance().modules_network();
	impFun_ = impFun;

	// Adapted from Alg. 3 of Budde, D'Argenio, Hartmanns,
	// "Better Automated Importance Splitting for Transient Rare Events,"
	// SETTA 2017.
	tune();
	n_ = nSims_;
	internalSimulator_.get()->arbitraryLevelEffort = n_;
	ModelSuite::tech_log("Building thresholds with \"" + name
						+ "\" for num_sims,sim_len = " + std::to_string(n_)
	                    + "," + std::to_string(maxSimLen_));

	// Probe for reachable importance values to select threshold candidates
	const bool forceRealMax(0.0 <= ModelSuite::get_DFT());
	ImportanceVec thrCandidates(reachable_importance_values(forceRealMax));
	if (thrCandidates.size() < 2ul)  // we must have reached beyond initial importance
		throw_FigException("ES could not find reachable importance values");
	if (highVerbosity) {
		ModelSuite::tech_log("\nFound " + std::to_string(thrCandidates.size())
							+ " reachable importance values:");
		for (auto imp: thrCandidates)
			ModelSuite::tech_log(" "+std::to_string(imp));
	}

	// Estimate probabilities of going from one (reachable) importance value to the next
	auto Pup = FE_for_ES(thrCandidates);
	assert(!currentThresholds_.empty());  // could we build something?
	assert(!Pup.empty());
	if (0.0f >= Pup.back())
		artificial_thresholds_selection(thrCandidates, Pup);
	if (highVerbosity)
		print_lvlup(Pup);

	// Turn level-up probabilities into effort factors
	auto effort(Pup);
	std::fill(begin(effort), end(effort), static_cast<decltype(effort)::value_type>(0));
	for (size_t i = 0ul ; i < Pup.size() ; i++) {
		const auto prev = effort[(i-1)%Pup.size()];
		effort[i] = 1.0f/Pup[i] + (prev-std::round(prev));  // E [# sims to move up from lvl i]
	}

	// Select the thresholds based on the effort factors
	ThresholdsVec().swap(thresholds_);
	thresholds_.reserve(effort.size()+2ul);
	const auto IMP_INI(initial_importance(*impFun));
	const bool THR_HAS_INI(currentThresholds_.front().first == IMP_INI);
	thresholds_.emplace_back(IMP_INI, 1u);
	for (size_t i = THR_HAS_INI ? 1ul : 0ul ; i < Pup.size() ; i++) {
		const int thisEffort = std::round(effort[i]);
		if (1 < thisEffort)
			thresholds_.emplace_back(thrCandidates[i], thisEffort);
	}
	thresholds_.emplace_back(impFun->max_value(true)+static_cast<ImportanceValue>(1u), 1ul);

	// NOTE If these thresholds produce too much splitting,
	//      e.g. thresholds_[i..i+5] = (imp,imp+1,...,imp+4) for effort ~ 10,
	//      pick a threshold, remove it, and increase proportionally
	//      the effort in the thresholds directly below and above it,
	//      e.g. thresholds_[i..i+3] = (imp,imp+2,imp+4) for effort ~ 15.

	ModelSuite::tech_log("\n");
	show_thresholds(thresholds_);
	impFun_.reset();
	model_.reset();
	return thresholds_;
}


ImportanceVec
ThresholdsBuilderES::reachable_importance_values(bool forceRealMax) const
{
	using namespace std::placeholders;  // _1, _2, ...
	const EventWatcher& watch_events =
			std::bind(&ThresholdsBuilderES::importance_seeker, this, _1, _2, _3);

	static constexpr size_t NUM_INDEPENDENT_RUNS = 20ul;
	static constexpr size_t MAX_FAILS = (1ul)<<(10ul);
	size_t numFails(0ul);

    ImportanceValue maxImportanceReached(initial_importance(*impFun_));
	std::unordered_set< ImportanceValue > reachableImpValues = { maxImportanceReached };
	TraialsVec traialsNow(get_traials(NUM_INDEPENDENT_RUNS, *impFun_)),
	           traialsNext;
	assert(!traialsNow.empty());
	assert(traialsNext.empty());
	traialsNext.reserve(traialsNow.size());

	do {
		while (!traialsNow.empty()) {
			Traial& traial(traialsNow.back());
			traialsNow.pop_back();
			const ImportanceValue startImp(traial.level);
			traial.depth = 0;
			traial.numLevelsCrossed = 0;
			model_->simulation_step(traial, *property_, watch_events);
			if (startImp < traial.level) {
				reachableImpValues.emplace(traial.level);
				maxImportanceReached = std::max(maxImportanceReached, traial.level);
				traialsNext.push_back(traial);
			} else {
				if (MAX_FAILS < ++numFails)
					goto end_reachability_analysis;  // assume max importance unreachable and quit
				else if (traialsNow.size() > 1ul)
					traial = traialsNow[numFails%traialsNow.size()].get();
				else
					traial.initialise(*model_,*impFun_);  // start over
				traialsNow.push_back(traial);
			}
			assert(traialsNow.size()+traialsNext.size() == NUM_INDEPENDENT_RUNS);
		}
		std::swap(traialsNow, traialsNext);
	} while (maxImportanceReached < impFun_->max_value());
    end_reachability_analysis:
	    TraialPool::get_instance().return_traials(traialsNow);
		TraialPool::get_instance().return_traials(traialsNext);

	ImportanceVec result(begin(reachableImpValues), end(reachableImpValues));
	std::sort(begin(result), end(result));

	// Some models require ES to reach the (real) max importance value
	if (forceRealMax) {
		const auto maxReachedValue(result.back());
		const auto maxImportanceValue(impFun_->max_value(true));
		const auto numExtraLevels(maxImportanceValue-maxReachedValue);
		if (0 < numExtraLevels && highVerbosity)
			ModelSuite::tech_log("\nReached up to importance value " +
								 std::to_string(maxReachedValue) +
								 "; forcing to reach value " +
								 std::to_string(maxImportanceValue));
		result.reserve(result.size()+numExtraLevels);
		for (auto i=maxReachedValue+1 ; i<=maxImportanceValue ; i++)
			result.push_back(i);
	}

	return result;
}


std::vector< float >
ThresholdsBuilderES::FE_for_ES(const ImportanceVec& reachableImportanceValues) const
{
	std::vector< float > Pup({0.0});
	static constexpr auto NUM_PATHS = 4ul;
	using namespace std::placeholders;  // _1, _2, ...
	const EventWatcher& watch_events =
			std::bind(&ThresholdsBuilderES::FE_watcher, this, _1, _2, _3);

	if (reachableImportanceValues.size() < 2ul)
		return Pup;  // only one reachable importance value: ES failed!

	// Use reachableImportanceValues as candidates to paths to the rare event
	std::vector<PathCandidate> paths(NUM_PATHS);
	ThresholdsVec().swap(currentThresholds_);
	currentThresholds_.reserve(reachableImportanceValues.size());
	for (size_t i = 0ul ; i < reachableImportanceValues.size() ; i++)
		currentThresholds_.emplace_back(reachableImportanceValues[i], 1u);
	assert(nullptr != property_);
	internalSimulator_->property_ = property_.get();
	internalSimulator_->bind(impFun_);
	auto& maxImportanceLvl = internalSimulator_.get()->arbitraryMaxLevel;
	auto& effortPerLevel = internalSimulator_.get()->arbitraryLevelEffort;
	maxImportanceLvl = reachableImportanceValues.back();
	effortPerLevel = n_;

	// Check possible (importance) paths to the rare event, and keep "the best"
	if (highVerbosity)
		ModelSuite::tech_log("\nLooking for feasible paths to the rare event: ");
	auto idx = 0ul;
	do {
		internalSimulator_->fixed_effort(paths[idx], watch_events);
		if (paths.at(idx).empty() || paths.at(idx).front().size() < 2ul)
			continue;
		if (paths[idx].front().back().first < reachableImportanceValues.back()
				&& effortPerLevel < MAX_N) {
			ModelSuite::tech_log(highVerbosity ? ("-") : (""));
			effortPerLevel *= 2;
		} else {
			ModelSuite::tech_log(highVerbosity ? ("+") : (""));
			idx++;
		}
	} while (idx < NUM_PATHS);
	auto path = choose_best_path_to_rare(paths).front();
	/// @todo TODO: ^^^ hardcoded for SimulationEngineSFE: generalise!
	if (highVerbosity) {
		ModelSuite::tech_log("\nChosen path goes via the ImportanceValues:");
		for (const auto& p: path)
			ModelSuite::tech_log(" " + std::to_string(p.first));
	}

	// Set internals (of this class instance) to follow such "best path"
	ThresholdsVec().swap(currentThresholds_);
	currentThresholds_.reserve(path.size());
	double probLvlUp(1.0), minProbLvlUp(1.0);
	for (auto i = 0ul ; i < path.size() ; i++) {
		probLvlUp *= std::max(0.0001, probability_next_lvl_up(path, i));  // bound these (preliminary) effort values
		minProbLvlUp = std::min(minProbLvlUp, probLvlUp);
		const unsigned long effort(std::floor(1.0/probLvlUp));
		probLvlUp = 1.0 - (1.0/effort - probLvlUp);  // carry
		assert(0.0 <= probLvlUp && probLvlUp <= 1.0);
		currentThresholds_.emplace_back(path[i].first, effort);
	}
	std::vector<float>(path.size(), 0.0f).swap(Pup);
	maxImportanceLvl = path.back().first;
	effortPerLevel = std::ceil(1.0/minProbLvlUp);  // maximise effort: this the hardest level-up!
	for (const auto& impVal: reachableImportanceValues)  // try to reach all reachable ImportanceValues
		if (currentThresholds_.back().first < impVal)
			currentThresholds_.emplace_back(impVal, effortPerLevel);
	if (highVerbosity) {
		ModelSuite::tech_log("\nPreliminary effort for the reachable importance values:");
		for (const auto& p: currentThresholds_)
			ModelSuite::tech_log(" " + std::to_string(p.second));
	}

	// Run Fixed Effort a couple of times following the "best path"
	static constexpr auto NUM_FE_RUNS = 4ul;
	paths.reserve(NUM_FE_RUNS);
	ModelSuite::tech_log("\nRunning internal Fixed Effort [");
	for (auto i = 0ul ; i < NUM_FE_RUNS ; i++) {
		PathCandidate pathWrapper;
		internalSimulator_->fixed_effort(pathWrapper,watch_events);
		assert(!pathWrapper.empty());
		assert(!pathWrapper.front().empty());
		paths.emplace_back(pathWrapper);
		ModelSuite::tech_log(0.0 >= pathWrapper.front().back().second ? "-" : "+");
	}
	ModelSuite::tech_log("]");

	// Average the probabilities found by those Fixed Effort runs
	for (auto i = 0ul ; i < Pup.size() ; i++) {
		auto num(0u);
		auto acc(0.0);
		for (const auto& pathWrapper: paths) {
			if (pathWrapper.front().size() <= i)
				continue;
			const auto& thisPathProbability = pathWrapper.front()[i].second;
			if (thisPathProbability > 0.0) {
				num++;
				acc += thisPathProbability;
			}
		}
		Pup[i] = acc/static_cast<decltype(acc)>(num);
	}

	internalSimulator_->unbind();
	internalSimulator_->property_ = nullptr;
	return Pup;
}


void
ThresholdsBuilderES::artificial_thresholds_selection(
        ImportanceVec& reachableImportanceValues,
        std::vector< float >& Pup) const
{
	ModelSuite::tech_log("\nExpected Success failed");
	Pup.resize(reachableImportanceValues.size(), 0.0f);

	if (reachableImportanceValues.size() < 2ul) {
		ModelSuite::tech_log("; cannot even find reachable importance values\n");
		goto throw_fail_exception;
	} else if (0.0f >= Pup[0]) {
		ModelSuite::tech_log("; cannot even evaluate the first potential threshold\n");
		goto throw_fail_exception;
	} else if (0.0 >= Pup[1]) {
		ModelSuite::tech_log("; cannot even evaluate the second potential threshold\n");
		goto throw_fail_exception;
	} else {
		goto continue_artificial_selection;
	}
throw_fail_exception:
		throw_FigException("Expected Success failed to find thresholds");
continue_artificial_selection:
	bool print(true);
	ModelSuite::tech_log("!\nResorting to fixed choice of thresholds starting "
	                     "above the ImportanceValue ");
	/// @todo Implement regresive average to follow the trend of the probabilities
	for (size_t i = 2ul ; i < Pup.size() ; i++) {
		if (0.0f < Pup[i])
			continue;  // this value has already been chosen by Expected Success
		Pup[i] = linear_interpol<float>(reachableImportanceValues[i-2],
										reachableImportanceValues[i-1],
										Pup[i-2],
										Pup[i-1],
										reachableImportanceValues[i]);
//		const auto HI = std::max(Pup[i-1], Pup[i-2]),
//		           LO = std::min(Pup[i-1], Pup[i-2]);
//		const double trend = std::pow(LO,2.0)/HI;
//		Pup[i] = std::max(trend, 0.1);  // bound max effort
		if (print)
			ModelSuite::tech_log(std::to_string(reachableImportanceValues[i-1]));
		print = false;
	}
	// Failure recovered
	return;
	// Unrecoverable failure
}


void
ThresholdsBuilderES::tune(const size_t &,
                          const ImportanceValue &,
                          const unsigned&)
{
	assert(nullptr != model_);
	assert(nullptr != impFun_);

	// Factor [1], #(FE-sims) per iteration,
	// will be inversely proportional to the importance range
	// in the interval (3,20)
	// unless we have user specified info on the probability of importance lvl up
	if (ModelSuite::get_DFT() < 0.0) {
		// Just use the importance function range
		const auto PP_EXP(impFun_->post_processing().type == PostProcessing::EXP);
		const auto PP_EXP_BASE(PP_EXP ? impFun_->post_processing().value : 0.0f);
		const auto normalise = [&PP_EXP,&PP_EXP_BASE] (const double& x)
		    { return PP_EXP ? (log(x)/log(PP_EXP_BASE)) : x; };
		const size_t IMP_RANGE(normalise(impFun_->max_value(true))
                               - normalise(initial_importance(*impFun_)));
		constexpr auto MIN_IMP_RANGE =  3ul;
		constexpr auto MAX_IMP_RANGE = 20ul;
		nSims_ = IMP_RANGE < MIN_IMP_RANGE ? MAX_NSIMS :
		         IMP_RANGE > MAX_IMP_RANGE ? MIN_NSIMS :
		         std::round(linear_interpol<float>(MIN_IMP_RANGE, MAX_IMP_RANGE,
		                                           MIN_NSIMS, MAX_NSIMS,
		                                           IMP_RANGE));
		assert(nSims_ >= MIN_NSIMS);
		assert(nSims_ <= MAX_NSIMS);
	} else {
		// We're working on a DFT for which the user specified an explicit
		// probability of observing any failure before any repair,
		// i.e. the probability of increasing one level in importance
		const auto probImpLvlUp(ModelSuite::get_DFT());
		nSims_ = probImpLvlUp < 1e-20 ? MAX_NSIMS
		                              : static_cast<size_t>(1.0/probImpLvlUp);
	}

	// Factor [2], #(steps) per FE-sim,
	// will be inversely proportional to the model size,
	// understood as the #(clocks)+log(#(concrete_states)),
	// in the interval (1K,5K)
	const size_t MODEL_SIZE(model_->num_clocks()+model_->state_size());
	constexpr auto MIN_MODEL_SIZE = (1ul) << (6ul);
	constexpr auto MAX_MODEL_SIZE = (1ul) << (10ul);
	maxSimLen_ = MODEL_SIZE < MIN_MODEL_SIZE ? MAX_SIM_LEN :
	             MODEL_SIZE > MAX_MODEL_SIZE ? MIN_SIM_LEN :
	             std::round(linear_interpol<float>(MIN_MODEL_SIZE, MAX_MODEL_SIZE,
	                                               MIN_SIM_LEN, MAX_SIM_LEN,
	                                               MODEL_SIZE));
	assert(maxSimLen_ >= MIN_SIM_LEN);
	assert(maxSimLen_ <= MAX_SIM_LEN);
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
