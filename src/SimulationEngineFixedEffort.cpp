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
#include <algorithm>  // std::fill(), std::move()
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
	const std::string& simEngineName,
	std::shared_ptr<const ModuleNetwork> model,
	bool thresholds) :
		SimulationEngine(simEngineName, model, thresholds),
		arbitrary_effort([](const unsigned&){return 0u;}),
		arbitraryMaxLevel(0ul),
		property_(nullptr)
{ /* Not much to do around here */ }


SimulationEngineFixedEffort::~SimulationEngineFixedEffort()
{
	//TraialPool::get_instance().return_traials(traials_);
	// ^^^ pointless, and besides the TraialPool might be dead already,
	//     so this would trigger a re-creation of the pool
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
SimulationEngineFixedEffort::transient_simulations(
		const PropertyTransient& property,
		const size_t& numRuns) const
{
	std::vector< double > results(numRuns);
	ThresholdsPathCandidates Pup;

	if (toBuildThresholds_)
		throw_FigException("invalid invocation of transient_simulations(): "
		                   "this instance of SimulationEngineFixedEffort "
		                   "was created to build thresholds");

	if (nullptr == property_ || property_->get_id() != property.get_id())
		property_ = &property;  // bind new property

	// Perform 'numRuns' independent Fixed Effort simulations
	for (size_t i = 0ul ; i < numRuns ; i++) {

		// Populate 'Pup' with importance paths towards the rare event
		fixed_effort(Pup, get_event_watcher(property));
		/// fixed_effort(impFun_->thresholds(), Pup, watch_events);

		// For each potential path from the initial state to the rare event,
		// the product of the conditional probabilities for all levels
		// is the rare event estimate.
		// If several (*disjoint*) paths exist, then the summation
		// for all paths is the rare event estimate.
		results[i] = 0.0;
		for (const auto& path: Pup) {
			if (path.empty())
				continue;
			double pathEstimate(1.0);
			for (const auto& condProb: path)
				pathEstimate *= condProb.second;
			results[i] += pathEstimate;
		}
	}

	return results;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
