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
// Support for committed actions: Leonardo Rodríguez.

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
#include <iostream>
#include <sstream>
// C
#include <cassert>
// FIG
#include <FigException.h>
#include <ModuleInstance.h>
#include <ModelSuite.h>
#include <ImportanceFunction.h>
#include <Traial.h>


// ADL
using std::begin;
using std::end;


namespace { const fig::Label NoLabel = fig::Label::make_ignored();}


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

bool ModuleInstance::highVerbosity = ModelSuite::get_verbosity();


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
	markovian_check();
}

// ModuleInstance can be built from the following containers
using str = const std::string&;
using st  = const State<STATE_INTERNAL_TYPE>&;
template ModuleInstance::ModuleInstance(str, st, const std::set<Clock>&);
template ModuleInstance::ModuleInstance(str, st, const std::list<Clock>&);
template ModuleInstance::ModuleInstance(str, st, const std::deque<Clock>&);
template ModuleInstance::ModuleInstance(str, st, const std::vector<Clock>&);
template ModuleInstance::ModuleInstance(str, st, const std::forward_list<Clock>&);


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
	transitions_.emplace_back(transition);
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
	transitions_.emplace_back(transition);
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
using lab = const Label&;
using str = const std::string&;
using pre = const Precondition&;
using pos = const Postcondition&;
template void ModuleInstance::add_transition(lab, str, pre, pos, const std::set< std::string >&);
template void ModuleInstance::add_transition(lab, str, pre, pos, const std::list< std::string >&);
template void ModuleInstance::add_transition(lab, str, pre, pos, const std::deque< std::string >&);
template void ModuleInstance::add_transition(lab, str, pre, pos, const std::vector< std::string >&);
template void ModuleInstance::add_transition(lab, str, pre, pos, const std::forward_list< std::string >&);
template void ModuleInstance::add_transition(lab, str, pre, pos, const std::unordered_set< std::string >&);


State<STATE_INTERNAL_TYPE>
ModuleInstance::initial_state() const
{
#ifndef NDEBUG
	if (!sealed())
		throw_FigException("ModuleInstance hasn't been sealed yet");
#endif
	return lState_;
}


