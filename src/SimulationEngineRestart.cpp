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
#include <typeinfo>
#include <iterator>
#include <algorithm>   // std::fill
#include <functional>  // std::functional<>
// FIG
#include <SimulationEngineRestart.h>
#include <FigLog.h>
#include <FigException.h>
#include <ModuleNetwork.h>
#include <TraialPool.h>
#include <FigException.h>
#include <PropertyRate.h>
#include <PropertyTBoundSS.h>
#include <PropertyTransient.h>


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
    bool thresholds) :
        SimulationEngine("restart", model, thresholds),
        dieOutDepth_(0u),
        oTraial_(TraialPool::get_instance().get_traial()),
        currentSimLength_(0.0)
{
	if (thresholds)
		throw_FigException("RESTART engine has not yet been implemented "
		                   "to use for building thresholds");
	/* Not much to do around here */
}


SimulationEngineRestart::~SimulationEngineRestart()
{
	//TraialPool::get_instance().return_traials(ssstack_);
	// ^^^ pointless, and besides the TraialPool might be dead already,
	//	 so this would trigger a re-creation of the pool or something worse
}


const std::string&
SimulationEngineRestart::name() const noexcept
{
	static std::string name;
	name = SimulationEngine::name();
	name += die_out_depth() > 0 ? std::to_string(die_out_depth()) : std::string("");
	return name;
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
	SimulationEngine::bind(ifun_ptr);
	minImportance_ = ifun_ptr->min_value(true);
	maxImportance_ = ifun_ptr->max_value(true);
}


void
SimulationEngineRestart::unbind()
{
	minImportance_ = static_cast<ImportanceValue>(0);
	maxImportance_ = static_cast<ImportanceValue>(0);
	SimulationEngine::unbind();
}


void
SimulationEngineRestart::set_die_out_depth(unsigned dieOutDepth)
{
	if (locked())
		throw_FigException("engine \"" + name() + "\" is currently locked "
		                   "in \"simulation mode\"");
	dieOutDepth_ = static_cast<decltype(dieOutDepth_)>(dieOutDepth);
}


void
SimulationEngineRestart::reset() const
{
	SimulationEngine::reset();
	reinit_stack();
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
	TraialPool& tpool,
    std::stack< Reference < Traial > >& stack) const
{
	const decltype(traial.level)& nLvlCross = traial.numLevelsCrossed;
	const auto previousLvl = traial.level - nLvlCross;
	unsigned long prevEffort(1ul), currEffort(1ul);

	assert(0 < traial.level);
	assert(0 < nLvlCross);
	assert(traial.level >= nLvlCross);
	assert(traial.depth < 0);

	// Could have gone up several thresholds => split accordingly
	for (auto i = 1ul ; i <= nLvlCross ; i++) {
		if (static_cast<int>(previousLvl+i) < traial.nextSplitLevel)
			continue;  // skip this level
		assert(impFun_->max_value() >= static_cast<tl_type>(previousLvl+i));
		prevEffort *= currEffort;
		currEffort = impFun_->effort_of(previousLvl+i);
		assert(1ul < currEffort);
		tpool.get_traial_copies(stack,
		                        traial,
		                        static_cast<uint>(prevEffort*(currEffort-1)),
		                        static_cast<short>(i-nLvlCross));
	}
}


