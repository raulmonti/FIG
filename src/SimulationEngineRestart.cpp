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
#include <deque>
#include <iterator>
#include <algorithm>   // std::fill
#include <functional>  // std::functional<>
// FIG
#include <SimulationEngineRestart.h>
#include <ModuleNetwork.h>
#include <TraialPool.h>
#include <FigException.h>
#include <FigLog.h>


using namespace std::placeholders;  // _1, _2, _3, ...


// ADL
using std::begin;
using std::end;
using std::pow;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

// Available engine names in SimulationEngine::names
SimulationEngineRestart::SimulationEngineRestart(
    std::shared_ptr<const ModuleNetwork> model,
    const unsigned& dieOutDepth) :
        SimulationEngine("restart", model),
//        splitsPerThreshold_(splitsPerThreshold),
		dieOutDepth_(dieOutDepth),
        numChunksTruncated_(0u),
        oTraial_(TraialPool::get_instance().get_traial())
{ /* Not much to do around here */ }


SimulationEngineRestart::~SimulationEngineRestart()
{
	//TraialPool::get_instance().return_traials(ssstack_);
	// ^^^ pointless, and besides the TraialPool might be dead already,
    //     so this would trigger a re-creation of the pool or something worse
}


//	unsigned
//	SimulationEngineRestart::global_effort() const noexcept
//	{
//		return splitsPerThreshold_;
//	}


const unsigned&
SimulationEngineRestart::die_out_depth() const noexcept
{
	return dieOutDepth_;
}


void
SimulationEngineRestart::bind(std::shared_ptr< const ImportanceFunction > ifun_ptr)
{
	if (ifun_ptr->strategy() == "")
		throw_FigException("ImportanceFunction doesn't seem to have "
						   "internal importance information");
	else if (ifun_ptr->strategy() == "flat")
		throw_FigException("RESTART simulation engine requires an importance "
						   "building strategy other than \"flat\"");
	reinit_stack();
	SimulationEngine::bind(ifun_ptr);
}


//	void
//	SimulationEngineRestart::set_global_effort(unsigned spt)
//	{
//	    if (locked())
//	        throw_FigException("engine \"" + name() + "\" is currently locked "
//	                           "in \"simulation mode\"");
//		if (spt < 2u)
//			throw_FigException("bad global splitting value \"" + std::to_string(spt) + "\". "
//							   "At least one Traial must be created, besides the "
//							   "original one, when crossing a threshold upwards");
//	    splitsPerThreshold_ = spt;
//	}


void
SimulationEngineRestart::set_die_out_depth(unsigned dieOutDepth)
{
    if (locked())
        throw_FigException("engine \"" + name() + "\" is currently locked "
                           "in \"simulation mode\"");
    dieOutDepth_ = dieOutDepth;
}


void
SimulationEngineRestart::reinit_stack() const
{
	static TraialPool& tpool(TraialPool::get_instance());
	while (!ssstack_.empty()) {
		if (&ssstack_.top().get() != &oTraial_)  // avoid future aliasing!
			tpool.return_traial(std::move(ssstack_.top()));
		ssstack_.pop();
	}
	ssstack_.push(oTraial_);
}


void
SimulationEngineRestart::handle_lvl_up(
    const Traial& traial,
    std::stack< Reference < Traial > >& stack) const
{
	static TraialPool& tpool(TraialPool::get_instance());
	const auto previousLvl(static_cast<short>(traial.level)-traial.numLevelsCrossed);
	unsigned long prevEffort(1ul), currEffort(1ul);

	assert(0 < traial.level);
	assert(0 < traial.numLevelsCrossed);
	assert(0 <= previousLvl);

	// Could have gone up several thresholds => split accordingly
	for (short i = 1 ; i <= traial.numLevelsCrossed ; i++) {
		assert(impFun_->max_value() >= static_cast<ImportanceValue>(previousLvl+i));
		prevEffort *= currEffort;
		currEffort = impFun_->effort_of(previousLvl+i);
		assert(1ul < currEffort);
		tpool.get_traial_copies(stack,
		                        traial,
		                        prevEffort*(currEffort-1),
		                        i-traial.numLevelsCrossed);
	}
}


