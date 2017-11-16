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
#include <algorithm>  // std::fill
// FIG
#include <SimulationEngineRestart.h>
#include <ModuleNetwork.h>
#include <TraialPool.h>
#include <FigException.h>
#include <FigLog.h>

// ADL
using std::begin;
using std::end;
using std::pow;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

// Available engine names in SimulationEngine::names
SimulationEngineRestart::SimulationEngineRestart(
	std::shared_ptr<const ModuleNetwork> network,
	const unsigned& splitsPerThreshold,
	const unsigned& dieOutDepth) :
		SimulationEngine("restart", network),
		splitsPerThreshold_(splitsPerThreshold),
		dieOutDepth_(dieOutDepth),
        numChunksTruncated_(0u),
        oTraial_(TraialPool::get_instance().get_traial())
{ /* Not much to do around here */ }


SimulationEngineRestart::~SimulationEngineRestart()
{
	TraialPool::get_instance().return_traials(stack_);
}


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
	if (ifun_ptr->strategy() == "")
		throw_FigException("ImportanceFunction doesn't seem to have "
						   "internal importance information");
	else if (ifun_ptr->strategy() == "flat")
		throw_FigException("RESTART simulation engine requires an importance "
						   "building strategy other than \"flat\"");
	reinit_stack();
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


void
SimulationEngineRestart::reinit_stack() const
{
	static auto tpool = TraialPool::get_instance();
	while (!stack_.empty()) {
		if (&stack_.top().get() != &oTraial_)  // avoid future aliasing!
			tpool.return_traial(std::move(stack_.top()));
		stack_.pop();
	}
	stack_.push(oTraial_);
}