std::vector<double>
SimulationEngineRestart::transient_simulations(const PropertyTransient& property,
											   const size_t& numRuns) const
{
	assert(0u < numRuns);
	const unsigned numThresholds(impFun_->num_thresholds());
	std::vector< unsigned > raresCount(numThresholds+1, 0u);
	std::vector< double > weighedRaresCount(numRuns, 0.0l);
	std::stack< Reference< Traial > > stack;
	TraialPool& tpool(TraialPool::get_instance());

	if (die_out_depth() > 0)
		throw_FigException("There is no support yet for transient analysis "
		                   "using RESTART with prolonged retrials (requested "
		                   "RESTART-P" +std::to_string(die_out_depth())+
		                   ") - Aborting estimations");

	// For the sake of efficiency, distinguish when operating with a concrete ifun
	EventWatcher watch_events = impFun_->concrete_simulation()
	        ? std::bind(&SimulationEngineRestart::transient_event_concrete, this, _1, _2, _3)
	        : std::bind(&SimulationEngineRestart::transient_event,          this, _1, _2, _3);

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
			watch_events(property, traial, e);
			if (IS_RARE_EVENT(e)) {
				// We are? Then count and kill
				raresCount[traial.level]++;
				tpool.return_traial(std::move(traial));
				stack.pop();
				continue;
			}
			// We aren't? Then keep dancing
			e = model_->simulation_step(traial, property, watch_events);

			// The following events are treated as mutually exclusive
			// Checking order is relevant!
			if (IS_STOP_EVENT(e) || IS_THR_DOWN_EVENT(e)) {
				// Traial reached a stop event or went down => kill it
				tpool.return_traial(std::move(traial));  // alternative way: tpool.return_traial(stack.top());
				stack.pop();

			} else if (IS_THR_UP_EVENT(e)) {
				// Could have gone up several thresholds => split accordingly
				handle_lvl_up(traial, tpool, stack);
				traial.nextSplitLevel = static_cast<decltype(traial.nextSplitLevel)>(traial.level+1);
				// Offsprings are on top of stack now: continue attending them
			}
			// RARE events are checked first thing in next iteration
		}

		// Save weighed RE counts of this run, downscaling the # of RE observed
		// by the relative importance of the threshold level they belong to
		weighedRaresCount[i] = 0.0l;
		double effort(1.0);
		for (auto t = 0u ; t <= numThresholds ; t++) {
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
	const EventWatcher& watch_events = impFun_->concrete_simulation()
			? std::bind(&SimulationEngineRestart::rate_event_concrete, this, _1, _2, _3)
			: std::bind(&SimulationEngineRestart::rate_event,          this, _1, _2, _3);
	const EventWatcher& register_time = impFun_->concrete_simulation()
			? std::bind(&SimulationEngineRestart::count_time_concrete, this, _1, _2, _3)
			: std::bind(&SimulationEngineRestart::count_time,          this, _1, _2, _3);

	// Run a single RESTART importance-splitting simulation for "runLength"
	// simulation time units and starting from the last saved ssstack_,
	// or from the system's initial state if requested.
	simsLifetime = static_cast<CLOCK_INTERNAL_TYPE>(runLength);
	return RESTART_run(property, watch_events, register_time);
}


double
SimulationEngineRestart::tbound_ss_simulation(const PropertyTBoundSS& property) const
{
	const auto transientTime = property.tbound_low();
	const auto runLength = property.tbound_upp();
	assert(0 <= transientTime);
	assert(transientTime < runLength);

	// For the sake of efficiency, distinguish when operating with a concrete ifun
	const EventWatcher& watch_events = impFun_->concrete_simulation()
			? std::bind(&SimulationEngineRestart::rate_event_concrete, this, _1, _2, _3)
			: std::bind(&SimulationEngineRestart::rate_event,          this, _1, _2, _3);
	const EventWatcher& register_time = impFun_->concrete_simulation()
			? std::bind(&SimulationEngineRestart::count_time_concrete, this, _1, _2, _3)
			: std::bind(&SimulationEngineRestart::count_time,          this, _1, _2, _3);
	EventWatcher discard_transient =
			std::bind(&SimulationEngineRestart::kill_time,             this, _1, _2, _3);

	// Run a single RESTART simulation:
	reinit_stack();
	assert(ssstack_.size() == 1ul);
	assert(&oTraial_ == &ssstack_.top().get());
	oTraial_.initialise(*model_, *impFun_);

	// - first discard transient phase
	simsLifetime = static_cast<CLOCK_INTERNAL_TYPE>(transientTime);
	model_->simulation_step(oTraial_, property, discard_transient);
	assert(ssstack_.size() == 1ul);
	assert(&oTraial_ == &ssstack_.top().get());
	assert(oTraial_.lifeTime >= transientTime);
	if (nullptr != fig_cli::traceDump)
		(*fig_cli::traceDump) << "\n\nEnd of (user-defined) transient phase\n";

	// - and then register (time of) property satisfaction up to finishTime,
	//   using (a single run of) the RESTART importance splitting algorithm
	simsLifetime = static_cast<CLOCK_INTERNAL_TYPE>(runLength-transientTime);
	return RESTART_run(property, watch_events, register_time);
}


