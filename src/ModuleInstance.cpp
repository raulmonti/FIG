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
#include <string>
#include <algorithm>  // find_if()
#include <unordered_map>
// C
#include <cassert>
// FIG
#include <FigException.h>
#include <ModuleInstance.h>


namespace fig
{

const State< STATE_INTERNAL_TYPE >&
ModuleInstance::mark_added(const int& globalIndex, const int& firstClock)
{
	assert(0 <= globalIndex);
	assert(0 <= firstClock);
	if (0 <= globalIndex_ || 0 <= firstClock_)
#ifndef NDEBUG
		throw FigException("this module has already been added to the network");
#else
		return;
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
	// Map our clocks to their global positions
	std::unordered_map< std::string, unsigned > localClocks;
	localClocks.reserve(lClocks_.size());
	unsigned clockGlobalPos = firstClock;
	for (const auto& clk: lClocks_)
		localClocks[clk.name] = clockGlobalPos++;
	// Callback all our transitions
	for (auto& pair: transitions_by_label_)
		for (auto& tr_ptr: pair.second)
			tr_ptr->callback(localClocks, globalVars);
	localClocks.clear();
}


void
ModuleInstance::seal(
	std::function<size_t(const fig::State&,const std::string&)> posOfVar,
	const fig::State& globalState)
{
	/// @todo TODO implement, needs version of Transition::callback()
	///       which accepts std::function<>
}


void
ModuleInstance::add_transition(const Transition& transition)
{
#ifndef NDEBUG
	if (0 <= globalIndex_ || 0 <= firstClock_)
		throw FigException("this module has already been added to the network");
	if (!is_our_clock(transition.triggeringClock()))
		throw FigException(std::string("triggering clock \"")
						   .append(transition.triggeringClock())
						   .append("\" does not reside in module \"")
						   .append(name).append("\""));
	for (const auto& clockName: transition.resetClocksList())
		if (!is_our_clock(clockName))
			throw FigException(std::string("reset clock \"").append(clockName)
							   .append("\" does not reside in module \"")
							   .append(name).append("\""));
#else
	if (0 <= globalIndex_ || 0 <= firstClock_)
		return;
#endif
	auto ptr = std::make_shared<Transition>(transition);
	transitions_by_label_[transition.label().str].emplace_back(ptr);
	transitions_by_clock_[transition.triggeringClock()].emplace_back(ptr);
}


void
ModuleInstance::add_transition(Transition&& transition)
{
#ifndef NDEBUG
	if (0 <= globalIndex_ || 0 <= firstClock_)
		throw FigException("this module has already been added to the network");
	if (!is_our_clock(transition.triggeringClock()))
		throw FigException(std::string("triggering clock \"")
						   .append(transition.triggeringClock())
						   .append("\" does not reside in module \"")
						   .append(name).append("\""));
	for (const auto& clockName: transition.resetClocksList())
		if (!is_our_clock(clockName))
			throw FigException(std::string("reset clock \"").append(clockName)
							   .append("\" does not reside in module \"")
							   .append(name).append("\""));
#else
	if (0 <= globalIndex_ || 0 <= firstClock_)
		return;
#endif
	auto ptr = std::make_shared<Transition>(std::forward<Transition>(transition));
	// shared_ptr from rvalue: http://stackoverflow.com/q/15917475
	transitions_by_label_[transition.label().str].emplace_back(ptr);
	transitions_by_clock_[transition.triggeringClock()].emplace_back(ptr);
}


const Label&
ModuleInstance::jump(const std::string& clockName,
					 const CLOCK_INTERNAL_TYPE& elapsedTime,
					 Traial& traial)
{
	auto transitions = transitions_by_clock_[clockName];
	for(auto& tr_ptr: transitions) {
		if (tr_ptr->pre(traial.state)) { // If the traial satisfies this precondition
			tr_ptr->pos(traial.state);   // apply postcondition to its state
			tr_ptr->handle_clocks(       // and update all our clocks.
				traial,
				lClocks_.begin(),
				lClocks_.end(),
				firstClock_,
				elapsedTime);
			// Finally notify the broadcasted output label
			assert(tr_ptr->label().is_output());
			return tr_ptr->label();
		}
	}
	return std::move(Label());  // No transition triggered => broadcast tau
}


void
ModuleInstance::jump(const Label& label,
					 const CLOCK_INTERNAL_TYPE& elapsedTime,
					 Traial& traial)
{
	assert(label.is_output());
	auto transitions = transitions_by_label_[label.str];
	for(auto& tr_ptr: transitions) {
		if (tr_ptr->pre(traial.state)) { // If the traial satisfies this precondition
			tr_ptr->pos(traial.state);   // apply postcondition to its state
			tr_ptr->handle_clocks(       // and update all our clocks.
				traial,
				lClocks_.begin(),
				lClocks_.end(),
				firstClock_,
				elapsedTime);
			break;  // Only one transition could've been enabled, we trust Raúl
		}
	}
}

} // namespace fig
