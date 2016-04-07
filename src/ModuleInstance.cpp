//==============================================================================
//
//  ModuleInstance.cpp
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
#include <set>
#include <list>
#include <deque>
#include <vector>
#include <forward_list>
#include <unordered_set>
#include <string>
#include <iterator>   // std::begin(), std::end()
#include <algorithm>  // std::find_if()
// C
#include <cassert>
// FIG
#include <FigException.h>
#include <ModuleInstance.h>
#include <ImportanceFunction.h>
#include <Traial.h>

// ADL
using std::begin;
using std::end;


namespace { const fig::Label TAU; }


namespace fig
{

template< template< typename, typename... > class Container1,
		  typename ValueType1,
		  typename... OtherContainerArgs1 >
ModuleInstance::ModuleInstance(
	const std::string& thename,
	const State< STATE_INTERNAL_TYPE >& state,
	const Container1< ValueType1, OtherContainerArgs1... >& clocks) :
		lState_(state),
		name(thename),
		globalIndex_(-1),
		firstVar_(-1),
		firstClock_(-1),
		sealed_(false)
{
	// Copy clocks
	static_assert(std::is_constructible< Clock, ValueType1 >::value,
				  "ERROR: type mismatch. ModuleInstance ctors require a "
				  "container with the clocks defined in this module");
	lClocks_.insert(begin(lClocks_), begin(clocks), end(clocks));
}

// ModuleInstance can be built from the following containers
template ModuleInstance::ModuleInstance(const std::string&,
	const State<STATE_INTERNAL_TYPE>&, const std::set<Clock>&);
template ModuleInstance::ModuleInstance(const std::string&,
	const State<STATE_INTERNAL_TYPE>&, const std::list<Clock>&);
template ModuleInstance::ModuleInstance(const std::string&,
	const State<STATE_INTERNAL_TYPE>&, const std::deque<Clock>&);
template ModuleInstance::ModuleInstance(const std::string&,
	const State<STATE_INTERNAL_TYPE>&, const std::vector<Clock>&);
template ModuleInstance::ModuleInstance(const std::string&,
	const State<STATE_INTERNAL_TYPE>&, const std::forward_list<Clock>&);


void
ModuleInstance::add_transition(const Transition& transition)
{
#ifndef NDEBUG
	if (0 <= globalIndex_ || 0 <= firstClock_)
		throw_FigException("this module has already been added to the network");
	if (!is_our_clock(transition.triggeringClock))
		throw_FigException(std::string("triggering clock \"")
						   .append(transition.triggeringClock)
						   .append("\" does not reside in module \"")
						   .append(name).append("\""));
	for (const auto& clockName: transition.resetClocksList())
		if (!is_our_clock(clockName))
			throw_FigException(std::string("reset clock \"").append(clockName)
							   .append("\" does not reside in module \"")
							   .append(name).append("\""));
#else
	if (0 <= globalIndex_ || 0 <= firstClock_)
		return;
#endif
	auto ptr = std::make_shared<Transition>(transition);
	transitions_.emplace_back(ptr);
	transitions_by_label_[transition.label().str].emplace_back(ptr);
	transitions_by_clock_[transition.triggeringClock].emplace_back(ptr);
}


void
ModuleInstance::add_transition(Transition&& transition)
{
#ifndef NDEBUG
	if (0 <= globalIndex_ || 0 <= firstClock_)
		throw_FigException("this module has already been added to the network");
	if (!is_our_clock(transition.triggeringClock))
		throw_FigException(std::string("triggering clock \"")
						   .append(transition.triggeringClock)
						   .append("\" does not reside in module \"")
						   .append(name).append("\""));
	for (const auto& clockName: transition.resetClocksList())
		if (!is_our_clock(clockName))
			throw_FigException(std::string("reset clock \"").append(clockName)
							   .append("\" does not reside in module \"")
							   .append(name).append("\""));
#else
	if (0 <= globalIndex_ || 0 <= firstClock_)
		return;
#endif
	// shared_ptr from rvalue: http://stackoverflow.com/q/15917475
	auto ptr = std::make_shared<Transition>(std::forward<Transition>(transition));
	transitions_.emplace_back(ptr);
	transitions_by_label_[transition.label().str].emplace_back(ptr);
	transitions_by_clock_[transition.triggeringClock].emplace_back(ptr);
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
void
ModuleInstance::add_transition(
	const Label& label,
	const std::string& triggeringClock,
	const Precondition& pre,
	const Postcondition& pos,
	const Container<ValueType, OtherContainerArgs...>& resetClocks)
{
	add_transition(Transition(
		std::forward<const Label&>(label),
		std::forward<const std::string&>(triggeringClock),
		std::forward<const Precondition&>(pre),
		std::forward<const Postcondition&>(pos),
		std::forward<const Container<ValueType, OtherContainerArgs...>&>(resetClocks)
	));
}

// ModuleInstance::add_transition(...) can only be invoked with the following containers
template void ModuleInstance::add_transition(const Label& label,
											 const std::string& triggeringClock,
											 const Precondition& pre,
											 const Postcondition& pos,
											 const std::set< std::string >& resetClocks);
template void ModuleInstance::add_transition(const Label& label,
											 const std::string& triggeringClock,
											 const Precondition& pre,
											 const Postcondition& pos,
											 const std::list< std::string >& resetClocks);
template void ModuleInstance::add_transition(const Label& label,
											 const std::string& triggeringClock,
											 const Precondition& pre,
											 const Postcondition& pos,
											 const std::deque< std::string >& resetClocks);
template void ModuleInstance::add_transition(const Label& label,
											 const std::string& triggeringClock,
											 const Precondition& pre,
											 const Postcondition& pos,
											 const std::vector< std::string >& resetClocks);
template void ModuleInstance::add_transition(const Label& label,
											 const std::string& triggeringClock,
											 const Precondition& pre,
											 const Postcondition& pos,
											 const std::forward_list< std::string >& resetClocks);
template void ModuleInstance::add_transition(const Label& label,
											 const std::string& triggeringClock,
											 const Precondition& pre,
											 const Postcondition& pos,
											 const std::unordered_set< std::string >& resetClocks);


State<STATE_INTERNAL_TYPE>
ModuleInstance::initial_state() const
{
#ifndef NDEBUG
	if (!sealed())
		throw_FigException("ModuleInstance hasn't been sealed yet");
#endif
	return lState_;
}


size_t
ModuleInstance::initial_concrete_state() const
{
#ifndef NDEBUG
	if (!sealed())
		throw_FigException("ModuleInstance hasn't been sealed yet");
#endif
	return lState_.encode();
}


std::forward_list<size_t>
ModuleInstance::adjacent_states(const size_t& s) const
{
	assert(s < concrete_state_size());
	std::forward_list<size_t> adjacentStates;
	State<STATE_INTERNAL_TYPE> state(lState_);
	state.decode(s);
	for (const auto tr_ptr: transitions()) {
		// For each enabled transition of the module...
		if (tr_ptr->precondition()(state)) {
			// ...update variables...
			tr_ptr->postcondition()(state);
			// ...and store resulting concrete state
			adjacentStates.push_front(state.encode());
			// Restore original state
			state.decode(s);
		}
	}
	// Remove duplicates before returning
	adjacentStates.sort();
	adjacentStates.unique();
	return adjacentStates;
}


const Label&
ModuleInstance::jump(const Traial::Timeout& to,
					 Traial& traial) const
{
#ifndef NDEBUG
	if (!sealed_)
		throw_FigException("this module hasn't been sealed yet");
#endif

	/// @todo TODO erase debug print
//	std::cerr << name << " & " << std::hex << this << " : " << clockName << std::endl;

	const float elapsedTime(to.value);
	const auto iter = transitions_by_clock_.find(to.name);
	assert(end(transitions_by_clock_) != iter);  // deny foreign clocks
	traial.kill_time(to.gpos, 1ul, 100.0f);      // mark this clock 'expired'
	const auto& transitions = iter->second;
	for (const auto& tr_ptr: transitions) {
		if (tr_ptr->pre(traial.state)) { // If the traial satisfies this precondition
			tr_ptr->pos(traial.state);   // apply postcondition to its state

			/// @todo TODO erase debug check
//			auto to_vals = traial.clocks_values();
//			for (int i = firstClock_ ; i < lClocks_.size()+firstClock_ ; i++) {
//				if (to_vals[i].first != to.name &&
//					to_vals[i].second > 0.0         &&
//					to_vals[i].second <= elapsedTime) {
//					std::cerr << "\n" << to_vals[i].first
//							  << " has " << to_vals[i].second << std::endl;
//					std::cerr << "We're jumping due to " << to.name
//							  << " which is also has " << elapsedTime << std::endl;
//					exit (EXIT_FAILURE);
//				}
//			}
			/////////////////////////////////////

            tr_ptr->handle_clocks(       // and update clocks according to it.
                traial,
                begin(lClocks_),
                end(lClocks_),
                firstClock_,
				elapsedTime);
            // Finally broadcast the output label triggered
            assert(tr_ptr->label().is_output());
			return tr_ptr->label();
		}
	}
    // No transition was enabled => advance all clocks and broadcast tau
	traial.kill_time(firstClock_, num_clocks(), elapsedTime);
	return TAU;
}


/// @todo TODO erase debug include
extern bool trackSimulation;

void
ModuleInstance::jump(const Label& label,
					 const CLOCK_INTERNAL_TYPE& elapsedTime,
					 Traial& traial) const
{
#ifndef NDEBUG
	if (!sealed_)
		throw_FigException("this module hasn't been sealed yet");
#endif
	assert(label.is_output());
	const auto iter = transitions_by_label_.find(label.str);
    // Foreign labels and taus won't touch us
    if (!label.is_tau() && end(transitions_by_label_) != iter) {
        const auto& transitions = iter->second;
        for (const auto& tr_ptr: transitions) {
            if (tr_ptr->pre(traial.state)) { // If the traial satisfies this precondition
                tr_ptr->pos(traial.state);   // apply postcondition to its state

				/// @todo TODO erase debug check
				auto to_vals = traial.clocks_values();
				for (int i = firstClock_ ; i < lClocks_.size()+firstClock_ ; i++)
					assert(to_vals[i].second <= 0.0 ||
						   to_vals[i].second > elapsedTime);
				/////////////////////////////////////

				tr_ptr->handle_clocks(       // and update clocks according to it.
                   traial,
                   begin(lClocks_),
                   end(lClocks_),
                   firstClock_,
                   elapsedTime);
                return;  // At most one transition could've been enabled, trust Raúl
            }
       }
    }
    // No transition was enabled? Then just advance all clocks
	traial.kill_time(firstClock_, num_clocks(), elapsedTime);
}


void
ModuleInstance::jump(const Label& label,
					 State<STATE_INTERNAL_TYPE>& state) const
{
#ifndef NDEBUG
	if (!sealed_)
		throw_FigException("this module hasn't been sealed yet");
#endif
	if (label.is_input() || label.is_tau())
		return;  // none of our business
	const auto trans = transitions_by_label_.find(label.str);
	if (end(transitions_by_label_) == trans)
		return;  // none of our business
	for (const auto& tr_ptr: trans->second) {
		if (tr_ptr->pre(state)) { // If the state satisfies this precondition
			tr_ptr->pos(state);   // apply the corresponding postcondition
			return;  // At most one transition could've been enabled, trust Raúl
		}
	}
}


bool
ModuleInstance::is_our_clock(const std::string& clockName) const
{
	if (clockName.empty())
		return true;
	auto clockFound = std::find_if(begin(lClocks_),
								   end(lClocks_),
								   [&] (const Clock& clk)
								   { return clockName == clk.name(); });
	return end(lClocks_) != clockFound;
}


PositionsMap
ModuleInstance::map_our_clocks() const
{
	PositionsMap localClocks;
	if (0 > firstClock_)
		// mark_added() hasn't been called yet, there's nothing we can do
		return localClocks;
	localClocks.reserve(lClocks_.size());
	unsigned clockGlobalPos = firstClock_;
	for (const auto& clk: lClocks_)
		localClocks[clk.name()] = clockGlobalPos++;
	return localClocks;
}


const State< STATE_INTERNAL_TYPE >&
ModuleInstance::mark_added(const int& globalIndex,
                           const int& firstVar,
                           const int& firstClock)
{
	assert(0 <= globalIndex);
    assert(0 <= firstVar);
    assert(0 <= firstClock);
    if (0 <= globalIndex_) {
#ifndef NDEBUG
        throw_FigException(std::string("module already added to the network at ")
                           .append("position ").append(std::to_string(globalIndex)));
#else
        std::cerr << "This module has already been added to the network "
                     " at position " << globalIndex << std::endl;
        return lState_;
#endif
    }
    globalIndex_ = globalIndex;
    firstVar_    = firstVar;
    firstClock_  = firstClock;
	return lState_;
}


void
ModuleInstance::seal(const PositionsMap& globalVars)
{
	assert(0 <= globalIndex_);
	assert(0 <= firstClock_);
	if (sealed_)
#ifndef NDEBUG
		throw_FigException("this module has already been sealed");
#else
		return;
#endif
	sealed_ = true;
	// Callback all our transitions
	auto localClocks = map_our_clocks();
	for (auto& pair: transitions_by_label_)
		for (auto& tr_ptr: pair.second)
			tr_ptr->callback(localClocks, globalVars);
}


void
ModuleInstance::seal(const fig::State<STATE_INTERNAL_TYPE>& globalState)
{
	assert(0 <= globalIndex_);
	assert(0 <= firstClock_);
	if (sealed_)
#ifndef NDEBUG
		throw_FigException("this module has already been sealed");
#else
		return;
#endif
	sealed_ = true;
	// Callback all our transitions
	auto localClocks = map_our_clocks();
	for (auto& pair: transitions_by_label_)
		for (auto& tr_ptr: pair.second)
			tr_ptr->callback(localClocks, globalState);
}

} // namespace fig