template< class SSProperty >
double
SimulationEngineRestart::RESTART_run(const SSProperty& property,
									 const EventWatcher& watch_events,
									 const EventWatcher& register_time) const
{
	const unsigned numThresholds(impFun_->num_thresholds());
	std::vector< double > raresCount(numThresholds+1, 0.0);
	TraialPool& tpool(TraialPool::get_instance());

	// Run a single RESTART simulation for this->simsLifeTime
	// simulation time units and starting from the last saved ssstack_
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
		watch_events(property, traial, e);
		if (IS_RARE_EVENT(e)) {
			// We are? Then register rare time
			assert(property.is_rare(traial.state));
			assert(impFun_->importance_of(traial.state) > static_cast<ImportanceValue>(0)
				   || impFun_->strategy() == "adhoc");
			const auto rareEventLevel = traial.level + (traial.depth > 0 ? traial.depth : 0);
			// ^^^ count rare-time in Traial's last going-up level
			currentSimLength_ = traial.lifeTime;  // reduce fp prec. loss
			traial.lifeTime = 0.0;
			model_->simulation_step(traial, property, register_time);
			assert(static_cast<CLOCK_INTERNAL_TYPE>(0.0) < traial.lifeTime);
			traial.lifeTime = std::min(traial.lifeTime, simsLifetime-currentSimLength_);
			raresCount[rareEventLevel] += static_cast<double>(traial.lifeTime);
			traial.lifeTime += currentSimLength_;
		}

		// Check where are we and whether we should do another sprint
		if (!watch_events(property, traial, e))
			e = model_->simulation_step(traial, property, watch_events);

		// Checking order of the following events is relevant!
		if (traial.lifeTime >= simsLifetime || IS_THR_DOWN_EVENT(e)) {
			// Traial reached EOS or went down => kill it
			assert(!(&traial==&oTraial_ && IS_THR_DOWN_EVENT(e)));
			assert(&traial != &oTraial_ || ssstack_.size() == 1ul);
			if (&traial != &oTraial_)  // avoid future aliasing!
				tpool.return_traial(std::move(traial));
			ssstack_.pop();

		} else if (IS_THR_UP_EVENT(e)) {
			// Could have gone up several thresholds => split accordingly
			assert(traial.numLevelsCrossed > 0);
			handle_lvl_up(traial, tpool, ssstack_);
			traial.nextSplitLevel = static_cast<decltype(traial.nextSplitLevel)>(traial.level+1);
			assert(&(ssstack_.top().get()) != &oTraial_);
			// Offsprings are on top of ssstack_ now: continue attending them
		}
		// RARE events are checked first thing in next iteration
	}
	if (ssstack_.empty())  // enable next iteration of batch means
		ssstack_.push(oTraial_);

	// To estimate, weigh the accumulated times by the relative importance of
	// the thresholds-levels where they were registered
	double weighedAccTime(0.0);
	unsigned long effort(1ul);
	for (auto t = 0u ; t <= numThresholds ; t++) {
		effort *= impFun_->effort_of(t);
		weighedAccTime += raresCount[t] / effort;
	}
	// Return the (weighed) simulation-time spent on rare states
	assert(0.0 <= weighedAccTime);
	return weighedAccTime;
}

// SimulationEngineRestart::RESTART_run can only be invoked
// with the following "DerivedProperties"
template
double SimulationEngineRestart::RESTART_run(const PropertyRate&, const EventWatcher&, const EventWatcher&) const;
template
double SimulationEngineRestart::RESTART_run(const PropertyTBoundSS&, const EventWatcher&, const EventWatcher&) const;

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
