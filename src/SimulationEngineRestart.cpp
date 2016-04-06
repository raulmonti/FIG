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
#include <ModuleNetwork.h>
#include <TraialPool.h>
#include <FigException.h>

using std::pow;


/// @todo TODO erase debug include
#include <flag.h>

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


unsigned
SimulationEngineRestart::splits_per_threshold() const noexcept
{
	return splitsPerThreshold_;
}


const unsigned&
SimulationEngineRestart::die_out_depth() const noexcept
{
	return dieOutDepth_;
}


void
SimulationEngineRestart::bind(std::shared_ptr< const ImportanceFunction > ifun_ptr)
{
    if (locked())
        throw_FigException("engine \"" + name() + "\" is currently locked "
                           "in \"simulation mode\"");
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
SimulationEngineRestart::set_splits_per_threshold(unsigned spt)
{
    if (locked())
        throw_FigException("engine \"" + name() + "\" is currently locked "
                           "in \"simulation mode\"");
    if (spt < 2u)
		throw_FigException("bad splitting value \"" + std::to_string(spt) + "\". "
						   "At least one Traial must be created, besides the "
						   "original one, when crossing a threshold upwards");
    splitsPerThreshold_ = spt;
}


void
SimulationEngineRestart::set_die_out_depth(unsigned dieOutDepth)
{
    if (locked())
        throw_FigException("engine \"" + name() + "\" is currently locked "
                           "in \"simulation mode\"");
    dieOutDepth_ = dieOutDepth;
}


double
SimulationEngineRestart::log_experiments_per_sim() const
{
	if (!bound())
		throw_FigException("engine isn't bound to any importance function");
	// log( splitsPerThreshold ^ numThresholds )
	return impFun_->num_thresholds() * std::log(splitsPerThreshold_);
}


double
SimulationEngineRestart::transient_simulations(const PropertyTransient& property,
											   const size_t& numRuns) const
{
	assert(0u < numRuns);
	const unsigned numThresholds(impFun_->num_thresholds());
	std::valarray<unsigned> raresCount(0u, numThresholds+1);
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
	for (size_t i = 0u ; i < numRuns && !interrupted ; i++) {
		tpool.get_traials(stack, 1u);
		stack.top().get().initialize(*network_, *impFun_);

		while (!stack.empty()) {
			Event e(EventType::NONE);
			Traial& traial = stack.top();

			// Check whether we're standing on a rare event first
			(this->*watch_events)(property, traial, e);
			if (IS_RARE_EVENT(e)) {
				// We are? Then count and kill
				raresCount[traial.level]++;
				tpool.return_traial(std::move(traial));
				stack.pop();
				continue;
			}
			// We aren't? Then keep dancing
			e = network_->simulation_step(traial, property, *this, watch_events);

			// The following events are treated as mutually exclusive
			// Checking order is relevant!
			if (IS_STOP_EVENT(e) || IS_THR_DOWN_EVENT(e)) {
				// Traial reached a stop event or went down => kill it
				tpool.return_traial(std::move(traial));
				stack.pop();

			} else if (IS_THR_UP_EVENT(e)) {
				// Could have gone up several thresholds => split accordingly
				assert(traial.numLevelsCrossed > 0);
				for (ImportanceValue i = static_cast<ImportanceValue>(1u)
					; i <= static_cast<ImportanceValue>(traial.numLevelsCrossed)
					; i++)
				{
					const unsigned thisLevelRetrials = std::round(
						(splitsPerThreshold_-1u) * pow(splitsPerThreshold_, i-1));
					assert(0u < thisLevelRetrials);
					assert(thisLevelRetrials < pow(splitsPerThreshold_, numThresholds));
					tpool.get_traial_copies(stack, traial, thisLevelRetrials,
											static_cast<short>(i)-traial.numLevelsCrossed);
				}
				// Offsprings are on top of stack now: continue attending them
			}
			// RARE events are checked first thing in next iteration
		}
	}
	// Return any Traial still on the loose
	tpool.return_traials(stack);

	// To estimate, weigh each count by the relative importance of the
	// threshold level it belongs to. We do that here in an "upscale fashion",
	// which must be balanced in the ConfidenceInterval update.
	double weighedRaresCount(0.0);
	for (unsigned i = 0u ; i <= numThresholds ; i++)
		weighedRaresCount += raresCount[i]
							  * pow(splitsPerThreshold_, numThresholds-i);
	// Return the weighed count or its negative value
	assert(0.0 <= weighedRaresCount);
	if (MIN_COUNT_RARE_EVENTS > raresCount.sum())
		return -weighedRaresCount;
	else
		return  weighedRaresCount;
}


double
SimulationEngineRestart::rate_simulation(const PropertyRate& property,
										 const size_t& runLength,
										 bool reinit) const
{
	assert(0u < runLength);
	const unsigned numThresholds(impFun_->num_thresholds());
	std::valarray< CLOCK_INTERNAL_TYPE > raresCount(0.0, numThresholds+1);
	std::stack< Reference< Traial > > stack;
	auto tpool = TraialPool::get_instance();
	static Traial& originalTraial(tpool.get_traial());
//	static thread_local Traial& originalTraial(tpool.get_traial());
	simsLifetime = static_cast<CLOCK_INTERNAL_TYPE>(runLength);

	// For the sake of efficiency, distinguish when operating with a concrete ifun
	bool (SimulationEngineRestart::*watch_events)
		 (const PropertyRate&, Traial&, Event&) const;
	bool (SimulationEngineRestart::*register_time)
		 (const PropertyRate&, Traial&, Event&) const;
	if (impFun_->concrete()) {
		watch_events = &SimulationEngineRestart::rate_event_concrete;
		register_time = &SimulationEngineRestart::count_time_concrete;
	} else {
		watch_events = &SimulationEngineRestart::rate_event;
		register_time = &SimulationEngineRestart::count_time;
	}

	/// @todo TODO erase debug stuff
	size_t nufPrint(0ul);
	static unsigned call(0u);
	call++;
	trackSimulation = call > 18u;
	////////////////////////////

	// Run a single RESTART importance-splitting simulation for "runLength"
	// simulation time units and starting from the last saved state,
	// or from the system's initial state if requested.
	if (reinit || originalTraial.lifeTime == static_cast<CLOCK_INTERNAL_TYPE>(0.0)) {
		/// @todo TODO erase debug stuff
		if (call > 13u)
			std::cerr << "\nReinit\n";
		////////////////////////////
		originalTraial.initialize(*network_, *impFun_);
	} else {
		/// @todo TODO erase debug stuff
		if (call > 13u) {
			std::cerr << "\nContinue from state (";
			for (const auto& v: originalTraial.state)
				std::cerr << v << ",";
			std::cerr << "\b)[";
			for (const auto& t: originalTraial.clocks_values())
				std::cerr << t.second << ";";
			std::cerr << "\b]\n";
		}
		////////////////////////////
		originalTraial.lifeTime = 0.0;
	}
	stack.emplace(originalTraial);
	while (!stack.empty() && !interrupted) {
		Event e(EventType::NONE);
		Traial& traial = stack.top();

		/// @todo TODO erase debug stuff
		if (call > 17u) {
			auto to_values = traial.clocks_values();
			auto min = std::min(to_values[0].second, to_values[1].second);
			min = std::min(min, to_values[2].second);
			if (min < -3.0) {
				std::cerr << "[";
				for (const auto& v: to_values)
					std::cerr << v.second << ";";
				std::cerr << "\b] ???\n";
				exit(EXIT_FAILURE);
			}
		}
		//////////////////////////////

		// Check whether we're standing on a rare event
		(this->*watch_events)(property, traial, e);
		if (IS_RARE_EVENT(e)) {
			// We are? Then register rare time
			const CLOCK_INTERNAL_TYPE simLength(traial.lifeTime);  // reduce fp prec. loss
			traial.lifeTime = 0.0;
			network_->simulation_step(traial, property, *this, register_time);

			/// @todo TODO erase debug stuff
			if (call > 17u && nufPrint++ < 5ul)
				std::cerr << "(" << traial.lifeTime << ")";
			////////////////////////////

			raresCount[traial.level] += traial.lifeTime;
			traial.lifeTime += simLength;
		}

		// Check where we are and whether we should do another sprint
		if (!(this->*watch_events)(property, traial, e))
			e = network_->simulation_step(traial, property, *this, watch_events);

		/// @todo TODO erase debug stuff
//		if (trackSimulation && traial.lifeTime > 1.5f)
//			exit(EXIT_FAILURE);
		////////////////////////////

		// Checking order of the following events is relevant!
		if (traial.lifeTime >= simsLifetime || IS_THR_DOWN_EVENT(e)) {

			/// @todo TODO erase debug stuff
			if (trackSimulation && IS_THR_DOWN_EVENT(e))
				std::cerr << "\nDown from " << traial.level - traial.numLevelsCrossed
						  << " into " << traial.level << std::endl;
			////////////////////////////

			// Traial reached EOS or went down => kill it
			if (&traial != &originalTraial)  // avoid future aliasing!
				tpool.return_traial(std::move(traial));
			stack.pop();

		} else if (IS_THR_UP_EVENT(e)) {

			/// @todo TODO erase debug stuff
			if (trackSimulation)
				std::cerr << "\nUp from " << traial.level - traial.numLevelsCrossed
						  << " into " << traial.level << std::endl;
			////////////////////////////

			// Could have gone up several thresholds => split accordingly
			assert(traial.numLevelsCrossed > 0);
			for (ImportanceValue i = static_cast<ImportanceValue>(1u)
				; i <= static_cast<ImportanceValue>(traial.numLevelsCrossed)
				; i++)
			{
				const unsigned thisLevelRetrials = std::round(
					(splitsPerThreshold_-1u) * pow(splitsPerThreshold_, i-1));
				assert(0u < thisLevelRetrials);
				assert(thisLevelRetrials < pow(splitsPerThreshold_, numThresholds));
				tpool.get_traial_copies(stack, traial, thisLevelRetrials,
										static_cast<short>(i)-traial.numLevelsCrossed);
			}
			// Offsprings are on top of stack now: continue attending them
		}
		// RARE events are checked first thing in next iteration
	}
	assert(originalTraial.lifeTime != 0.0);  // allow next iteration of batch means
	// Return any Traial still on the loose
	const size_t numLooseTraials(stack.size());
	for (size_t i = 0ul ; i < numLooseTraials ; i++) {
		Traial& traial = stack.top();
		if (&traial != &originalTraial) {  // avoid future aliasing!
			tpool.return_traial(std::move(traial));
			stack.pop();
		}
	}

	// To estimate, weigh counts by the relative importance of their thresholds
	double accTime(0.0);
	for (unsigned i = 0u ; i <= numThresholds ; i++)
		accTime += raresCount[i] / pow(splitsPerThreshold_, i);

	/// @todo TODO erase debug stuff
	if (call > 17u) {
		std::cerr << "\nRare time: " << raresCount.sum() << std::endl;
		std::cerr << "Weighed rare time: " << accTime << std::endl;
	}
	////////////////////////////

	/// @todo TODO erase debug stuff
	auto to_values = originalTraial.clocks_values();
	auto min = std::min(to_values[0].second, to_values[1].second);
	min = std::min(min, to_values[2].second);
	if (min < -3.0) {
		std::cerr << "[";
		for (const auto& v: to_values)
			std::cerr << v.second << ";";
		std::cerr << "\b] ???\n";
		exit(EXIT_FAILURE);
	}
	//////////////////////////////

	// Return estimate or its negative value
	assert(0.0 <= accTime);
	if (MIN_ACC_RARE_TIME > raresCount.sum())
		return -accTime / static_cast<double>(runLength);
	else
		return  accTime / static_cast<double>(runLength);
}

} // namespace fig
