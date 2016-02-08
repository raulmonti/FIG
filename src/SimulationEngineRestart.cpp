//==============================================================================
//
//  SimulationEngineRestart.cpp
//
//  Copyleft 2016-
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
#include <cmath>
// C++
#include <stack>
#include <limits>    // std::numeric_limits<>
#include <valarray>  // std::valarray<>
// FIG
#include <SimulationEngineRestart.h>
#include <PropertyTransient.h>
#include <ModuleNetwork.h>
#include <ModelSuite.h>
#include <TraialPool.h>
#include <FigException.h>


namespace fig
{

// Available engine names in SimulationEngine::names
SimulationEngineRestart::SimulationEngineRestart(
	std::shared_ptr<const ModuleNetwork> network,
	const unsigned& splitsPerThreshold,
	const unsigned& dieOutDepth) :
		SimulationEngine("restart", network),
		splitsPerThreshold_(splitsPerThreshold),
		dieOutDepth_(dieOutDepth)
{ /* Not much to do around here */ }


void
SimulationEngineRestart::bind(std::shared_ptr< const ImportanceFunction > ifun_ptr)
{
	const std::string impStrategy(ifun_ptr->strategy());
	if (impStrategy == "")
		throw_FigException("ImportanceFunction doesn't seem to have "
						   "internal importance information");
	else if (impStrategy == "flat")
		throw_FigException("RESTART simulation engine requires an importance "
						   "building strategy other than \"flat\"");
	SimulationEngine::bind(ifun_ptr);
}


void
SimulationEngineRestart::set_splits_per_threshold(unsigned splitsPerThreshold)
{
	if (splitsPerThreshold < 2u)
		throw_FigException("at least 1 Traial must be created, besides the "
						   "original one, when crossing a threshold upwards");
	splitsPerThreshold_ = splitsPerThreshold;
}


void
SimulationEngineRestart::set_die_out_depth(unsigned dieOutDepth)
{
	dieOutDepth_ = dieOutDepth;
}


double
SimulationEngineRestart::transient_simulations(const PropertyTransient& property,
											   const size_t& numRuns) const
{
	assert(0u < numRuns);
	unsigned numThresholds(impFun_->num_thresholds());
	std::valarray<long> rareEventsCount(numThresholds, 0l);
	std::stack< Reference< Traial > > stack;
	auto tpool = TraialPool::get_instance();

	// For the sake of efficiency, distinguish when operating with a concrete ifun
	bool (SimulationEngineRestart::*watch_events)
		 (const PropertyTransient&, Traial&, Event&) const;
	if (impFun_->concrete())
		watch_events = &SimulationEngineRestart::transient_event_concrete;
	else
		watch_events = &SimulationEngineRestart::transient_event;

	// Perform 'numRuns' RESTART importance-splitting simulations
	for (size_t i = 0u ; i < numRuns ; i++) {
		tpool.get_traials(stack, 1u);
		static_cast<Traial&>(stack.top()).initialize(*network_, *impFun_);

		while (!stack.empty()) {
			Traial& traial = stack.top();
			ImportanceValue prevThr = traial.importance;
			Event e = network_->simulation_step(traial, property, *this, watch_events);

			// The following events are treated as mutually exclusive
			// Checking order is relevant!
			if (IS_STOP_EVENT(e) || IS_THR_DOWN_EVENT(e)) {
				// Traial hit the wall or went down => kill it
				stack.pop();
			} else if (IS_RARE_EVENT(e)) {
				// Reached rare event => count and kill
				rareEventsCount[traial.importance]++;
				stack.pop();
			} else if (IS_THR_UP_EVENT(e)) {
				// Could have gone up several thresholds => split accordingly
				for (unsigned short i = 1u
					; i <= traial.importance - prevThr  // # thresholds crossed
					; i++)
				{
					unsigned thisLevelRetrials =
						std::round(std::pow(splitsPerThreshold_, i)
									- std::pow(splitsPerThreshold_, i-1));
					tpool.get_traial_copies(stack, traial, thisLevelRetrials);
				}
				continue;  // attend the offsprings
			}
		}
	}
	double estimate(0.0);
	assert(0l <= rareEventsCount.sum());

	// To estimate, weigh counts by their threshold level's relative importance
	for (unsigned i = 0u ; i < numThresholds ; i++)
		estimate += rareEventsCount[i] * std::pow(splitsPerThreshold_, numThresholds-i-1u);
	assert(estimate >= 0.0);
	if (ModelSuite::MIN_COUNT_RARE_EVENTS > rareEventsCount.sum()) {
		/// @todo TODO proper log in technical log
		std::cerr << "Too few rare events generated (" << rareEventsCount.sum() << ")\n";
		estimate *= -1.0;
	}

	return estimate;
}

} // namespace fig
