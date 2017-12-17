//==============================================================================
//
//  SimulationEngineBFE.cpp
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
#include <SimulationEngineBFE.h>
#include <PropertyTransient.h>
#include <TraialPool.h>
#include <ModelSuite.h>

// ADL
using std::begin;
using std::end;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

SimulationEngineBFE::SimulationEngineBFE(
	std::shared_ptr<const ModuleNetwork> network,
	unsigned effortPerLevel) :
		SimulationEngineFixedEffort("bfe", network),
		effortPerLevel_(effortPerLevel)
{ /* Not much to do around here */ }


std::vector<double>
SimulationEngineBFE::transient_simulations(const PropertyTransient& property,
												   const size_t& numRuns) const
{
	auto event_watcher = &fig::SimulationEngineBFE::transient_event;
	auto lvl_effort = [&](const size_t& effort){ return effort*base_nsims(); };
	const ModuleNetwork& network(*fig::ModelSuite::get_instance().modules_network());
	const size_t LVL_MAX(impFun_->max_value()+1),
				 LVL_INI(impFun_->initial_value()),
				 EFF_MAX(lvl_effort(impFun_->max_thresholds_effort()));

	assert(0ul < numRuns);
	assert(LVL_INI < LVL_MAX);
	assert(0ul < EFF_MAX);
	assert(0ul < effortPerLevel_);

	std::vector< double > Pup(LVL_MAX);
	std::vector< double > results(numRuns);
	TraialPool::get_instance().get_traials(traials_, EFF_MAX);
	std::vector< Reference< Traial > > traialsNow, traialsNext;
	traialsNow.reserve(EFF_MAX);
	traialsNext.reserve(EFF_MAX);
	if (reachCount_.size() != LVL_MAX)
		decltype(reachCount_)(LVL_MAX,0).swap(reachCount_);

	// Perform 'numRuns' independent Fixed Effort simulations
	for (size_t i = 0ul ; i < numRuns ; i++) {
		// Re-init ADTs
		std::fill(begin(Pup), end(Pup), 1.0f);
		std::move(begin(traialsNow), end(traialsNow), std::back_inserter(traials_));
		std::move(begin(traialsNext), end(traialsNext), std::back_inserter(traials_));
		traialsNow.clear();
		traialsNext.clear();
		assert(traials_.size() == EFF_MAX);
		for (auto j = 0ul ; j < lvl_effort(impFun_->effort_of(LVL_INI)) ; j++) {
			traials_.back().get().initialise(network,*impFun_);
			traialsNext.push_back(std::move(traials_.back()));
			traials_.pop_back();
		}



		fixed_effort(property,
					 *this,
					 event_watcher,
					 impFun_->thresholds(),
					 Pup);




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
		// The product of these conditional probabilities for all levels
		// is the rare event estimate
		results[i] = 1.0;
		for (l = LVL_INI ; l < LVL_MAX && 0.0 < results[i] ; l++)
			results[i] *= Pup[l];
		assert(0ul < numSuccesses || 0.0 == results[i]);
	}

	TraialPool::get_instance().return_traials(traials_);
	return results;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
