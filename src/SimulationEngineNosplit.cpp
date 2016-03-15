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


double
SimulationEngineNosplit::transient_simulations(const PropertyTransient& property,
                                               const size_t& numRuns) const
{
	assert(0ul < numRuns);
	long raresCount(0);
    Traial& traial = TraialPool::get_instance().get_traial();

	// For the sake of efficiency, distinguish when operating with a concrete ifun
    bool (SimulationEngineNosplit::*watch_events)
         (const PropertyTransient&, Traial&, Event&) const;
    if (impFun_->concrete())
        watch_events = &SimulationEngineNosplit::transient_event_concrete;
    else
        watch_events = &SimulationEngineNosplit::transient_event;

    // Perform 'numRuns' standard Monte Carlo simulations
	for (size_t i = 0ul ; i < numRuns && !interrupted ; i++) {
		traial.initialize(*network_, *impFun_);
        Event e = network_->simulation_step(traial, property, *this, watch_events);
		raresCount += IS_RARE_EVENT(e) ? 1l : 0l;
    }
    TraialPool::get_instance().return_traial(std::move(traial));

    // Return estimate or its negative value
	if (MIN_COUNT_RARE_EVENTS > raresCount)
		return -static_cast<double>(raresCount);
    else
		return  static_cast<double>(raresCount);
}


double
SimulationEngineNosplit::rate_simulation(const PropertyRate& property,
										 const size_t& runLength) const
{
	assert(0ul < runLength);
	double accTime(0.0);
	Traial& traial = TraialPool::get_instance().get_traial();
	simsLifetime = static_cast<CLOCK_INTERNAL_TYPE>(runLength);

	// For the sake of efficiency, distinguish when operating with a concrete ifun
	bool (SimulationEngineNosplit::*watch_events)
		 (const PropertyRate&, Traial&, Event&) const;
	bool (SimulationEngineNosplit::*register_time)
		 (const PropertyRate&, Traial&, Event&) const;
	if (impFun_->concrete()) {
		watch_events = &SimulationEngineNosplit::rate_event_concrete;
		register_time = &SimulationEngineNosplit::count_time_concrete;
	} else {
		watch_events = &SimulationEngineNosplit::rate_event;
		register_time = &SimulationEngineNosplit::count_time;
	}

	// Run a single standard Monte Carlo simulation for "runLength"
	// simulation time units and starting from the system's initial state
	traial.initialize(*network_, *impFun_);
	do {
		Event e = network_->simulation_step(traial, property, *this, watch_events);
		if (!IS_RARE_EVENT(e))
			break;  // reached EOS
		const CLOCK_INTERNAL_TYPE simLength(traial.lifeTime);  // hack to improve fp precision
		traial.lifeTime = 0.0;
		network_->simulation_step(traial, property, *this, register_time);
		accTime += traial.lifeTime;
		traial.lifeTime += simLength;
	} while (traial.lifeTime < simsLifetime && !interrupted);
	TraialPool::get_instance().return_traial(std::move(traial));

	// Return estimate or its negative value
	assert(0.0 <= accTime);
	if (MIN_ACC_RARE_TIME > accTime)
		return -accTime / simsLifetime;
	else
		return  accTime / simsLifetime;
}

} // namespace fig