void
ModuleInstance::instantiate_initial_state(StateInstance& s) const
{
#ifndef NDEBUG
	if (!sealed())
		throw_FigException("ModuleInstance hasn't been sealed yet");
#endif
	if (lState_.size() != s.size())
		throw_FigException("can't copy valuation into a StateInstance of "
						   "different size than our local state");
	lState_.copy_to_state_instance(s);
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
    for (const Transition& tr: transitions_) {
	    // For each enabled transition of the module...
	    if (tr.precondition()(state)) {
		    // ...update variables...
            try{
		        tr.postcondition()(state);
            }catch(const FigException &e){
                continue;
            }
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
ModuleInstance::apply_postcondition(Traial &traial,
                                    const transition_vector_t& transitions) const
{
#ifndef NDEBUG  // Check for nondeterminism only in DEBUG mode
	const Label* labPtr(nullptr);
	StateInstance traialState = traial.state;
	for (const Transition &tr : transitions) {
		// If the traial satisfies this precondition...
		if (tr.pre(traialState)) {
			if (nullptr != labPtr
			        && !tr.label().is_in_committed()
			        && !tr.label().is_out_committed()) {
				std::stringstream errMsg;
				errMsg << "[ERROR] Nondeterminism detected in Module "
				       << name << ": Label of trans #1: "
				       << labPtr->str << " , Label of trans #2: "
				       << tr.label().str;
				throw_FigException(errMsg.str());
			} else if (nullptr != labPtr && highVerbosity) {
				figTechLog << "\n[WARNING] Nondeterminism of committed actions "
				              "detected in Module " << name
				           << ": the transition labels are \"" << labPtr->str
				           << "\" and \"" << tr.label().str << "\"\n";
				continue;  // avoid applying postcondition now! do it next time
			} else {
				labPtr = &tr.label();
			}
#else
	for (const Transition &tr : transitions) {
		// If the traial satisfies this precondition...
		if (tr.pre(traial.state)) {
#endif
			// ...apply postcondition to its state...
			tr.pos(traial.state);
			// ...and reset corresponing clocks (the other clocks aint touched)
			const size_t NUM_CLOCKS(num_clocks());
			std::vector<CLOCK_INTERNAL_TYPE> clockValues(NUM_CLOCKS);
			for (size_t i = firstClock_ ; i < firstClock_+NUM_CLOCKS ; i++ ) {
				clockValues[i-firstClock_] =
				        tr.resetClocks()[i] ? lClocks_[i-firstClock_].sample()
				                            : traial.clock_value(i);
			}
			traial.update_clocks(firstClock_, NUM_CLOCKS, clockValues);
#ifndef NDEBUG
		}
	}
	if (nullptr == labPtr)
		return ::NoLabel;  // No enabled transition found
	else
		return *labPtr;
#else
			return tr.label();  // At most one transition should've been enabled
		}
	}
	return ::NoLabel;  // No enabled transition found
#endif
}


const Label&
ModuleInstance::jump(const Traial::Timeout& to,
					 Traial& traial) const
{
#ifndef NDEBUG
	if (!sealed_)
		throw_FigException("this module hasn't been sealed yet");
#endif
	const float elapsedTime(to.value);
	const auto iter = transitions_by_clock_.find(to.name);
	assert(end(transitions_by_clock_) != iter);  // deny foreign clocks
	// Step 1: make time elapse in all clocks
	traial.advance_time(firstClock_, num_clocks(), elapsedTime);
	traial.advance_time(to.gpos, 100.0f);  // mark this clock as 'expired'
	// Step 2: attend any enabled transition with matching clock name
	return apply_postcondition(traial, iter->second);
}


void
ModuleInstance::jump(const Label& label,
					 const CLOCK_INTERNAL_TYPE& elapsedTime,
					 Traial& traial) const
{
#ifndef NDEBUG
	if (!sealed_)
		throw_FigException("this module hasn't been sealed yet");
#endif
    assert(label.is_output() || label.is_tau() || label.should_ignore());
	// Step 1: make time elapse in all clocks
	traial.advance_time(firstClock_, num_clocks(), elapsedTime);
	if (label.should_ignore())
		return;
	// Step 2: attend any enabled transition with matching input label
	const auto iter = transitions_by_label_.find(label.str);
	if (!label.is_tau() && end(transitions_by_label_) != iter) {
		const auto& transitions = iter->second;
		apply_postcondition(traial, transitions);
	// Step 3: if step 2 matched nothing, attend any enabled wildcard transition
	} else {
		const auto iter = transitions_by_label_.find("_");
		if (end(transitions_by_label_) != iter) {
			const auto &transitions = iter->second;
			apply_postcondition(traial, transitions);
		}
	}
}


const Label&
ModuleInstance::jump_committed(Traial& traial)
{
#ifndef NDEBUG
	if (!sealed_)
		throw_FigException("this module hasn't been sealed yet");
#endif
	// Look for (and apply) any enabled output committed transition
	return apply_postcondition(traial, transitions_out_committed_);
}


void
ModuleInstance::jump_committed(const Label& label, Traial& traial) const
{
#ifndef NDEBUG
	if (!sealed_)
        throw_FigException("this module hasn't been sealed yet");
#endif
	assert(label.is_out_committed() || label.should_ignore());
	if (label.should_ignore())
		return;
	// Transitions with this label ***must be committed***:
	// IOSA-C labels can have a single type (output, input, committed, tau)
    const auto iter = transitions_by_label_.find(label.str);
	if (iter != transitions_by_label_.end())
		apply_postcondition(traial, iter->second);
}


void
ModuleInstance::apply_postcondition(State<STATE_INTERNAL_TYPE>& state,
                                    const transition_vector_t& transitions) const
{
#ifndef NDEBUG
	// Check for nondeterminism only in DEBUG mode
	std::string labStr;
	typedef State<STATE_INTERNAL_TYPE> state_t;
	state_t initialState(const_cast<const state_t&>(state));
#endif
	// iterate over transitions vector
	for (const Transition &tr : transitions) {
		// If the state satisfies this precondition
#ifndef NDEBUG
		if (tr.pre(initialState)) {
#else
		if (tr.pre(state)) {
#endif
			// Apply the postcondition
			tr.pos(state);
#ifndef NDEBUG
			if (!labStr.empty()
			        && !tr.label().is_in_committed()
			        && !tr.label().is_out_committed()) {
				std::stringstream errMsg;
				errMsg << "[ERROR] Nondeterminism detected in Module "
				       << name << ": Label of trans #1: "
				       << labStr << " , Label of trans #2: "
				       << tr.label().str;
				throw_FigException(errMsg.str());
			} else if (!labStr.empty() && highVerbosity) {
				figTechLog << "\n[WARNING] Nondeterminism of committed actions "
				              "detected in Module " << name
				           << ": the transition labels are \"" << labStr
				           << "\" and \"" << tr.label().str << "\"\n";
			} else {
				labStr = tr.label().str;
			}
#else
			break;  // At most one transition should've been enabled
#endif
		}
	}
}


void
ModuleInstance::jump(const Label& label,
                     State<STATE_INTERNAL_TYPE>& state) const
{
#ifndef NDEBUG
	if (!sealed_)
		throw_FigException("this module hasn't been sealed yet");
#endif
	if (label.is_input() || label.should_ignore())
		return;  // none of our business
	const auto trans = transitions_by_label_.find(label.str);
	if (!label.is_tau() && end(transitions_by_label_) != trans) {
		apply_postcondition(state, trans->second);
	} else {
		const auto trans = transitions_by_label_.find("_");
		if (end(transitions_by_label_) != trans) {
			apply_postcondition(state, trans->second);
		}
	}
}


void
ModuleInstance::markovian_check()
{
	for (const Clock& clk: lClocks_) {
		if (clk.dist_name() != "exponential") {
			markovian_ = false;
			break;
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
#ifndef NRANGECHK
	for (Transition& tr: transitions_)
		tr.callback(localClocks, globalVars);
#else
	for (Transition& tr: transitions_)
		tr.callback(localClocks, const_cast<PositionsMap&>(globalVars));
#endif
	// Reference them by clock and by label
	order_transitions();
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
	for (Transition& tr: transitions_)
		tr.callback(localClocks, globalState);
	// Reference them by clock, by label, and by committment :p
	order_transitions();
}


void
ModuleInstance::order_transitions()
{
	if (!sealed())
#ifndef NDEBUG
		throw_FigException("this module hasn't been sealed yet");
#else
		return;
#endif

	for (const Transition& tr: transitions_) {
		transitions_by_label_[tr.label().str].emplace_back(tr);
		transitions_by_clock_[tr.triggeringClock].emplace_back(tr);
		if (tr.label().is_out_committed())
			transitions_out_committed_.emplace_back(tr);
		else if (tr.label().is_in_committed())
			transitions_in_committed_.emplace_back(tr);
	}

#ifndef NDEBUG
	for (const auto& vec: transitions_by_clock_)
		for (const Transition& tr1: vec.second)
			if (std::find_if(begin(transitions_), end(transitions_),
							 [&tr1](const Transition& tr2) { return &tr1==&tr2; })
				  == end(transitions_))
				throw_FigException("Transition with label \"" + tr1.label().str
								   + "\" and triggering clock \"" + tr1.triggeringClock
								   + "\" isn't mapped in the clocks list !!!");
	for (const auto& vec: transitions_by_label_)
		for (const Transition& tr1: vec.second)
			if (std::find_if(begin(transitions_), end(transitions_),
							 [&tr1](const Transition& tr2) { return &tr1==&tr2; })
				  == end(transitions_))
				throw_FigException("Transition with label \"" + tr1.label().str
								   + "\" and triggering clock \"" + tr1.triggeringClock
								   + "\" isn't mapped in the labels list !!!");
#endif
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
