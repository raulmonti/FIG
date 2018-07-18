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
 *        Check which threshold/importance value occurs most often,
 *        and delete from the vector any Traial with different value.
 *
 * @param traials    Traials to analyse and filter
 * @param reachCount How often each importance/threshold was reached,
 *                   from among the values that occurr in \p traials
 *
 * @return Next step value, viz. ImportanceValue of the chosen (surviving) Traials
 */
ImportanceValue
filter_next_value(TraialVec& traials,
				  const ReachabilityCount& reachCount)
{
    assert(!traials.empty());
	assert(!reachCount.empty());
	using pair_t = std::remove_reference<decltype(reachCount)>::type::value_type;
	TraialVec chosenTraials;
    const auto N(traials.size());
    chosenTraials.reserve(N);
	// Find most frequent ImportanceValue
	auto nextStep = std::max_element(begin(reachCount), end(reachCount),
	                                 [](const pair_t& p1, const pair_t& p2)
	                                 { return p1.second < p2.second; });
	const auto nextValue(nextStep->first);
	// Filter the traials vectors
    for (auto i = 0ul ; i < N ; i++) {
        Traial& t(traials.back());
        traials.pop_back();
		if (t.level == nextValue)
			chosenTraials.push_back(t);
        else
			fig::TraialPool::get_instance().return_traial(std::move(t));
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
	// shared_ptr avoids memory leaks and also deletion when exiting scope
	static std::shared_ptr<const EventWatcher> event_watcher(nullptr);
	switch (property.type) {
	case PropertyType::TRANSIENT:
		event_watcher = std::make_shared<const EventWatcher>(
				std::bind(&SimulationEngineSFE::transient_event, this, _1, _2, _3));
		break;
	case PropertyType::RATE:
		event_watcher = std::make_shared<const EventWatcher>(
				std::bind(&SimulationEngineSFE::rate_event, this, _1, _2, _3));
		break;
	default:
		throw_FigException("unsupported property type: "+std::to_string(property.type));
		break;
	}
	return *event_watcher;
}


void
SimulationEngineSFE::fixed_effort(ThresholdsPathCandidates& result,
                                  const EventWatcher& watch_events) const
{
	assert(nullptr != impFun_);

	auto lvl_effort = [&](const unsigned& effort){ return effort*base_nsims(); };
	const size_t LVL_MAX(toBuildThresholds_ ? arbitraryMaxLevel : impFun_->max_value()),
	             LVL_INI(impFun_->initial_value(toBuildThresholds_)),
				 EFF_MAX(std::max(std::max(
							lvl_effort_min(),
							arbitraryLevelEffort),
							lvl_effort(impFun_->max_thresholds_effort(toBuildThresholds_))));
	decltype(reachCount_) reachCountLocal(reachCount_);
	auto tpool(TraialPool::get_instance());
	std::vector< Reference< Traial > > traialsNow, traialsNext;

	// Init result & internal ADTs
	ThresholdsPathCandidates(1).swap(result);
	auto& pathToRare(result.front());
	assert(pathToRare.empty());
	pathToRare.reserve(LVL_MAX);
	traialsNow.reserve(EFF_MAX);
	traialsNext.reserve(EFF_MAX);

	// Bootstrap the Fixed Effort run
	Traial& seedTraial(tpool.get_traial());
	seedTraial.initialise(*model_, *impFun_);
	traialsNext.push_back(seedTraial);
	size_t numSuccesses;
	ImportanceValue l(LVL_INI);

	// Run Fixed Effort: For each threshold level 'l' ...
	do {
        // ... prepare the Traials to run the simulations ...
        numSuccesses = 0ul;
		const size_t LVL_EFFORT =
				std::max(lvl_effort_min(), toBuildThresholds_ ? arbitraryLevelEffort
															  : lvl_effort(impFun_->effort_of(l)));
		assert(0ul < LVL_EFFORT);
        assert(traialsNow.empty());
        assert(!traialsNext.empty());
        for (auto i = 0ul ; i < LVL_EFFORT ; i++) {
            const bool useFresh(i >= traialsNext.size());
			Traial& traial(useFresh ? tpool.get_traial() : traialsNext[i].get());
			if (useFresh)
				traial = traialsNext[i%traialsNext.size()].get();  // copy *contents*
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
			assert(traial.level < LVL_MAX+(toBuildThresholds_?0:1));
			model_->simulation_step(traial, *property_, watch_events);
			if (traial.level > l || property_->is_rare(traial.state))
				numSuccesses++;
			if (traial.level > l) {
				traialsNext.push_back(traial);
				reachCount_[traial.level]++;
				reachCountLocal[traial.level]++;
			} else {
				tpool.return_traial(traial);
			}
        }
		assert(traialsNow.empty());
		// ... and interpret the results
		pathToRare.emplace_back(l, static_cast<double>(numSuccesses)/LVL_EFFORT);
		if (!traialsNext.empty()) {
			auto nextLvl = filter_next_value(traialsNext, reachCountLocal);
			assert(l < nextLvl);
			if (LVL_MAX < nextLvl && LVL_MAX == arbitraryMaxLevel)
				nextLvl = LVL_MAX;  // patch for badly chosen arbitraryMaxLevel
			assert(nextLvl <= LVL_MAX);
			l = nextLvl;
			if (toBuildThresholds_ && l == LVL_MAX)
				pathToRare.emplace_back(l,1.0);
		}
	} while (!traialsNext.empty() &&
			 l < LVL_MAX + (toBuildThresholds_ ? 0 : 1 ));

	tpool.return_traials(traialsNow);
	tpool.return_traials(traialsNext);
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
