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
#include <map>
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
typedef std::vector< fig::Reference< Traial > > TraialVec;
typedef fig::SimulationEngine::ReachabilityCount ReachabilityCount;

/**
 * @brief Select the most likely "next step value" from several posibilities
 *
 *        The vector given as argument contains several Traial instances
 *        that could be the next step of a Fixed Effort sweep.
 *        Using \p ifun check which ImportanceValue has the most occurrences,
 *        and delete from the vector any Traial with different ImportanceValue.
 *
 * @param traials    Traials to analyse and filter
 * @param traialSink Container where discarded Traials will be placed
 * @param reachCount How often each ImportanceValue was reached,
 *                   from among the importance values that occurr in \p traials
 * @param ifun       Importance function to assess the importance of each Traial
 *
 * @return Next step value, viz. ImportanceValue of the chosen (surviving) Traials
 */
ImportanceValue
filter_next_value(TraialVec& traials,
                  TraialVec& traialSink,
                  const ReachabilityCount& reachCount,
                  const fig::ImportanceFunction& ifun)
{
    assert(!traials.empty());
	assert(!reachCount.empty());
	using pair_t = std::remove_reference<decltype(reachCount)>::type::value_type;
//	using pair_t = decltype(typeVar)::value_type;
	TraialVec chosenTraials;
    const auto N(traials.size());
    chosenTraials.reserve(N);
	// Find most frequent ImportanceValue
	auto nextStep = std::max_element(begin(reachCount), end(reachCount),
	                                 [](const pair_t& p1, const pair_t& p2)
	                                 { return p1.second < p2.second; });
	const ImportanceValue nextValue(nextStep->second);
	const size_t nextValueIndex(nextStep->first);
	// Filter the traials vectors
    for (auto i = 0ul ; i < N ; i++) {
        Traial& t(traials.back());
        traials.pop_back();
        if (ifun.importance_of(t.state) == nextValue)
			chosenTraials.push_back(t);
        else
			traialSink.push_back(t);
    }
	assert(traials.empty());
	assert(chosenTraials.size() <= N);
	chosenTraials.swap(traials);
	return nextValueIndex;
}

} // namespace   // // // // // // // // // // // // // // // // // // // // //


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

SimulationEngineSFE::SimulationEngineSFE(std::shared_ptr<const ModuleNetwork> model) :
        SimulationEngineFixedEffort("sfe", model)
{ /* Not much to do around here */ }


SimulationEngineSFE::~SimulationEngineSFE()
{
	TraialPool::get_instance().return_traials(traials_);
}


SimulationEngineFixedEffort::EventWatcher
SimulationEngineSFE::get_event_watcher(const Property& property) const
{
	using namespace std::placeholders;  // _1, _2, ...
	if (property.type == PropertyType::TRANSIENT)
		return std::bind(&SimulationEngineSFE::transient_event, *this, _1, _2, _3);
	else if (property.type == PropertyType::RATE)
		return std::bind(&SimulationEngineSFE::rate_event, *this, _1, _2, _3);
	else
		throw_FigException("unsupported property type: "+std::to_string(property.type));
}


void
SimulationEngineSFE::fixed_effort(const ThresholdsVec& thresholds,
                                  ThresholdsPathCandidates& result,
                                  EventWatcher watch_events) const
{
//	auto event_watcher =
//		(nullptr != watch_events) ? watch_events
//						 : (property_->type == PropertyType::TRANSIENT)
//	                       ? static_cast<EventWatcher>(&fig::SimulationEngineSFE::transient_event)
//						   : (property_->type == PropertyType::RATE)
//	                         ? static_cast<EventWatcher>(&fig::SimulationEngineSFE::rate_event)
//							 : nullptr;
	auto lvl_effort = [&](const size_t& effort){ return effort*base_nsims(); };
	const size_t LVL_MAX(impFun_->max_value()),
				 LVL_INI(impFun_->initial_value()),
				 EFF_MAX(lvl_effort(impFun_->max_thresholds_effort()));
	decltype(reachCount_) reachCountLocal(reachCount_);
	std::vector< Reference< Traial > > traialsNow, traialsNext;

	// Init result & internal ADTs
    if (result.empty())
		result.push_back(ThresholdsPathProb());
	auto pathToRare(*result.begin());
	pathToRare.reserve(thresholds.size());
	pathToRare.clear();
	traialsNow.reserve(EFF_MAX);
	traialsNext.reserve(EFF_MAX);
	if (traials_.size() < EFF_MAX)
		TraialPool::get_instance().get_traials(traials_, EFF_MAX-traials_.size());
//	if (reachCount_.size() != LVL_MAX+1)
//		decltype(reachCount_)(LVL_MAX+1,0).swap(reachCount_);

	// Bootstrap for the Fixed Effort loop that follows
    traials_.back().get().initialise(*model_, *impFun_);
	traialsNext.push_back(std::move(traials_.back()));
    traials_.pop_back();
	size_t numSuccesses;
	ImportanceValue l(LVL_INI);

	// Fixed Effort loop: For each threshold level 'l' ...
	do {
        // ... prepare the Traials to run the simulations ...
        numSuccesses = 0ul;
		const auto LVL_EFFORT(lvl_effort(impFun_->effort_of(l)));
		assert(0 < LVL_EFFORT);
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
			model_.simulation_step(traial, property_, watch_events);
            if (traial.level > l) {
                numSuccesses++;
                traialsNext.push_back(traial);
                reachCount_[l]++;
				reachCountLocal[l]++;
			} else {
				if (property_.is_rare(traial.state))
                    numSuccesses++;
                traials_.push_back(traial);
            }
        }
        // ... and interpret the results
		l = filter_next_value(traialsNext, traials_, reachCountLocal, *impFun_);
        assert(l <= LVL_MAX);
		pathToRare.emplace_back(l, static_cast<double>(numSuccesses)/LVL_EFFORT);
	} while (l < LVL_MAX && traialsNext.size() > 0ul);
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
