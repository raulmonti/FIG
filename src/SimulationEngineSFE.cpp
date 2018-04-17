//==============================================================================
//
//  SimulationEngineSFE.cpp
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

// C++
#include <unordered_map>
#include <algorithm>   // std::fill(), std::max_element(), std::move()
#include <functional>  // std::bind()
// FIG
#include <SimulationEngineSFE.h>
#include <PropertyTransient.h>
#include <TraialPool.h>
#include <ModelSuite.h>

// ADL
using std::begin;
using std::end;


namespace   // // // // // // // // // // // // // // // // // // // // // // //
{

using fig::Traial;
using fig::ImportanceValue;
using fig::ImportanceFunction;
typedef std::vector< fig::Reference< Traial > > TraialVec;
typedef fig::SimulationEngine::ReachabilityCount ReachabilityCount;

/**
 * @brief Select the most likely "next step value" from several possibilities
 *
 *        The vector given as argument contains several Traial instances
 *        that could be the next step of a Fixed Effort sweep.
 *        Check with \p ifun which threshold/importance value occurs most often,
 *        and delete from the vector any Traial with different value.
 *
 * @param traials    Traials to analyse and filter
 * @param traialSink Container where discarded Traials will be placed
 * @param reachCount How often each importance/threshold was reached,
 *                   from among the values that occurr in \p traials
 * @param ifun       Importance function to assess the importance of each Traial
 * @param useImp     Use ImportanceValue rather than threshold-level to filter
 *
 * @return Next step value, viz. ImportanceValue of the chosen (surviving) Traials
 */
ImportanceValue
filter_next_value(TraialVec& traials,
                  TraialVec& traialSink,
                  const ReachabilityCount& reachCount,
                  const ImportanceFunction& ifun,
                  const bool useImportance)
{
    assert(!traials.empty());
	assert(!reachCount.empty());
	using pair_t = std::remove_reference<decltype(reachCount)>::type::value_type;
//	using levelof_t = ImportanceValue(ImportanceFunction::*)(const fig::StateInstance&) const;
//	using namespace std::placeholders;  // _1, _2, ...
//	auto ifun_level_of = useImportance
//	        ? std::bind(static_cast<levelof_t>(&ImportanceFunction::importance_of), &ifun, _1)
//	        : std::bind(static_cast<levelof_t>(&ImportanceFunction::level_of),      &ifun, _1);
	TraialVec chosenTraials;
    const auto N(traials.size());
    chosenTraials.reserve(N);
	// Find most frequent ImportanceValue
	auto nextStep = std::max_element(begin(reachCount), end(reachCount),
	                                 [](const pair_t& p1, const pair_t& p2)
	                                 { return p1.second < p2.second; });
	const auto nextValue(nextStep->first);
//	const size_t nextValueIndex(nextStep->second);
	// Filter the traials vectors
    for (auto i = 0ul ; i < N ; i++) {
        Traial& t(traials.back());
        traials.pop_back();
//		if (ifun_level_of(t.state) == nextValue)
		if (t.level == nextValue)
			chosenTraials.push_back(t);
        else
			traialSink.push_back(t);
    }
	assert(traials.empty());
	assert(!chosenTraials.empty());
	assert(chosenTraials.size() <= N);
	chosenTraials.swap(traials);
	return nextValue;
}

} // namespace   // // // // // // // // // // // // // // // // // // // // //


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

SimulationEngineSFE::SimulationEngineSFE(
    std::shared_ptr<const ModuleNetwork> model,
    const bool thresholds) :
        SimulationEngineFixedEffort("sfe", model, thresholds)
{ /* Not much to do around here */ }


SimulationEngineSFE::~SimulationEngineSFE()
{
    //TraialPool::get_instance().return_traials(traials_);
    // ^^^ pointless, and besides the TraialPool might be dead already,
    //     so this would trigger a re-creation of the pool or something worse
}


