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
#include <algorithm>  // std::fill(), std::move()
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
using TraialRef = fig::Reference< Traial >;

/// @brief From several potential "next step values" select the most likely one
/// @details The vector given as argument contains several Traial instances
///          that could be the next step of a Fixed Effort sweep.
///          Using \p ifun check which ImportanceValue has the most occurrences,
///          and delete from the vector any Traial that doesn't has a different
///          ImportanceValue.
void filter_next_value(std::vector< TraialRef >& traialsNext,
                       const fig::ImportanceFunction& ifun)
{
	/// @todo TODO implement!
}

} // namespace   // // // // // // // // // // // // // // // // // // // // //


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

using SimulationEngineFixedEffort::EventWatcher;


SimulationEngineSFE::SimulationEngineSFE(
	std::shared_ptr<const ModuleNetwork> network,
	unsigned effortPerLevel) :
		SimulationEngineFixedEffort("sfe", network),
		effortPerLevel_(effortPerLevel)
{ /* Not much to do around here */ }


void
SimulationEngineSFE::fixed_effort(const ThresholdsVec& thresholds,
                                  ThresholdsPathCandidates& result,
								  EventWatcher fun)
{
	auto event_watcher =
		(nullptr != fun) ? fun
						 : (property_->type == PropertyType::TRANSIENT)
						   ? &fig::SimulationEngineSFE::transient_event
						   : (property_->type == PropertyType::RATE)
							 ? &fig::SimulationEngineSFE::rate_event
							 : nullptr;
	auto lvl_effort = [&](const size_t& effort){ return effort*base_nsims(); };
	const size_t LVL_MAX(impFun_->max_value()+1),
				 LVL_INI(impFun_->initial_value()),
				 EFF_MAX(lvl_effort(impFun_->max_thresholds_effort()));

	std::vector< Reference< Traial > > traialsNow, traialsNext;
	traialsNow.reserve(EFF_MAX);
	traialsNext.reserve(EFF_MAX);
	if (traials_.size() < EFF_MAX)
		TraialPool::get_instance().get_traials(traials_, EFF_MAX);
	if (reachCount_.size() != LVL_MAX)
		decltype(reachCount_)(LVL_MAX,0).swap(reachCount_);
	for (auto j = 0ul ; j < lvl_effort(impFun_->effort_of(LVL_INI)) ; j++) {
		traials_.back().get().initialise(*model_,*impFun_);
		traialsNext.push_back(std::move(traials_.back()));
		traials_.pop_back();
	}


	size_t numSuccesses;
	ImportanceValue l(LVL_INI);
	do {
		numSuccesses = 0ul;
		const auto LVL_EFFORT(lvl_effort(impFun_->effort_of(l)));
		assert(0 < LVL_EFFORT);
		assert(traialsNow.empty());
		assert(!traialsNext.empty());




		filter_next_value(traialsNext, *impFun_);



	} while (l < LVL_MAX && traialsNext.size() > 0ul);







	// For each threshold level 'l' ...
	ImportanceValue l;
	size_t numSuccesses(1ul);
	for (l = LVL_INI ; l < LVL_MAX && 0ul < traialsNext.size() ; l++) {
		// ... prepare the Traials to run the simulations ...
		numSuccesses = 0ul;
		const auto LVL_EFFORT(lvl_effort(impFun_->effort_of(l)));
		assert(0 < LVL_EFFORT);
		assert(traialsNow.empty());
		assert(!traialsNext.empty());
		for (auto j = 0ul ; j < LVL_EFFORT ; j++) {
			const bool useFresh(j >= traialsNext.size());
			Traial& traial(useFresh ? traials_.back() : traialsNext[j]);
			if (useFresh) {
				traials_.pop_back();
				traial = traialsNext[j%traialsNext.size()].get();  // copy *contents*
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
		// ... run Fixed Effort until the next level 'l+1' ...
		do {
			Traial& traial(traialsNow.back());
			traialsNow.pop_back();
			assert(traial.level < LVL_MAX);
			reachCount_[traial.level]++;
			network.simulation_step(traial, property, *this, event_watcher);
			if (traial.level > l) {
				numSuccesses++;
				traialsNext.push_back(traial);
			} else {
				if (property.is_rare(traial.state))
					numSuccesses++;
				traials_.push_back(traial);
			}
		} while (!traialsNow.empty());
		// ... and estimate the probability of reaching 'l+1' from 'l'
		Pup[l] = static_cast<double>(numSuccesses) / LVL_EFFORT;
	}
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
