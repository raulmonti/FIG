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


namespace   // // // // // // // // // // // // // // // // // // // // // // //
{

/// DEBUG mode: Show in tech log the probability of level-up for each level
/// RELEASE mode: NOP
void
debug_print_lvlup(const std::vector< float >& Pup)
{
#ifndef NDEBUG
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
#endif
}


/// Retrieve initial importance of the ImportanceFunction instance,
/// taking care to ask for <i>importance</i> and not for <i>threshold-level</i>.
fig::ImportanceValue
initial_importance(const fig::ImportanceFunction& ifun)
{
    return ifun.initial_value(true);
}

//
//	/// Wrap a vector of reachable importance values as a thresholds vector ADT
//	fig::ThresholdsVec
//	wrap_as_thresholds(const fig::ImportanceVec& impVec)
//	{
//		constexpr unsigned dummyEffort(1u);
//		fig::ThresholdsVec result;
//		result.reserve(impVec.size());
//		std::fill_n()
//		        std::for_each
//		for (const auto& imp: impVec)
//			result.emplace_back(imp);
//	}

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
	ImportanceVec thrCandidates(reachable_importance_values());
	if (thrCandidates.size() < 2ul)  // we must have reached beyond initial importance
		throw_FigException("ES could not find reachable importance values");
	ModelSuite::debug_log("\nFound " + std::to_string(thrCandidates.size()-1ul)
	                    + " relevant importance values:");
	for (auto imp: thrCandidates)
		ModelSuite::debug_log(" "+std::to_string(imp));

	// Estimate probabilities of going from one (reachable) importance value to the next
	ModelSuite::debug_log("\nRunning internal Fixed Effort");
	auto Pup = FE_for_ES(thrCandidates);
	assert(!currentThresholds_.empty());  // could we build something?
	assert(!Pup.empty());
	if (0.0f >= Pup.back())
		artificial_thresholds_selection(thrCandidates, Pup);
	debug_print_lvlup(Pup);  // print debug info in tech log

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

	/// @todo if these thresholds_ would produce too much splitting,
	///       e.g. thresholds_[i..i+5] = (imp,imp+1,...,imp+4) for effort ~ 10,
	///       pick a threshold, remove it, and increase proportionally
	///       the effort in the thresholds directly below and above it,
	///       e.g. thresholds_[i..i+3] = (imp,imp+2,imp+4) for effort ~ 15.

	ModelSuite::tech_log("\n");
	show_thresholds(thresholds_);
	impFun_.reset();
	model_.reset();
	return thresholds_;
}


ImportanceVec
ThresholdsBuilderES::reachable_importance_values() const
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
	return result;
}