const SimulationEngineFixedEffort::EventWatcher&
SimulationEngineSFE::get_event_watcher(const Property& property) const
{
	using namespace std::placeholders;  // _1, _2, ...
//	static std::unordered_map< PropertyType,
//							   Reference< EventWatcher >
//							 > event_watchers;
//	if (event_watchers.empty()) {
//		event_watchers.emplace(PropertyType::TRANSIENT,
//							   std::bind(&SimulationEngineSFE::transient_event, this, _1, _2, _3));
//		event_watchers.emplace(PropertyType::RATE,
//							   std::bind(&SimulationEngineSFE::rate_event, this, _1, _2, _3));
//	}
//	if (property.type != PropertyType::TRANSIENT &&
//		property.type != PropertyType::RATE)
//		throw_FigException("unsupported property type: "+std::to_string(property.type));
//	else
//		return event_watchers[property.type].get();
	if (property.type == PropertyType::TRANSIENT) {
		static const EventWatcher& transient_event_watcher(
					std::bind(&SimulationEngineSFE::transient_event, this, _1, _2, _3));
		return transient_event_watcher;
	} else if (property.type == PropertyType::RATE) {
		static const EventWatcher& rate_event_watcher(
					std::bind(&SimulationEngineSFE::rate_event, this, _1, _2, _3));
		return rate_event_watcher;
//		return std::bind(&SimulationEngineSFE::rate_event, this, _1, _2, _3);
	} else {
		throw_FigException("unsupported property type: "+std::to_string(property.type));
	}
}


