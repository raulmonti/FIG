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
#include <cmath>       // std::log
#include <functional>  // std::placeholders
// FIG
#include <core_typedefs.h>
#include <SimulationEngineNosplit.h>
#include <StoppingConditions.h>
#include <FigException.h>
#include <ImportanceFunctionConcrete.h>
#include <PropertyTransient.h>
#include <ConfidenceInterval.h>
#include <FigLog.h>


using namespace std::placeholders;  // _1, _2, _3, ...


namespace fig
{

// Available engine names in SimulationEngine::names
SimulationEngineNosplit::SimulationEngineNosplit(
    std::shared_ptr<const ModuleNetwork> model,
    bool thresholds) :
        SimulationEngine("nosplit", model, thresholds),
        oTraial_(TraialPool::get_instance().get_traial())
{
	if (thresholds)
		throw_FigException("No-split engine (aka standard monte carlo) has "
		                   "not yet been implemented to use for building thresholds");
	/* Not much to do around here */
}


SimulationEngineNosplit::~SimulationEngineNosplit()
{
	//TraialPool::get_instance().return_traial(oTraial_);
	// ^^^ pointless, and besides the TraialPool might be dead already,
    //     so this would trigger a re-creation of the pool or something worse
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
	const EventWatcher& watch_events = impFun_->concrete_simulation()
			? std::bind(&SimulationEngineNosplit::transient_event_concrete, this, _1, _2, _3)
			: std::bind(&SimulationEngineNosplit::transient_event,          this, _1, _2, _3);
//	bool (SimulationEngineNosplit::*watch_events)
//		 (const Property&, Traial&, Event&) const;
//	if (impFun_->concrete())
//		watch_events = &SimulationEngineNosplit::transient_event_concrete;
//	else
//		watch_events = &SimulationEngineNosplit::transient_event;
//	const Property& prop(property);

	// Perform 'numRuns' independent standard Monte Carlo simulations
	for (size_t i = 0ul ; i < numRuns && !interrupted ; i++) {
		traial.initialise(*model_, *impFun_);
//		Event e = model_->simulation_step(traial, prop, *this, watch_events);
		Event e = model_->simulation_step(traial, property, watch_events);
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
	const CLOCK_INTERNAL_TYPE FIRST_TIME(0.0);
	simsLifetime = static_cast<CLOCK_INTERNAL_TYPE>(runLength);

	// For the sake of efficiency, distinguish when operating with a concrete ifun
//	auto watch_events = impFun_->concrete_simulation()
	EventWatcher watch_events = impFun_->concrete_simulation()
			? std::bind(&SimulationEngineNosplit::rate_event_concrete, *this, _1, _2, _3)
	        : std::bind(&SimulationEngineNosplit::rate_event,          *this, _1, _2, _3);
//	auto register_time = impFun_->concrete_simulation()
	EventWatcher register_time = impFun_->concrete_simulation()
			? std::bind(&SimulationEngineNosplit::count_time_concrete, *this, _1, _2, _3)
	        : std::bind(&SimulationEngineNosplit::count_time,          *this, _1, _2, _3);
//	bool (SimulationEngineNosplit::*watch_events)
//		 (const PropertyRate&, Traial&, Event&) const;
//	bool (SimulationEngineNosplit::*register_time)
//		 (const PropertyRate&, Traial&, Event&) const;
//	if (impFun_->concrete_simulation()) {
//		watch_events = &SimulationEngineNosplit::rate_event_concrete;
//		register_time = &SimulationEngineNosplit::count_time_concrete;
//	} else {
//		watch_events = &SimulationEngineNosplit::rate_event;
//		register_time = &SimulationEngineNosplit::count_time;
//	}

	// Run a single standard Monte Carlo simulation for "runLength"
	// simulation time units and starting from the last saved state,
	// or from the system's initial state if requested.
	if (reinit || oTraial_.lifeTime == FIRST_TIME)
		oTraial_.initialise(*model_, *impFun_);
	else
		oTraial_.lifeTime = 0.0;
	do {
//		Event e = model_->simulation_step(oTraial_, property, *this, watch_events);
		Event e = model_->simulation_step(oTraial_, property, watch_events);
		if (!IS_RARE_EVENT(e))
			break;  // reached EOS
		const CLOCK_INTERNAL_TYPE simLength(oTraial_.lifeTime);  // reduce fp prec. loss
		oTraial_.lifeTime = 0.0;
//		model_->simulation_step(oTraial_, property, *this, register_time);
		model_->simulation_step(oTraial_, property, register_time);
		assert(static_cast<CLOCK_INTERNAL_TYPE>(0.0) < oTraial_.lifeTime);
		accTime += oTraial_.lifeTime;
		oTraial_.lifeTime += simLength;
		if (oTraial_.lifeTime > SIM_TIME_CHUNK
			&& simsLifetime > SIM_TIME_CHUNK) {
			// reduce fp precision loss
			oTraial_.lifeTime -= SIM_TIME_CHUNK;
			simsLifetime -= SIM_TIME_CHUNK;
		}
	} while (oTraial_.lifeTime < simsLifetime && !interrupted);

	// Allow next iteration of batch means
	if (oTraial_.lifeTime == FIRST_TIME)
		oTraial_.lifeTime = (FIRST_TIME + 1.1) * 2.2;
	assert(oTraial_.lifeTime != FIRST_TIME);

	// Return the simulation-time spent on rare states
	assert(0.0 <= accTime);
	return accTime;
}

} // namespace fig