std::vector<double>
SimulationEngineRestart::transient_simulations(const PropertyTransient& property,
											   const size_t& numRuns) const
{
	assert(0u < numRuns);
	const unsigned numThresholds(impFun_->num_thresholds());
	std::vector< unsigned > raresCount(numThresholds+1, 0u);
	std::vector< double > weighedRaresCount;
	weighedRaresCount.reserve(numRuns);
	std::stack< Reference< Traial > > stack;
	static TraialPool& tpool(TraialPool::get_instance());

	if (reachCount_.size() != numThresholds+1)
		reachCount_.clear();

	// For the sake of efficiency, distinguish when operating with a concrete ifun
//	auto watch_events = impFun_->concrete_simulation()
	EventWatcher watch_events = impFun_->concrete_simulation()
			? std::bind(&SimulationEngineRestart::transient_event_concrete, *this, _1, _2, _3)
			: std::bind(&SimulationEngineRestart::transient_event,          *this, _1, _2, _3);
//	bool (SimulationEngineRestart::*watch_events)
//	     (const PropertyTransient&, Traial&, Event&) const;
//	if (impFun_->concrete_simulation())
//		watch_events = &SimulationEngineRestart::transient_event_concrete;
//	else
//		watch_events = &SimulationEngineRestart::transient_event;

	// Perform 'numRuns' independent RESTART simulations
	for (size_t i = 0ul ; i < numRuns && !interrupted ; i++) {

		std::fill(begin(raresCount), end(raresCount), 0u);  // reset counts
		tpool.get_traials(stack, 1u);
		stack.top().get().initialise(*model_, *impFun_);

		// RESTART importance-splitting simulation:
		while (!stack.empty() && !interrupted) {
			Event e(EventType::NONE);
			Traial& traial = stack.top();
			assert(traial.level <= numThresholds);
			reachCount_[traial.level]++;

			// Check whether we're standing on a rare event first
//			(this->*watch_events)(property, traial, e);
			watch_events(property, traial, e);
			if (IS_RARE_EVENT(e)) {
				// We are? Then count and kill
//				assert(static_cast<ImportanceValue>(0) < traial.level || impFun_->strategy() == "adhoc");
				raresCount[traial.level]++;
				tpool.return_traial(std::move(traial));
				stack.pop();
				continue;
			}
			// We aren't? Then keep dancing
//			e = model_->simulation_step(traial, property, *this, watch_events);
			e = model_->simulation_step(traial, property, watch_events);

			// The following events are treated as mutually exclusive
			// Checking order is relevant!
			if (IS_STOP_EVENT(e) || IS_THR_DOWN_EVENT(e)) {
				// Traial reached a stop event or went down => kill it
				tpool.return_traial(std::move(traial));  // alternative way: tpool.return_traial(stack.top());
				stack.pop();

			} else if (IS_THR_UP_EVENT(e)) {
				// Could have gone up several thresholds => split accordingly
				handle_lvl_up(traial, stack);
				// Offsprings are on top of stack now: continue attending them
			}
			// RARE events are checked first thing in next iteration
		}

		// Save weighed RE counts of this run, downscaling the # of RE observed
		// by the relative importance of the threshold level they belong to
		weighedRaresCount.push_back(0.0);
		double effort(1.0);
		for (int t = 0u ; t <= (int)numThresholds ; t++) {
			effort *= impFun_->effort_of(t);
			weighedRaresCount[i] += raresCount[t] / effort;
		}
		assert(!std::isnan(weighedRaresCount.back()));
		assert(!std::isinf(weighedRaresCount.back()));
		assert(0.0 <= weighedRaresCount.back());
	}
	// Return any Traial still on the loose
	tpool.return_traials(stack);

	return weighedRaresCount;
}