void
SimulationEngineSFE::fixed_effort(ThresholdsPathCandidates& result,
                                  const EventWatcher& watch_events) const
{
	assert(nullptr != impFun_);

	auto lvl_effort = [&](const unsigned& effort){ return effort*base_nsims(); };
	const size_t LVL_MAX(impFun_->max_value(toBuildThresholds_)),
	             LVL_INI(impFun_->initial_value(toBuildThresholds_)),
				 EFF_MAX(std::max(std::max(
							lvl_effort_min(),
							arbitraryLevelEffort),
							lvl_effort(impFun_->max_thresholds_effort(toBuildThresholds_))));
	decltype(reachCount_) reachCountLocal(reachCount_);
	std::vector< Reference< Traial > > traialsNow, traialsNext;

	// Init result & internal ADTs
	ThresholdsPathCandidates(1).swap(result);
	//assert(result.size() == 1);
	auto& pathToRare(result.front());
	//assert(&(*result.begin()) == &pathToRare);
	pathToRare.reserve(LVL_MAX);
	traialsNow.reserve(EFF_MAX);
	traialsNext.reserve(EFF_MAX);
	if (traials_.size() < EFF_MAX)
		TraialPool::get_instance().get_traials(traials_, EFF_MAX-traials_.size());

	// Bootstrap the Fixed Effort run
    traials_.back().get().initialise(*model_, *impFun_);
	traialsNext.push_back(std::move(traials_.back()));
    traials_.pop_back();
	size_t numSuccesses;
	ImportanceValue l(LVL_INI);

	// Run Fixed Effort: For each threshold level 'l' ...
	do {
        // ... prepare the Traials to run the simulations ...
        numSuccesses = 0ul;
		const size_t LVL_EFFORT =
				std::max(lvl_effort_min(), toBuildThresholds_ ? arbitraryLevelEffort
															  : lvl_effort(impFun_->effort_of(l)));

		/// @todo TODO erase debug print
		static int kkk = 99;
		if (0 < kkk--)
			ModelSuite::debug_log("{"+std::to_string(LVL_EFFORT)+"}");


		assert(0ul < LVL_EFFORT);
        assert(traialsNow.empty());
        assert(!traialsNext.empty());
        for (auto i = 0ul ; i < LVL_EFFORT ; i++) {
            const bool useFresh(i >= traialsNext.size());
            Traial& traial(useFresh ? traials_.back() : traialsNext[i]);
            if (useFresh) {
                traials_.pop_back();
                traial = traialsNext[i%traialsNext.size()].get();  // copy *contents*
            }
            assert(traial.level == l);
            traial.depth = 0;
            traialsNow.push_back(traial);
        }
		assert(traialsNow.size() == LVL_EFFORT);
		traialsNext.erase(begin(traialsNext),
		                  begin(traialsNext)+std::min(traialsNext.size(),LVL_EFFORT));
		std::move(begin(traialsNext), end(traialsNext), std::back_inserter(traials_));
		traialsNext.clear();
		reachCountLocal.clear();
		// ... run Fixed Effort until any level > 'l' ...
        for (auto i = 0ul ; i < LVL_EFFORT ; i++) {
            Traial& traial(traialsNow.back());
			traialsNow.pop_back();
			assert(traial.level < LVL_MAX);
			model_->simulation_step(traial, *property_, watch_events);
			if (traial.level > l || property_->is_rare(traial.state))
				numSuccesses++;
			if (traial.level > l) {
				traialsNext.push_back(traial);
				reachCount_[traial.level]++;
				reachCountLocal[traial.level]++;
			} else {
				traials_.push_back(traial);
			}
        }
        // ... and interpret the results
		pathToRare.emplace_back(l, static_cast<double>(numSuccesses)/LVL_EFFORT);
		/// @todo TODO revise whether 'l' (or 'nextLvl'? see below)
		///            should be emplaced_back in 'pathToRare'

		if (!traialsNext.empty()) {

			auto nextLvl = filter_next_value(traialsNext,
											 traials_,
											 reachCountLocal,
											 *impFun_,
											 toBuildThresholds_);
			assert(l < nextLvl);
			assert(nextLvl <= LVL_MAX);


			/// @todo TODO erase debug print
			static int lll = 999;
			if (0 < lll--)
				ModelSuite::debug_log("."+std::to_string(l)+
									  "->"+std::to_string(nextLvl)+
									  "("+std::to_string(pathToRare.back().second)+").");

			l = nextLvl;
		}
	} while (l < LVL_MAX && !traialsNext.empty());
	// If we didn't reach the rare event, last probability must be 0.0
	assert(l >= LVL_MAX || 0.0 >= pathToRare.back().second);

	/// @todo TODO erase debug print
	static bool doPrint(true);
	if (doPrint && l >= LVL_MAX && !toBuildThresholds_) {
		doPrint = false;
		ModelSuite::debug_log("\nSuccessful path: ");
		for (auto p: pathToRare)
			ModelSuite::debug_log("("+std::to_string(p.first)+
								  ":"+std::to_string(p.second)+") ");
		ModelSuite::debug_log("\n");
	}

//	assert(!pathToRare.empty());
//	assert(result.front().size() == pathToRare.size());
//	// For each threshold level 'l' ...
//	ImportanceValue l;
//	size_t numSuccesses(1ul);
//	for (l = LVL_INI ; l < LVL_MAX && 0ul < traialsNext.size() ; l++) {
//		// ... prepare the Traials to run the simulations ...
//		numSuccesses = 0ul;
//		const auto LVL_EFFORT(lvl_effort(impFun_->effort_of(l)));
//		assert(0 < LVL_EFFORT);
//		assert(traialsNow.empty());
//		assert(!traialsNext.empty());
//		for (auto j = 0ul ; j < LVL_EFFORT ; j++) {
//			const bool useFresh(j >= traialsNext.size());
//			Traial& traial(useFresh ? traials_.back() : traialsNext[j]);
//			if (useFresh) {
//				traials_.pop_back();
//				traial = traialsNext[j%traialsNext.size()].get();  // copy *contents*
//			}
//			assert(traial.level == l);
//			traial.depth = 0;
//			traialsNow.push_back(traial);
//		}
//		assert(traialsNow.size() == LVL_EFFORT);
//		traialsNext.erase(begin(traialsNext),
//						  begin(traialsNext)+std::min(traialsNext.size(),LVL_EFFORT));
//		std::move(begin(traialsNext), end(traialsNext), std::back_inserter(traials_));
//		traialsNext.clear();
//		// ... run Fixed Effort until the next level 'l+1' ...
//		do {
//			Traial& traial(traialsNow.back());
//			traialsNow.pop_back();
//			assert(traial.level < LVL_MAX);
//			reachCount_[traial.level]++;
//			network.simulation_step(traial, property, *this, event_watcher);
//			if (traial.level > l) {
//				numSuccesses++;
//				traialsNext.push_back(traial);
//			} else {
//				if (property.is_rare(traial.state))
//					numSuccesses++;
//				traials_.push_back(traial);
//			}
//		} while (!traialsNow.empty());
//		// ... and estimate the probability of reaching 'l+1' from 'l'
//		Pup[l] = static_cast<double>(numSuccesses) / LVL_EFFORT;
//	}
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
