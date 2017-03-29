//==============================================================================
//
//  SimulationEngineNosplit.cpp
//
//  Copyleft 2015-
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
#include <cmath>  // std::log
// FIG
#include <core_typedefs.h>
#include <SimulationEngineNosplit.h>
#include <StoppingConditions.h>
#include <FigException.h>
#include <ImportanceFunctionConcrete.h>
#include <PropertyTransient.h>
#include <ConfidenceInterval.h>
#include <FigLog.h>


namespace fig
{

// Available engine names in SimulationEngine::names
SimulationEngineNosplit::SimulationEngineNosplit(
    std::shared_ptr<const ModuleNetwork> network) :
		SimulationEngine("nosplit", network)
{ /* Not much to do around here */ }


double
SimulationEngineNosplit::log_experiments_per_sim() const
{
	if (!bound())
		throw_FigException("engine isn't bound to any importance function");
	return 0.0;  // == log(1)
}


std::vector<double>
SimulationEngineNosplit::transient_simulations(const PropertyTransient& property,
                                               const size_t& numRuns) const
{
	assert(0ul < numRuns);
	std::vector< double > raresCount;
	raresCount.reserve(numRuns);
	Traial& traial = TraialPool::get_instance().get_traial();

	// For the sake of efficiency, distinguish when operating with a concrete ifun
    bool (SimulationEngineNosplit::*watch_events)
         (const PropertyTransient&, Traial&, Event&) const;
    if (impFun_->concrete())
		watch_events = &SimulationEngineNosplit::transient_event_concrete;
    else
		watch_events = &SimulationEngineNosplit::transient_event;

	// Perform 'numRuns' independent standard Monte Carlo simulations
	for (size_t i = 0ul ; i < numRuns && !interrupted ; i++) {
		traial.initialize(*network_, *impFun_);
        Event e = network_->simulation_step(traial, property, *this, watch_events);
		raresCount.push_back(IS_RARE_EVENT(e) ? 1.0l : 0.0l);
    }
    TraialPool::get_instance().return_traial(std::move(traial));

	// Return number of rare states visited
	return raresCount;
}


double
SimulationEngineNosplit::rate_simulation(const PropertyRate& property,
										 const size_t& runLength,
										 bool reinit) const
{
	assert(0ul < runLength);
	double accTime(0.0);
//	static thread_local Traial& traial(TraialPool::get_instance().get_traial());
	static Traial& traial(TraialPool::get_instance().get_traial());
	const CLOCK_INTERNAL_TYPE FIRST_TIME(0.0);
	simsLifetime = static_cast<CLOCK_INTERNAL_TYPE>(runLength);

	// For the sake of efficiency, distinguish when operating with a concrete ifun
	bool (SimulationEngineNosplit::*watch_events)
		 (const PropertyRate&, Traial&, Event&) const;
	bool (SimulationEngineNosplit::*register_time)
		 (const PropertyRate&, Traial&, Event&) const;
	if (impFun_->concrete_simulation()) {
		watch_events = &SimulationEngineNosplit::rate_event_concrete;
		register_time = &SimulationEngineNosplit::count_time_concrete;
	} else {
		watch_events = &SimulationEngineNosplit::rate_event;
		register_time = &SimulationEngineNosplit::count_time;
	}

	// Run a single standard Monte Carlo simulation for "runLength"
	// simulation time units and starting from the last saved state,
	// or from the system's initial state if requested.
	if (reinit || traial.lifeTime == FIRST_TIME)
		traial.initialize(*network_, *impFun_);
	else
		traial.lifeTime = 0.0;
	do {
		Event e = network_->simulation_step(traial, property, *this, watch_events);
		if (!IS_RARE_EVENT(e))
			break;  // reached EOS
		const CLOCK_INTERNAL_TYPE simLength(traial.lifeTime);  // reduce fp prec. loss
		traial.lifeTime = 0.0;
		network_->simulation_step(traial, property, *this, register_time);
		assert(static_cast<CLOCK_INTERNAL_TYPE>(0.0) < traial.lifeTime);
		accTime += traial.lifeTime;
		traial.lifeTime += simLength;
		if (traial.lifeTime > SIM_TIME_CHUNK
			&& simsLifetime > SIM_TIME_CHUNK) {
			// reduce fp precision loss
			traial.lifeTime -= SIM_TIME_CHUNK;
			simsLifetime -= SIM_TIME_CHUNK;
		}
	} while (traial.lifeTime < simsLifetime && !interrupted);

	// Allow next iteration of batch means
	if (traial.lifeTime == FIRST_TIME)
		traial.lifeTime = (FIRST_TIME + 1.1) * 2.2;
	assert(traial.lifeTime != FIRST_TIME);

	// Return the simulation-time spent on rare states
	assert(0.0 <= accTime);
	return accTime;
}

} // namespace fig
