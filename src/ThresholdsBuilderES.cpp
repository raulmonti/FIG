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
using PathContent = PathCandidate::value_type;
template<typename T> std::string str(const T& val) { return std::to_string(val);  }
template<typename T> std::string str(const T* val) { return std::to_string(*val); }
template std::string str(const int&);
template std::string str(const long&);
template std::string str(const unsigned&);
template std::string str(const float&);
template std::string str(const double&);
template std::string str(const char*);


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
	fig::figTechLog << std::endl;
	fig::figTechLog.flags(defaultLogFlags);
}


/// Retrieve initial importance of the ImportanceFunction instance,
/// taking care to ask for <i>importance</i> and not for <i>threshold-level</i>.
fig::ImportanceValue
initial_importance(const fig::ImportanceFunction& ifun)
{
	return ifun.initial_value(true);
}


/// Retrieve max importance of the ImportanceFunction instance,
/// taking care to ask for <i>importance</i> and not for <i>threshold-level</i>.
fig::ImportanceValue
max_importance(const fig::ImportanceFunction& ifun)
{
	return ifun.max_value(true);
}


/// Return the element from \p paths that reached the highest ImportanceValue;
/// if there are N>1 such paths then average their probability values.
PathContent
choose_best_path_to_rare(const std::vector< PathCandidate >& paths)
{
	assert(!paths.empty());
	using Importance = fig::ImportanceValue;
	auto importance_of_at = [] (const PathCandidate& path, const size_t i) -> Importance
		{
			return path.front().at(i).first;
		};
	auto top_importance = [] (const PathCandidate& path) -> Importance
		{
			return path.front().back().first;
		};
	auto path_probability = [] (const PathContent& path) -> double
		{
			double pathProbability = 1.0;
			for (const auto& p: path)
				pathProbability *= p.second > 0.0 ? p.second : 1.0;
			assert(0.0 < pathProbability);
			assert(1.0 > pathProbability);
			return pathProbability;
		};

	PathCandidate bestPath(paths.front());
	Importance bestPathImportance(top_importance(bestPath));
	double bestPathProbability(path_probability(bestPath.front()));

	for (const auto& path: paths) {
		const auto thisPathImportance  = top_importance(path);
		const auto thisPathProbability = path_probability(path.front());
		if (thisPathImportance > bestPathImportance ||
				(thisPathImportance == bestPathImportance &&
				 thisPathProbability > bestPathProbability)) {
			bestPath = path;
			bestPathImportance = thisPathImportance;
			bestPathProbability = thisPathProbability;
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
        discard_this_path: ;
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
	assert (!result.empty());
	assert (!resultContent.empty());
	return resultContent;
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
	ThresholdsBuilderAdaptive(static_cast<unsigned>(n)),
    nSims_(0ul),
    maxSimLen_(0ul),
	maxImportanceReached_(0ul),
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
	n_ = static_cast<unsigned>(nSims_);
	ModelSuite::tech_log("Building thresholds with \"" + name
						+ "\" for num_sims,sim_len = " + str(n_)
						+ "," + str(maxSimLen_));

	// Probe for reachable importance values to select threshold candidates
	const bool forceRealMax(0.0 <= ModelSuite::get_DFT());
	ImportanceVec thrCandidates(reachable_importance_values(forceRealMax));
	if (thrCandidates.size() < 2ul)  // we must have reached beyond initial importance
		throw_FigException("ES could not find reachable importance values");
	if (highVerbosity) {
		ModelSuite::tech_log("\nFound " + str(thrCandidates.size())
							+ " reachable importance values:");
		for (auto imp: thrCandidates)
			ModelSuite::tech_log(" " + str(imp));
	}

	// Estimate probabilities of going from one (reachable) importance value to the next
	auto Pup = FE_for_ES(thrCandidates);
	assert(!currentThresholds_.empty());  // could we build something?
	assert(!Pup.empty());
	process_artificial_thresholds(Pup);
	if (highVerbosity)
		print_lvlup(Pup);

	// SimualtionEngineRestart can't split on initial! Patch if this is the case:
	const auto IMP_INI = initial_importance(*impFun_);
	if (IMP_INI == currentThresholds_.front().first) {
		assert(Pup.size() > 1ul);
		Pup[1] *= Pup[0];
		Pup[0] = 1.0f;
	}

	// Turn level-up probabilities into effort factors
	auto effort(Pup);
	std::fill(begin(effort), end(effort), static_cast<decltype(effort)::value_type>(0));
	for (auto i = 0ul ; i < Pup.size() ; i++) {
		const auto prev = effort[(i-1)%Pup.size()];
		effort[i] = 1.0f/Pup[i] + (prev-std::round(prev));  // E [# sims to move up from lvl i]
	}

	// Select the thresholds based on the effort factors
	decltype(thresholds_)().swap(thresholds_);
	thresholds_.reserve(effort.size()+2ul);
	thresholds_.emplace_back(IMP_INI, 1ul);  // formatting of thresholds_
	for (auto i = 0ul ; i < Pup.size() ; i++) {
		const auto thisEffort = static_cast<size_t>(std::round(effort[i]));
		assert(currentThresholds_[i].first > IMP_INI || thisEffort <= 1ul);
		if (thisEffort > 1ul)
			thresholds_.emplace_back(currentThresholds_[i].first, thisEffort);
	}
	if (max_importance(*impFun_) == thresholds_.back().first)
		thresholds_.back().second = 1ul;
	thresholds_.emplace_back(max_importance(*impFun_)+1u, 1ul);  // formatting of thresholds_

	ModelSuite::tech_log("\n");
	show_thresholds(thresholds_);
	impFun_.reset();
	model_.reset();
	return thresholds_;
}


ImportanceVec
ThresholdsBuilderES::reachable_importance_values(bool forceRealMax)
{
	using namespace std::placeholders;  // _1, _2, ...
	const EventWatcher& watch_events =
			std::bind(&ThresholdsBuilderES::importance_seeker, this, _1, _2, _3);

	assert(nullptr != impFun_);
	assert(nullptr != property_);

	static constexpr size_t NUM_INDEPENDENT_RUNS = 20ul;
	static constexpr size_t MAX_FAILS = (1ul)<<(10ul);
	const auto MAX_IMPORTANCE = max_importance(*impFun_);
	size_t numFails(0ul);

	maxImportanceReached_ = initial_importance(*impFun_);
	std::unordered_set< ImportanceValue > reachableImpValues = { maxImportanceReached_ };
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
				maxImportanceReached_ = std::max(maxImportanceReached_, traial.level);
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
	} while (maxImportanceReached_ < MAX_IMPORTANCE);
    end_reachability_analysis:
	    TraialPool::get_instance().return_traials(traialsNow);
		TraialPool::get_instance().return_traials(traialsNext);

	ImportanceVec result(begin(reachableImpValues), end(reachableImpValues));
	std::sort(begin(result), end(result));
	assert(result.back() == maxImportanceReached_);

	// Some models require ES to reach the (real) max importance value
	if (forceRealMax) {
		const auto maxImportance = max_importance(*impFun_);
		assert(maxImportance >= maxImportanceReached_);
		if (maxImportance > maxImportanceReached_) {
			if (highVerbosity)
				ModelSuite::tech_log("\nReached importance value " + str(maxImportanceReached_) +
									 "; forcing to reach value " + str(maxImportance));
			std::set< ImportanceValue > greaterImportanceValues;
			// fill gap between maxImportanceReached_ and maxImportance (sure?)
/*			auto randomImportanceValues = impFun_->random_sample(model_->initial_state(), 50);
			std::copy_if (begin(randomImportanceValues), end(randomImportanceValues),
						  std::inserter(greaterImportanceValues,end(greaterImportanceValues)),
						  [&] (const ImportanceValue& imp) { return imp > maxImportanceReached_; });
*/			greaterImportanceValues.emplace(maxImportance);
			result.reserve(result.size()+greaterImportanceValues.size());
			for (const auto& imp: greaterImportanceValues) {
				assert(imp > result.back());
				result.emplace_back(imp);
			}
		}
	}

	return result;
}


/**
 * @details This routine performs three distinct tasks/steps:
 *          <ol>
 *          <li>find <i>real</i> importance paths to the rare event;
 *          <li>choose <i>the best</i> path found and settle for that one only;
 *          <li>compute and refine the conditional probabilities of performing
 *              the level-ups in such best path.
 *          </ol>
 *          For the <b>first task</b>, "unguided" FE simulations are run,
 *          where the \p reachableImportanceValues define the threshold-levels,
 *          and the same effort is used for every level.<br>
 *          For the <b>second task</b>, the path (from the first step) that
 *          reached the highest importance with highest probability is selected.<br>
 *          For the <b>third task</b> heavier FE simulations are run, using the
 *          probability values computed in the first step to select "preliminary
 *          effort values" for each threshold-level, and making all runs follow
 *          the importance values of the best path selected in step two.
 * @todo Improve modularisation
 */
std::vector< float >
ThresholdsBuilderES::FE_for_ES(const ImportanceVec& reachableImportanceValues)
{
	static constexpr auto NUM_PATHS = 3ul;
	using namespace std::placeholders;  // _1, _2, ...
	const EventWatcher& watch_events =
			std::bind(&ThresholdsBuilderES::FE_watcher, this, _1, _2, _3);

	if (reachableImportanceValues.size() < 2ul)
		return std::vector<float>();  // only one reachable importance value: ES failed!

	//\///////////////////////////////////////////////////////////////////
	// Step #1: find real importance paths to the rare event

	// Use reachableImportanceValues as candidates to paths to the rare event
	std::vector<PathCandidate> paths(NUM_PATHS);
	decltype(currentThresholds_)().swap(currentThresholds_);
	currentThresholds_.reserve(reachableImportanceValues.size());
	for (auto i = 0ul ; i < reachableImportanceValues.size() ; i++)
		currentThresholds_.emplace_back(reachableImportanceValues[i], 0ul);
	assert(nullptr != property_);
	internalSimulator_->property_ = property_.get();
	internalSimulator_->bind(impFun_);
	auto& maxImportanceLvl = internalSimulator_.get()->arbitraryMaxLevel;
	auto& level_effort = internalSimulator_.get()->arbitrary_effort;
	maxImportanceLvl = reachableImportanceValues.back();
	level_effort = [&](const unsigned&) -> unsigned long { return n_; };

	// Check possible (importance) paths to the rare event
	if (highVerbosity)
		ModelSuite::tech_log("\nLooking for feasible paths to the rare event: ");
	auto idx = 0ul;
	do {
		internalSimulator_->fixed_effort(paths[idx], watch_events);
		const auto& path(paths[idx]);
		if (path.empty() || path.front().size() < 2ul)
			continue;
		if (path.front().back().first < reachableImportanceValues.back() && n_ < MAX_N) {
			ModelSuite::tech_log(highVerbosity ? ("*") : (""));
			n_ *= 2;
		} else {
			auto updatePrint = !highVerbosity ? ("") :
				path.front().back().first < reachableImportanceValues.back() ? ("-") : ("+");
			ModelSuite::tech_log(updatePrint);
			idx++;
		}
	} while (idx < NUM_PATHS);

	//\///////////////////////////////////////////////////////////////////
	// Step #2: choose the best path from step #1 and settle for it alone

	auto bestPath = choose_best_path_to_rare(paths);
	/// @todo TODO: ^^^ hardcoded for SimulationEngineSFE: generalise!
	assert(bestPath.size() <= reachableImportanceValues.size());
	if (highVerbosity) {
		ModelSuite::tech_log("\nChosen path visits "+str(bestPath.size())+" ImportanceValues:");
		for (const auto& p: bestPath)
			ModelSuite::tech_log(" " + str(p.first));
	}

	// Set internals of this class instance to follow bestPath
	decltype(currentThresholds_)().swap(currentThresholds_);
	static constexpr auto MIN_PROB(0.005);
	static constexpr auto MIN_EFFORT(1l<<7l);
	std::vector< double > prelimPup;
	long maxEffort(MIN_EFFORT);
	double probLvlUp(1.0);
	for (auto i = 0ul ; i < bestPath.size() ; i++) {
		// up-and-low-bound these (preliminary) effort values
		prelimPup.emplace_back(probability_next_lvl_up(bestPath, i));
		probLvlUp *= std::max(MIN_PROB, prelimPup.back());
		const long effort = static_cast<long>(std::floor(1.0/probLvlUp));
		probLvlUp = 1.0 - (1.0/effort - probLvlUp);  // carry
		assert(0.0 <= probLvlUp && probLvlUp <= 1.0);
		currentThresholds_.emplace_back(bestPath[i].first, std::max(MIN_EFFORT, effort));
		maxEffort = std::max(maxEffort, effort);
	}

	// Maximise effort for (yet) unreached ImportanceValues
	currentThresholds_.reserve(reachableImportanceValues.size());
	for (const auto& impVal: reachableImportanceValues)
		if (currentThresholds_.back().first < impVal)
			currentThresholds_.emplace_back(impVal, maxEffort);
	assert(currentThresholds_.size() >= prelimPup.size());
	prelimPup.resize(currentThresholds_.size(), 0.0);

	// Set internals of our SimulationEngine to use these preliminary effort values
	maxImportanceLvl = currentThresholds_.back().first;
	level_effort = [&](const unsigned& level) -> unsigned long
		{
			auto it = std::find_if(begin(currentThresholds_), end(currentThresholds_),
								   [&level](const Threshold& t) { return t.first >= level;});
			// return the effort for the (known) level reached,
			// or 0 if an ImportanceValue outside the "best bestPath" is observed
			if (end(currentThresholds_) == it || (*it).first != level)
				return 0ul;           // deviates from best bestPath: abort simulation
			else
				return (*it).second;  // known level: return this effort
		};

	if (highVerbosity) {
		assert(currentThresholds_.size() == prelimPup.size());
		ModelSuite::tech_log("\nPreliminary results for reachable importance "
							 "values: (value, lvl-up-prob, effort) = ");
		for (auto i = 0ul ; i < currentThresholds_.size() ; i++ )
			ModelSuite::tech_log(" (" + str(currentThresholds_[i].first)
								+ "," + str(prelimPup[i])
								+ "," + str(currentThresholds_[i].second) + ")");
	}

	//\///////////////////////////////////////////////////////////////////
	// Step #3: refine the probabilities of level-up in the best path

	// Run Fixed Effort a couple of times following the "best bestPath"
	static constexpr auto NUM_SAMPLES_FROM_BEST_PATH = NUM_PATHS;
	decltype(paths)().swap(paths);
	paths.reserve(NUM_SAMPLES_FROM_BEST_PATH);
	auto maxSteps = bestPath.size();
	ModelSuite::tech_log("\nRunning internal Fixed Effort [");
	do {
		PathCandidate pathWrapper;
		internalSimulator_->fixed_effort(pathWrapper, watch_events);
		// We discard runs that deviate form bestPath...
		if (pathWrapper.empty())
			continue;
		const auto& newPath(pathWrapper.front());
		// ...or that died too early
		if (newPath.size() < 2ul)
			continue;
		ModelSuite::tech_log(0.0 >= newPath.back().second ? "-" : "+");
		maxSteps = std::max(maxSteps, newPath.size());
		paths.emplace_back(std::move(pathWrapper));
	} while (paths.size() < NUM_SAMPLES_FROM_BEST_PATH);
	paths.emplace_back(PathCandidate(1,std::move(bestPath)));
	ModelSuite::tech_log("]");

	// Average the probabilities found by those Fixed Effort runs
	std::vector< float > Pup;
	for (auto i = 0ul ; i < maxSteps ; i++) {
		auto num(0u);
		auto acc(0.0f);
		for (const auto& pathWrapper: paths) {
			const auto& path(pathWrapper.front());
			if (path.size() <= i)
				continue;
			const auto& pathProbability = path[i].second;
			if (std::isnormal(pathProbability)) {  // avoid NaN, inf, 0.0
				num++;
				acc += pathProbability;
			} else if (pathProbability == 0.0 && 0ul < i && 0.0 < path[i-1].second) {
				num++;  // simulation died in this step => 0.0 prob of lvl up
			}
		}
		Pup.emplace_back(0u < num ? acc/static_cast<decltype(acc)>(num) : 0.0f);
	}

	internalSimulator_->unbind();
	internalSimulator_->property_ = nullptr;
	return Pup;
}


void
ThresholdsBuilderES::process_artificial_thresholds(std::vector< float >& Pup) const
{
	assert(!Pup.empty());
	ModelSuite::tech_log(std::string("\nExpected Success ") +
						 (Pup.back() <= 0.0f ? ("couldn't reach the rare event")
											 : ("finished successfully")));
	if (currentThresholds_.size() < 2ul) {
		ModelSuite::tech_log("; cannot even find reachable importance values\n");
		goto throw_fail_exception;
	} else if (0.0f >= Pup[0]) {
		ModelSuite::tech_log("; cannot even evaluate the first potential threshold\n");
		goto throw_fail_exception;
	} else if (0.0f >= Pup[1]) {
		ModelSuite::tech_log("; cannot even evaluate the second potential threshold\n");
		goto throw_fail_exception;
	} else {
		goto continue_artificial_selection;
	}
throw_fail_exception:
		throw_FigException("Expected Success failed to find thresholds");

continue_artificial_selection:
	// Delete artificial thresholds between the last reached ImportanceValue
	// and the (real) max ImportanceValue
	assert(currentThresholds_.size() >= Pup.size());
	Pup.resize(currentThresholds_.size(), 0.0f);
	const auto iterFirstFake = std::find(begin(Pup), end(Pup), 0.0f);
	const auto numReachedValues = std::distance(begin(Pup), iterFirstFake) - 1ul;
	assert(numReachedValues > 1);
	const auto& lastPup = Pup.at(numReachedValues);
	assert(lastPup > 0.0f);
	const auto MIN_PROB(lastPup/2.0f);  // bound max effort

	if (highVerbosity && Pup.back() <= 0.0f)
		ModelSuite::tech_log("!\nArtificial thresholds will be set above the ImportanceValue "
							 + str(currentThresholds_[numReachedValues].first));

	/// @todo Implement regresive average to follow the trend of all probabilities
	for (auto i = numReachedValues+1 ; i < Pup.size() ; i++) {
		assert(0.0f == Pup[i]);
		assert(0.0f < Pup[i-1]);
		assert(0.0f < Pup[i-2]);
		const auto HI = std::max(Pup[i-1], Pup[i-2]),
				   LO = std::min(Pup[i-1], Pup[i-2]),
				   TREND = LO*LO/HI;
		Pup[i] = std::max(MIN_PROB, TREND);  // bound max effort
	}
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
