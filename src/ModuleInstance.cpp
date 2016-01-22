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
#include <Traial.h>

// ADL
using std::begin;
using std::end;


namespace fig
{

const Label ModuleInstance::tau_;


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


const Label&
ModuleInstance::jump(const std::string& clockName,
					 const CLOCK_INTERNAL_TYPE& elapsedTime,
					 Traial& traial) const
{
#ifndef NDEBUG
	if (!sealed_)
		throw_FigException("this module hasn't been sealed yet");
#endif
//		std::cerr << name << ": jumping with " << clockName
//				  << " for " << elapsedTime << " time units" << std::endl;
	const auto iter = transitions_by_clock_.find(clockName);
	assert(end(transitions_by_clock_) != iter);  // deny foreign clocks
	const auto& transitions = iter->second;
	for (const auto& tr_ptr: transitions) {
		if (tr_ptr->pre(traial.state)) { // If the traial satisfies this precondition
			tr_ptr->pos(traial.state);   // apply postcondition to its state
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
//	std::cerr << " (*) no enabled transition\n";
	traial.kill_time(firstClock_, num_clocks(), elapsedTime);
    return ModuleInstance::tau_;
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
//		std::cerr << name << ": reacting to \"" << label.str
//				  << "\" for " << elapsedTime << " time units" << std::endl;
	assert(label.is_output());
	const auto iter = transitions_by_label_.find(label.str);
    // Foreign labels and taus won't touch us
    if (!label.is_tau() && end(transitions_by_label_) != iter) {
        const auto& transitions = iter->second;
        for (const auto& tr_ptr: transitions) {
            if (tr_ptr->pre(traial.state)) { // If the traial satisfies this precondition
                tr_ptr->pos(traial.state);   // apply postcondition to its state
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
//	std::cerr << " (*) tau or foreign label\n";
	traial.kill_time(firstClock_, num_clocks(), elapsedTime);
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
ModuleInstance::mark_added(const int& globalIndex, const int& firstClock)
{
	assert(0 <= globalIndex);
	assert(0 <= firstClock);
	if (0 <= globalIndex_ || 0 <= firstClock_)
#ifndef NDEBUG
		throw_FigException("this module has already been added to the network");
#else
		return lState_;
#endif
	globalIndex_ = globalIndex;
	firstClock_ = firstClock;
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
