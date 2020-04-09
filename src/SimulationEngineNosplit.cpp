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
#include <cmath>	   // std::log
#include <functional>  // std::placeholders
// FIG
#include <core_typedefs.h>
#include <FigLog.h>
#include <FigException.h>
#include <SimulationEngineNosplit.h>
#include <StoppingConditions.h>
#include <ImportanceFunctionConcrete.h>
#include <PropertyRate.h>
#include <PropertyTBoundSS.h>
#include <PropertyTransient.h>
#include <ConfidenceInterval.h>


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
	//	 so this would trigger a re-creation of the pool or something worse
}


std::vector<double>
SimulationEngineNosplit::transient_simulations(const PropertyTransient& property,
                                               const size_t& numRuns) const
{
	assert(0ul < numRuns);
	std::vector< double > raresCount(numRuns, 0.0l);
	Traial& traial = TraialPool::get_instance().get_traial();

	// For the sake of efficiency, distinguish when operating with a concrete ifun
	const EventWatcher& watch_events = impFun_->concrete_simulation()
	        ? std::bind(&SimulationEngineNosplit::transient_event_concrete, this, _1, _2, _3)
	        : std::bind(&SimulationEngineNosplit::transient_event,          this, _1, _2, _3);

	// Perform 'numRuns' independent standard Monte Carlo simulations
	for (size_t i = 0ul ; i < numRuns && !interrupted ; i++) {
		traial.initialise(*model_, *impFun_);
		Event e = model_->simulation_step(traial, property, watch_events);
		raresCount[i] = IS_RARE_EVENT(e) ? 1.0l : 0.0l;
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
	const decltype(oTraial_.lifeTime) FIRST_TIME(0.0);
	simsLifetime = static_cast<CLOCK_INTERNAL_TYPE>(runLength);

	// For the sake of efficiency, distinguish when operating with a concrete ifun
	EventWatcher watch_events = impFun_->concrete_simulation()
	        ? std::bind(&SimulationEngineNosplit::rate_event_concrete, this, _1, _2, _3)
	        : std::bind(&SimulationEngineNosplit::rate_event,          this, _1, _2, _3);
	EventWatcher register_time = impFun_->concrete_simulation()
	        ? std::bind(&SimulationEngineNosplit::count_time_concrete, this, _1, _2, _3)
	        : std::bind(&SimulationEngineNosplit::count_time,          this, _1, _2, _3);

	// Run a single standard Monte Carlo simulation for "runLength"
	// simulation time units and starting from the last saved state...
	oTraial_.lifeTime = 0.0;
	// ...or from the system's initial state if requested.
	if (reinit || oTraial_.lifeTime <= FIRST_TIME)
		oTraial_.initialise(*model_, *impFun_);
	do {
		Event e = model_->simulation_step(oTraial_, property, watch_events);
		if (!IS_RARE_EVENT(e))
			break;  // reached EOS
		const auto simLength(oTraial_.lifeTime);  // reduce fp prec. loss
		oTraial_.lifeTime = 0.0;
		model_->simulation_step(oTraial_, property, register_time);
		assert(static_cast<CLOCK_INTERNAL_TYPE>(0.0) < oTraial_.lifeTime);
		oTraial_.lifeTime = std::min(oTraial_.lifeTime, simsLifetime);
		accTime += static_cast<decltype(accTime)>(oTraial_.lifeTime);
		oTraial_.lifeTime += simLength;
		/// @todo TODO delete deprecated code below
//		if (oTraial_.lifeTime > SIM_TIME_CHUNK
//			&& simsLifetime > SIM_TIME_CHUNK) {
//			// reduce fp precision loss
//			oTraial_.lifeTime -= SIM_TIME_CHUNK;
//			simsLifetime -= SIM_TIME_CHUNK;
//		}
	} while (oTraial_.lifeTime < simsLifetime && !interrupted);

	// Allow next iteration of batch means
	if (oTraial_.lifeTime <= FIRST_TIME)
		oTraial_.lifeTime = (FIRST_TIME + 1.1f) * 2.2f;
	assert(oTraial_.lifeTime > FIRST_TIME);

	// Return the simulation-time spent on rare states
#ifndef NDEBUG
	assert(0.0 <= accTime);
	assert(accTime <= runLength);
#else
	accTime = std::min<double>(accTime, runLength);
#endif
	return static_cast<double>(accTime);
}


double
SimulationEngineNosplit::tbound_ss_simulation(const PropertyTBoundSS& property) const
{
	const auto transientTime = property.tbound_low();
	const auto finishTime = property.tbound_upp();
	assert(0 <= transientTime);
	assert(transientTime < finishTime);
	const auto batchTime = static_cast<double>(finishTime-transientTime);
	double accTime(0.0);

	// For the sake of efficiency, distinguish when operating with a concrete ifun
	EventWatcher watch_events = impFun_->concrete_simulation()
	        ? std::bind(&SimulationEngineNosplit::rate_event_concrete, this, _1, _2, _3)
	        : std::bind(&SimulationEngineNosplit::rate_event,          this, _1, _2, _3);
	EventWatcher register_time = impFun_->concrete_simulation()
	        ? std::bind(&SimulationEngineNosplit::count_time_concrete, this, _1, _2, _3)
	        : std::bind(&SimulationEngineNosplit::count_time,          this, _1, _2, _3);
	EventWatcher discard_transient =
			std::bind(&SimulationEngineNosplit::kill_time,             this, _1, _2, _3);

	// Run a single standard Monte Carlo simulation:
	oTraial_.initialise(*model_, *impFun_);

	// - first discard transient phase
	simsLifetime = static_cast<CLOCK_INTERNAL_TYPE>(transientTime);
	model_->simulation_step(oTraial_, property, discard_transient);
	assert(oTraial_.lifeTime >= transientTime || interrupted);

	// - and then register (time of) property satisfaction up to finishTime
	simsLifetime = static_cast<CLOCK_INTERNAL_TYPE>(finishTime);
	do {
		Event e = model_->simulation_step(oTraial_, property, watch_events);
		if (!IS_RARE_EVENT(e))
			break;  // reached EOS
		const auto simLength(oTraial_.lifeTime);  // reduce fp prec. loss
		oTraial_.lifeTime = 0.0;
		model_->simulation_step(oTraial_, property, register_time);
		assert(static_cast<CLOCK_INTERNAL_TYPE>(0.0) < oTraial_.lifeTime);
		oTraial_.lifeTime = std::min(oTraial_.lifeTime, simsLifetime);
		accTime += static_cast<decltype(accTime)>(oTraial_.lifeTime);
		oTraial_.lifeTime += simLength;
	} while (oTraial_.lifeTime < simsLifetime && !interrupted);

	// Return the simulation-time spent on rare states
#ifndef NDEBUG
	assert(0.0 <= accTime);
	assert(accTime <= batchTime);
#else
	accTime = std::min<double>(accTime, batchTime);
#endif
	return static_cast<double>(accTime);
}

} // namespace fig
