//==============================================================================
//
//  SimulationEngineFixedEffort.cpp
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
#include <algorithm>  // std::fill()
// FIG
#include <SimulationEngineFixedEffort.h>
#include <PropertyTransient.h>
#include <TraialPool.h>
#include <ModelSuite.h>

// ADL
using std::begin;
using std::end;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

SimulationEngineFixedEffort::SimulationEngineFixedEffort(
    std::shared_ptr<const ModuleNetwork> network) :
        SimulationEngine("fixedeffort", network)
{
	TraialPool::get_instance().get_traials(traials_, effort_per_level());
}


SimulationEngineFixedEffort::~SimulationEngineFixedEffort()
{
	TraialPool::get_instance().return_traials(traials_);
}


void
SimulationEngineFixedEffort::bind(std::shared_ptr< const ImportanceFunction > ifun_ptr)
{
	if (ifun_ptr->strategy() == "")
		throw_FigException("ImportanceFunction doesn't seem to have "
		                   "internal importance information");
	else if (ifun_ptr->strategy() == "flat")
		throw_FigException("RESTART simulation engine requires an importance "
		                   "building strategy other than \"flat\"");
	SimulationEngine::bind(ifun_ptr);
}


std::vector<double>
SimulationEngineFixedEffort::transient_simulations(const PropertyTransient& property,
                                                   const size_t& numRuns) const
{
	auto event_watcher = &fig::SimulationEngineFixedEffort::transient_event;
	const ModuleNetwork& network(*fig::ModelSuite::get_instance().modules_network());
	const size_t IMP_RANGE(impFun_->max_value(true) - impFun_->initial_value(true));
	std::vector< Reference< Traial > > freeNow, freeNext, startNow, startNext;
	std::vector< double > results(numRuns);
	std::vector< double > Pup(IMP_RANGE);

	assert(0ul < numRuns);
	assert(0ul < IMP_RANGE);
	assert(0ul < effort_per_level());
	assert(traials_.size() == effort_per_level());
	freeNow.reserve(effort_per_level());
	freeNext.reserve(effort_per_level());
	startNow.reserve(effort_per_level());
	startNext.reserve(effort_per_level());

	// Perform 'numRuns' independent Fixed Effort simulations
	for (size_t i = 0ul ; i < numRuns ; i++) {
		// Re-init ADTs
		std::fill(begin(Pup), end(Pup), 1.0f);
		freeNow.clear();
		freeNext.clear();
		startNow.clear();
		startNext.clear();
		for (Traial& t: traials_) {
			t.initialise(network,*impFun_);
			t.level = impFun_->initial_value(true);  // use importance value, not threshold level
			startNow.push_back(t);  // could use 'freeNow' as well
		}
		// For each importance value 'i' ...
		ImportanceValue imp;
		double numSuccesses(1.0);
		for (imp = impFun_->initial_value(true) ;
		     imp < impFun_->max_value(true) && !startNow.empty() && numSuccesses > 0.0;
		     imp++)
		{
			// ... run Fixed Effort until the next importance value 'i+1' ...
			numSuccesses = 0.0;
			size_t startNowIdx(0ul);
			while ( ! (freeNow.empty() && startNow.empty()) ) {
				// (Traial fetching for simulation)
				const bool useFree = !freeNow.empty();
				Traial& traial( useFree ? freeNow.back() : startNow.back());
				if (useFree) {
					traial = startNow[startNowIdx++].get();  // copy *contents*
					startNowIdx %= startNow.size();
					freeNow.pop_back();
				} else {
					startNow.pop_back();
				}
				// (simulation & bookkeeping)
				assert(traial.level == imp);
				traial.depth = 0;
				network.simulation_step(traial, property, *this, event_watcher);
				if (traial.level > imp) {
					numSuccesses += 1.0;
					startNext.push_back(traial);
				} else {
					if (property.is_rare(traial.state))
						numSuccesses += 1.0;
					freeNext.push_back(traial);
				}
			}
			// ... estimate the probability of reaching 'i+1' from 'i' ...
			Pup[imp] = numSuccesses / effort_per_level();
			std::swap(freeNow, freeNext);
			std::swap(startNow, startNext);
		}
		// ... and the final product of everything is the rare event estimate
		results[i] = 1.0;
		for (auto p: Pup)
			results[i] *= p;
	}
	return results;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
