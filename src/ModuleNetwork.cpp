//==============================================================================
//
//  ModuleNetwork.cpp
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


// C++
#include <iterator>   // std::begin(), std::end()
#include <algorithm>  // std::find_if()
#include <set>
#include <list>
#include <deque>
#include <vector>
#include <forward_list>
#include <unordered_set>
// FIG
#include <ModuleNetwork.h>
#include <TraialPool.h>
#include <ImportanceFunction.h>
#include <SimulationEngine.h>
#include <SimulationEngineNosplit.h>
#include <SimulationEngineRestart.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

// ADL
using std::find_if;
using std::begin;
using std::end;


namespace fig
{

ModuleNetwork::ModuleNetwork() :
	numClocks_(0u),
	sealed_(false)
{}


ModuleNetwork::ModuleNetwork(const ModuleNetwork& that) :
	gState(that.gState),
	initialClocks(that.initialClocks),
	numClocks_(that.numClocks_),
	sealed_(that.sealed_)
{
	// Efectively *copy* all modules, not just their pointers
	modules.reserve(that.modules.size());
	for (auto module_ptr: that.modules)
		modules.emplace_back(std::make_shared<ModuleInstance>(*module_ptr));
}


ModuleNetwork::~ModuleNetwork()
{
//	modules.clear();

//	Deleting this vector would be linear in its size.
//	Since the ModuleNetwork should only be deleted after simulations conclusion,
///	@warning we ignore this (potential?) memory leak due to its short life.
}


void
ModuleNetwork::add_module(std::shared_ptr< ModuleInstance >& module)
{
	if (sealed_)
#ifndef NDEBUG
		throw_FigException("ModuleNetwork has already been sealed");
#else
		return;
#endif
	auto state = module->mark_added(modules.size(), gState.size(), numClocks_);
	modules.push_back(module);
	gState.append(state);
	numClocks_ += module->clocks().size();
	transitions_.reserve(transitions_.size() + module->transitions_.size());
	transitions_.insert(transitions_.end(),
						module->transitions_.begin(),
						module->transitions_.end());
	module = nullptr;
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
void
ModuleNetwork::seal(const Container<ValueType, OtherContainerArgs...>& initialClocksNames)
{
	size_t numClocksReviewed(0u);
	static_assert(std::is_convertible< std::string, ValueType >::value,
				  "ERROR: type mismatch. ModuleNetwork::seal() needs "
				  "a container with the initial clock names");
	if (sealed_)
#ifndef NDEBUG
		throw_FigException("the ModuleNetwork has already been sealed");
#else
		return;
#endif
	sealed_ = true;
	initialClocks.reserve(std::distance(begin(initialClocksNames),
										end(initialClocksNames)));

	// For each module in the network...
	for (auto& module_ptr: modules) {
		// ... seal it ...
		module_ptr->seal(gState);
		// ... search for initial clocks within ...
		const auto& module_clocks = module_ptr->clocks();
		for (const auto& initClkName: initialClocksNames) {
			auto clkIter = find_if(begin(module_clocks),
								   end(module_clocks),
								   [&] (const Clock& clk) -> bool
								   { return clk.name() == initClkName; });
			// ... and if found, register position and distribution
			if (clkIter != end(module_clocks)) {
				size_t clockLocalPos = std::distance(begin(module_clocks), clkIter);
				initialClocks.emplace(numClocksReviewed + clockLocalPos, *clkIter);
			}
		}
		numClocksReviewed += module_ptr->num_clocks();
	}
	// Fill other global info
	TraialPool::numVariables = gState.size();
	TraialPool::numClocks = numClocksReviewed;
}

// ModuleNetwork::seal() can only be invoked with the following containers
template void ModuleNetwork::seal(const std::set<std::string>&);
template void ModuleNetwork::seal(const std::list<std::string>&);
template void ModuleNetwork::seal(const std::deque<std::string>&);
template void ModuleNetwork::seal(const std::vector<std::string>&);
template void ModuleNetwork::seal(const std::forward_list<std::string>&);
template void ModuleNetwork::seal(const std::unordered_set<std::string>&);


std::vector< Reference< const Clock > >
ModuleNetwork::clocks() const
{
    std::vector< Reference< const Clock > > allClocks;
    for (auto module_ptr: modules) {
        allClocks.reserve(allClocks.size() + module_ptr->num_clocks());
        for (const Clock& clk: module_ptr->clocks())
            allClocks.push_back(clk);
    }
    assert(allClocks.size() == numClocks_);
    return allClocks;
}


State<STATE_INTERNAL_TYPE>
ModuleNetwork::initial_state() const
{
#ifndef NDEBUG
	if (!sealed())
		throw_FigException("ModuleNetwork hasn't been sealed yet");
#endif
	return gState;
}


size_t
ModuleNetwork::initial_concrete_state() const
{
#ifndef NDEBUG
	if (!sealed())
		throw_FigException("ModuleNetwork hasn't been sealed yet");
#endif
	return gState.encode();
}


std::forward_list<size_t>
ModuleNetwork::adjacent_states(const size_t& s) const
{
	assert(s < concrete_state_size());
	std::forward_list<size_t> adjacentStates;
	State<STATE_INTERNAL_TYPE> state(gState);
	state.decode(s);
	for (const auto module_ptr: modules) {
		for (const auto tr_ptr: module_ptr->transitions()) {
			const Label& label = tr_ptr->label();
			// For each 'active' and enabled transition of this module...
			if (label.is_output() && tr_ptr->precondition()(state)) {
				// ...update the module variables...
				tr_ptr->postcondition()(state);
				// ...and those of other modules listening to this label...
				for (const auto other_module_ptr: modules)
					if (module_ptr->name != other_module_ptr->name)
						other_module_ptr->jump(label, state);
				// ...and store resulting concrete state
				adjacentStates.push_front(state.encode());
				// Restore original state
				state.decode(s);
			}
		}
	}
	// Remove duplicates before returning
	adjacentStates.sort();
	adjacentStates.unique();
	return adjacentStates;
}


template< typename DerivedProperty,
          class Simulator,
          class TraialMonitor >
Event ModuleNetwork::simulation_step(Traial& traial,
                                     const DerivedProperty& property,
                                     const Simulator& engine,
                                     TraialMonitor watch_events) const
{
	assert(sealed());
	Event e(EventType::NONE);

	// Jump...
	do {
		const Traial::Timeout& to = traial.next_timeout();
		const float elapsedTime(to.value);
		assert(0.0f <= elapsedTime);
        // Active jump in the module whose clock timed-out
		const Label& label = to.module->jump(to, traial);
		// Passive jumps in all modules listening to label
		for (auto module_ptr: modules)
			if (module_ptr->name != to.module->name)
				module_ptr->jump(label, elapsedTime, traial);
		traial.lifeTime += elapsedTime;
	// ...until a relevant event is observed
	} while ( !(engine.*watch_events)(property, traial, e) );

    return e;
}

/// "SimulationEngineNosplit + PropertyTransient"
/// TraialMonitor specialization
/// for "template<...> ModuleNetwork::simulation_step()"
typedef bool(SimulationEngineNosplit::*nosplit_transient_event)
	(const PropertyTransient&, Traial&, Event&) const;

/// "SimulationEngineNosplit + PropertyRate"
/// TraialMonitor specialization
/// for "template<...> ModuleNetwork::simulation_step()"
typedef bool(SimulationEngineNosplit::*nosplit_rate_event)
	(const PropertyRate&, Traial&, Event&) const;

/// "SimulationEngineRestart + PropertyTransient"
/// TraialMonitor specialization
/// for "template<...> ModuleNetwork::simulation_step()"
typedef bool(SimulationEngineRestart::*restart_transient_event)
    (const PropertyTransient&, Traial&, Event&) const;

/// "SimulationEngineRestart + PropertyRate"
/// TraialMonitor specialization
/// for "template<...> ModuleNetwork::simulation_step()"
typedef bool(SimulationEngineRestart::*restart_rate_event)
	(const PropertyRate&, Traial&, Event&) const;

// ModuleNetwork::simulation_step() can only be invoked with the following
// "DerivedProperty", "Simulator" and "TraialMonitor" combinations
template Event ModuleNetwork::simulation_step(Traial&,
											  const PropertyTransient&,
											  const SimulationEngineNosplit&,
											  nosplit_transient_event) const;
template Event ModuleNetwork::simulation_step(Traial&,
											  const PropertyRate&,
                                              const SimulationEngineNosplit&,
											  nosplit_rate_event) const;
template Event ModuleNetwork::simulation_step(Traial&,
                                              const PropertyTransient&,
                                              const SimulationEngineRestart&,
                                              restart_transient_event) const;
template Event ModuleNetwork::simulation_step(Traial&,
											  const PropertyRate&,
											  const SimulationEngineRestart&,
											  restart_rate_event) const;

} // namespace fig