std::vector< float >
ThresholdsBuilderES::FE_for_ES(const ImportanceVec& reachableImportanceValues) const
{
	std::vector< float > Pup({0.0});
	using namespace std::placeholders;  // _1, _2, ...
	const EventWatcher& watch_events =
			std::bind(&ThresholdsBuilderES::FE_watcher, this, _1, _2, _3);

	if (reachableImportanceValues.size() < 2ul)
		return Pup;  // only one reachable importance value: ES failed!

	SimulationEngineFixedEffort::ThresholdsPathCandidates paths;
	ThresholdsVec().swap(currentThresholds_);
	currentThresholds_.reserve(reachableImportanceValues.size());
	for (size_t i = 0ul ; i < reachableImportanceValues.size() ; i++)
		currentThresholds_.emplace_back(reachableImportanceValues[i], 1u);
	internalSimulator_->property_ = property_.get();
	internalSimulator_->bind(impFun_);
	auto& maxImportanceLvl = internalSimulator_.get()->arbitraryMaxLevel;
	auto& effortPerLevel = internalSimulator_.get()->arbitraryLevelEffort;
	maxImportanceLvl = reachableImportanceValues.back();
	effortPerLevel = n_;

    // Run Fixed Effort a couple of times
	ModelSuite::tech_log(" [");
	bool rarePathWasSet(false);
	for (size_t m = 1ul ; m < 4ul ; m++) {
		constexpr int NUM_ATTEMPTS(1<<3);
		int numAttemptsLeft(NUM_ATTEMPTS);
		do {
			const unsigned FAIL_BOOST =
				std::pow(rarePathWasSet ? 1 : NUM_ATTEMPTS-numAttemptsLeft+1, 1.3);
			effortPerLevel = std::max(effortPerLevel, n_*FAIL_BOOST);
			internalSimulator_->fixed_effort(paths, watch_events);
		} while (0.0 >= paths.front().back().second && 0 < --numAttemptsLeft);

		const auto& path(paths.front());
		ModelSuite::tech_log(0.0 >= path.back().second ? "-" : "+");
		if (0.0 >= path.back().second) {
			continue;  // no path to the rare event found yet, try again
		} else if (!rarePathWasSet) {
			//\////////////////////////////////////////////////////////////////
			// First time a path to the rare event is found: Save as *the* path
			ThresholdsVec().swap(currentThresholds_);
			currentThresholds_.reserve(path.size());
			double probLvlUp(1.0), minProbLvlUp(1.0);
			for (const auto& p: path) {
				probLvlUp *= p.second;
				minProbLvlUp = std::min(minProbLvlUp, probLvlUp);
				const unsigned long effort(std::floor(1.0/probLvlUp));
				probLvlUp = 1.0 - (1.0/effort - probLvlUp);  // carry
				assert(0.0 <= probLvlUp && probLvlUp <= 1.0);
				currentThresholds_.emplace_back(p.first, effort);
			}
			std::vector<float>(path.size(), 0.0f).swap(Pup);
			maxImportanceLvl = path.back().first;
			effortPerLevel = std::ceil(1.0/minProbLvlUp);  // maximise effort (this the hardest level-up!)
			rarePathWasSet = true;
			// TODO: ^^^ heuristic settles on *first* path chosen, a bit risky
			// TODO: ^^^ heuristic hardcoded for SimulationEngineSFE: Generalise!
			//\////////////////////////////////////////////////////////////////
		}
		assert(path.size() <= Pup.size());
		for (size_t i = 0ul ; i < path.size() && 0.0 < path[i].second ; i++)
			Pup[i] += (path[i].second - Pup[i]) / m;
	}
	ModelSuite::tech_log("]");
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
	Pup.resize(reachableImportanceValues.size());

	if (reachableImportanceValues.size() < 2ul) {
		// There's *nothing*, try desperately to build some threshold
		auto moreValues = impFun_->random_sample(model_->initial_state());
		reachableImportanceValues.insert(end(reachableImportanceValues),
		                                 begin(moreValues), end(moreValues));
		std::sort(begin(reachableImportanceValues), end(reachableImportanceValues));
		std::unique(begin(reachableImportanceValues), end(reachableImportanceValues));
		if (reachableImportanceValues.size() < 2ul) {
			ModelSuite::tech_log("; cannot find *any* threshold!");
			return;
		}
		Pup.resize(reachableImportanceValues.size());
		ModelSuite::tech_log(" utterly");
	}
	if (0.0f >= Pup[0]) {
		Pup[0] = 5.0e-2;
		Pup[1] = 5.0e-2;
		ModelSuite::tech_log(" on the first iteration");
	} else if (0.0 >= Pup[1]) {
		Pup[1] = Pup[0]/10.0f;
		ModelSuite::tech_log(" on the second iteration");
	}
	ModelSuite::tech_log("!\nResorting to fixed choice of thresholds starting "
	                     "above the ImportanceValue ");
	/// @todo Implement regresive average to follow the trend of the probabilities
	bool print(true);
	for (size_t i = 2ul ; i < Pup.size() ; i++) {
		if (0.0 < Pup[i])
			continue;  // this value was chosen by Expected Success
		const auto HI = std::max(Pup[i-1], Pup[i-2]),
		           LO = std::min(Pup[i-1], Pup[i-2]);
		const double trend = std::pow(LO,2.0)/HI;
		Pup[i] = std::max(trend, 0.1);  // bound max effort
		if (print)
			ModelSuite::tech_log(std::to_string(reachableImportanceValues[i-1]));
		print = false;
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