double
SimulationEngineRestart::rate_simulation(const PropertyRate& property,
										 const size_t& runLength,
										 bool reinit) const
{
	assert(0u < runLength);
	const unsigned numThresholds(impFun_->num_thresholds());
	std::vector< double > raresCount(numThresholds+1, 0.0);
	static TraialPool& tpool(TraialPool::get_instance());

	simsLifetime = static_cast<CLOCK_INTERNAL_TYPE>(runLength);
	numChunksTruncated_ = 0u;
	if (reachCount_.size() != numThresholds+1 || reinit)
		reachCount_.clear();

	// Reset batch or run with batch means?
	if (reinit || ssstack_.empty()) {
		reinit_stack();
		assert(ssstack_.size() == 1ul);
		assert(&oTraial_ == &ssstack_.top().get());
		oTraial_.initialise(*model_, *impFun_);
	} else {
		// Batch means, but reset life times
		std::stack< Reference< Traial > > tmp;
		const auto numTraialsBatchMeans(ssstack_.size());
		while (!ssstack_.empty()) {
			ssstack_.top().get().lifeTime = 0.0;
			tmp.push(std::move(ssstack_.top()));
			ssstack_.pop();
			assert(tmp.size()+ssstack_.size() == numTraialsBatchMeans);
		}
		while (!tmp.empty()) {
			ssstack_.push(std::move(tmp.top()));
			tmp.pop();
			assert(tmp.size()+ssstack_.size() == numTraialsBatchMeans);
		}
	}

	// For the sake of efficiency, distinguish when operating with a concrete ifun
//	auto watch_events = impFun_->concrete_simulation()
	const EventWatcher& watch_events = impFun_->concrete_simulation()
			? std::bind(&SimulationEngineRestart::rate_event_concrete, this, _1, _2, _3)
			: std::bind(&SimulationEngineRestart::rate_event,          this, _1, _2, _3);
//	auto register_time = impFun_->concrete_simulation()
	const EventWatcher& register_time = impFun_->concrete_simulation()
			? std::bind(&SimulationEngineRestart::count_time_concrete, this, _1, _2, _3)
			: std::bind(&SimulationEngineRestart::count_time,          this, _1, _2, _3);
//	bool (SimulationEngineRestart::*watch_events)
//		 (const PropertyRate&, Traial&, Event&) const;
//	bool (SimulationEngineRestart::*register_time)
//		 (const PropertyRate&, Traial&, Event&) const;
//	if (impFun_->concrete_simulation()) {
//		watch_events = &SimulationEngineRestart::rate_event_concrete;
//		register_time = &SimulationEngineRestart::count_time_concrete;
//	} else {
//		watch_events = &SimulationEngineRestart::rate_event;
//		register_time = &SimulationEngineRestart::count_time;
//	}

	// Run a single RESTART importance-splitting simulation for "runLength"
	// simulation time units and starting from the last saved ssstack_,
	// or from the system's initial state if requested.
	while (!ssstack_.empty() && !interrupted) {
		Event e(EventType::NONE);
		Traial& traial = ssstack_.top();
		assert(traial.level <= numThresholds);
		reachCount_[traial.level]++;
#ifndef NDEBUG
		if (&traial == &oTraial_ && ssstack_.size() > 1ul) {
			std::stringstream errMsg;
			errMsg << "[ERROR] Aliasing!!! Traials stack size: " << ssstack_.size()
			       << " | State of \"original Traial\" from batch means: ";
			oTraial_.print_out(errMsg);
			throw_FigException(errMsg.str());
		}
#endif

		// Check whether we're standing on a rare event
//		(this->*watch_events)(property, traial, e);
		watch_events(property, traial, e);
		if (IS_RARE_EVENT(e)) {
			// We are? Then register rare time
			assert(impFun_->importance_of(traial.state) > static_cast<ImportanceValue>(0)
				   || impFun_->strategy() == "adhoc");
			const CLOCK_INTERNAL_TYPE simLength(traial.lifeTime);  // reduce fp prec. loss
			traial.lifeTime = static_cast<CLOCK_INTERNAL_TYPE>(0.0);
//			model_->simulation_step(traial, property, *this, register_time);
			model_->simulation_step(traial, property, register_time);
			assert(static_cast<CLOCK_INTERNAL_TYPE>(0.0) < traial.lifeTime);
			raresCount[traial.level] += traial.lifeTime;
			traial.lifeTime += simLength;
		}

		// Check where are we and whether we should do another sprint
//		if (!(this->*watch_events)(property, traial, e))
		if (!watch_events(property, traial, e))
//			e = model_->simulation_step(traial, property, *this, watch_events);
			e = model_->simulation_step(traial, property, watch_events);

		// Checking order of the following events is relevant!
		if (traial.lifeTime > simsLifetime || IS_THR_DOWN_EVENT(e)) {
			// Traial reached EOS or went down => kill it
			assert(!(&traial==&oTraial_ && IS_THR_DOWN_EVENT(e)));
			if (&traial != &oTraial_)  // avoid future aliasing!
				tpool.return_traial(std::move(traial));
			ssstack_.pop();
			// Revert any time truncation
			if (0u < numChunksTruncated_) {
				simsLifetime += numChunksTruncated_ * SIM_TIME_CHUNK;
				numChunksTruncated_ = 0u;
			}

		} else if (IS_THR_UP_EVENT(e)) {
			// Revert any time truncation
			if (0u < numChunksTruncated_) {
				simsLifetime += numChunksTruncated_ * SIM_TIME_CHUNK;
				traial.lifeTime += numChunksTruncated_ * SIM_TIME_CHUNK;
				numChunksTruncated_ = 0u;
			}
			// Could have gone up several thresholds => split accordingly
			handle_lvl_up(traial, ssstack_);
			assert(&(ssstack_.top().get()) != &oTraial_);
			// Offsprings are on top of ssstack_ now: continue attending them
		}
		// RARE events are checked first thing in next iteration
	}
	if (ssstack_.empty())  // allow next iteration of batch means
		ssstack_.push(oTraial_);

	// To estimate, weigh times by the relative importance of their thresholds
	double weighedAccTime(0.0);
	unsigned long effort(1ul);
	for (int t = 0 ; t <= (int)numThresholds ; t++) {
		effort *= impFun_->effort_of(t);
		weighedAccTime += raresCount[t] / effort;
	}
	// Return the (weighed) simulation-time spent on rare states
	assert(0.0 <= weighedAccTime);
	return weighedAccTime;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