double
SimulationEngineRestart::log_experiments_per_sim() const
{
	if (!bound())
		throw_FigException("engine isn't bound to any importance function");
	// log( splitsPerThreshold ^ numThresholds )
	return impFun_->num_thresholds() * std::log(splitsPerThreshold_);
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
	auto tpool = TraialPool::get_instance();

	// For the sake of efficiency, distinguish when operating with a concrete ifun
	bool (SimulationEngineRestart::*watch_events)
		 (const PropertyTransient&, Traial&, Event&) const;
	if (impFun_->concrete_simulation())
		watch_events = &SimulationEngineRestart::transient_event_concrete;
	else
		watch_events = &SimulationEngineRestart::transient_event;

	// Perform 'numRuns' independent RESTART simulations
	for (size_t i = 0ul ; i < numRuns && !interrupted ; i++) {

		std::fill(begin(raresCount), end(raresCount), 0u);  // reset counts
		tpool.get_traials(stack, 1u);
		stack.top().get().initialise(*network_, *impFun_);

		// RESTART importance-splitting simulation:
		while (!stack.empty() && !interrupted) {
			Event e(EventType::NONE);
			Traial& traial = stack.top();

			// Check whether we're standing on a rare event first
			(this->*watch_events)(property, traial, e);
			if (IS_RARE_EVENT(e)) {
				// We are? Then count and kill
				assert(static_cast<ImportanceValue>(0) < traial.level
					   || impFun_->strategy() == "adhoc");
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
//				tpool.return_traial(stack.top());  // alternative way
				stack.pop();

			} else if (IS_THR_UP_EVENT(e)) {
				// Could have gone up several thresholds => split accordingly
				assert(traial.numLevelsCrossed > 0);
				unsigned long prevEffort(1ul), currEffort(1ul);
				for (short i = 0 ; i < traial.numLevelsCrossed ; i++) {
					prevEffort *= currEffort;
					currEffort = impFun_->effort_of(traial.level+i);
					assert(1ul < currEffort);
					tpool.get_traial_copies(stack_, traial, prevEffort*(currEffort-1),
					                        i+1-traial.numLevelsCrossed);
				}
				assert(&(stack_.top().get()) != &oTraial_);
//				for (ImportanceValue i = static_cast<ImportanceValue>(1u)
//					; i <= static_cast<ImportanceValue>(traial.numLevelsCrossed)
//					; i++)
//				{
//					const unsigned thisLevelRetrials = std::round(
//						(splitsPerThreshold_-1u) * pow(splitsPerThreshold_, i-1));
//					assert(0u < thisLevelRetrials);
//					assert(thisLevelRetrials < pow(splitsPerThreshold_, numThresholds));
//					tpool.get_traial_copies(stack, traial, thisLevelRetrials,
//											static_cast<short>(i)-traial.numLevelsCrossed);
//				}
				// Offsprings are on top of stack now: continue attending them
			}
			// RARE events are checked first thing in next iteration
		}

		// Save weighed RE counts of this run, downscaling the # of RE observed
		// by the relative importance of the threshold level they belong to
		weighedRaresCount.push_back(0.0);
		for (int t = 0u ; t <= (int)numThresholds ; t++)
			weighedRaresCount[i] += raresCount[t] * pow(splitsPerThreshold_, -t);
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
	auto tpool = TraialPool::get_instance();
	simsLifetime = static_cast<CLOCK_INTERNAL_TYPE>(runLength);
	numChunksTruncated_ = 0u;

	// Reset batch or run with batch means?
	if (reinit || stack_.empty()) {
		reinit_stack();
		assert(&oTraial_ == &stack_.top().get());
		oTraial_.initialise(*network_, *impFun_);
	} else {
		// Batch means, but reset life times
		std::stack< Reference< Traial > > tmp;
		while (!stack_.empty()) {
			stack_.top().get().lifeTime = 0.0;
			tmp.push(std::move(stack_.top()));
			stack_.pop();
		}
		while (!tmp.empty()) {
			stack_.push(std::move(tmp.top()));
			tmp.pop();
		}
	}

	// For the sake of efficiency, distinguish when operating with a concrete ifun
	bool (SimulationEngineRestart::*watch_events)
		 (const PropertyRate&, Traial&, Event&) const;
	bool (SimulationEngineRestart::*register_time)
		 (const PropertyRate&, Traial&, Event&) const;
	if (impFun_->concrete_simulation()) {
		watch_events = &SimulationEngineRestart::rate_event_concrete;
		register_time = &SimulationEngineRestart::count_time_concrete;
	} else {
		watch_events = &SimulationEngineRestart::rate_event;
		register_time = &SimulationEngineRestart::count_time;
	}

	// Run a single RESTART importance-splitting simulation for "runLength"
	// simulation time units and starting from the last saved stack_,
	// or from the system's initial state if requested.
	while (!stack_.empty() && !interrupted) {
		Event e(EventType::NONE);
		Traial& traial = stack_.top();
		assert(&traial != &oTraial_ || stack_.size() == 1ul);

		// Check whether we're standing on a rare event
		(this->*watch_events)(property, traial, e);
		if (IS_RARE_EVENT(e)) {
			// We are? Then register rare time
			assert(impFun_->importance_of(traial.state) > static_cast<ImportanceValue>(0)
				   || impFun_->strategy() == "adhoc");
			const CLOCK_INTERNAL_TYPE simLength(traial.lifeTime);  // reduce fp prec. loss
			traial.lifeTime = static_cast<CLOCK_INTERNAL_TYPE>(0.0);
			network_->simulation_step(traial, property, *this, register_time);
			assert(static_cast<CLOCK_INTERNAL_TYPE>(0.0) < traial.lifeTime);
			raresCount[traial.level] += traial.lifeTime;
			traial.lifeTime += simLength;
		}

		// Check where are we and whether we should do another sprint
		if (!(this->*watch_events)(property, traial, e))
			e = network_->simulation_step(traial, property, *this, watch_events);

		// Checking order of the following events is relevant!
		if (traial.lifeTime > simsLifetime || IS_THR_DOWN_EVENT(e)) {
			// Traial reached EOS or went down => kill it
			assert(!(&traial==&oTraial_ && IS_THR_DOWN_EVENT(e)));
			if (&traial != &oTraial_)  // avoid future aliasing!
				tpool.return_traial(std::move(stack_.top()));
			stack_.pop();
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
			assert(0 < traial.numLevelsCrossed);
			unsigned long prevEffort(1ul), currEffort(1ul);
			for (short i = 0 ; i < traial.numLevelsCrossed ; i++) {
				prevEffort *= currEffort;
				currEffort = impFun_->effort_of(traial.level+i);
				assert(1ul < currEffort);
				tpool.get_traial_copies(stack_, traial, prevEffort*(currEffort-1),
				                        i+1-traial.numLevelsCrossed);
			}
			assert(&(stack_.top().get()) != &oTraial_);
//			for (ImportanceValue i = static_cast<ImportanceValue>(1u)
//				; i <= static_cast<ImportanceValue>(traial.numLevelsCrossed)
//				; i++)
//			{
//				const unsigned thisLevelRetrials =
//
//				const unsigned thisLevelRetrials = std::round(
//					(splitsPerThreshold_-1u) * pow(splitsPerThreshold_, i-1));
//				assert(0u < thisLevelRetrials);
//				assert(thisLevelRetrials < pow(splitsPerThreshold_, numThresholds));
//				tpool.get_traial_copies(stack_, traial, thisLevelRetrials,
//										static_cast<short>(i)-traial.numLevelsCrossed);
//			}
			// Offsprings are on top of stack_ now: continue attending them
		}
		// RARE events are checked first thing in next iteration
	}
	if (stack_.empty())  // allow next iteration of batch means
		stack_.push(oTraial_);

	// To estimate, weigh times by the relative importance of their thresholds
	double weighedAccTime(0.0);
	for (int t = 0 ; t <= (int)numThresholds ; t++)
		weighedAccTime += raresCount[t] * pow(splitsPerThreshold_, -t);
	// Return the (weighed) simulation-time spent on rare states
	assert(0.0 <= weighedAccTime);
	return weighedAccTime;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
