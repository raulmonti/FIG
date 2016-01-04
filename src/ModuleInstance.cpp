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

void
ModuleInstance::add_transition(const Transition& transition)
{
#ifndef NDEBUG
	if (0 <= globalIndex_ || 0 <= firstClock_)
		throw FigException("this module has already been added to the network");
	if (!is_our_clock(transition.triggeringClock))
		throw FigException(std::string("triggering clock \"")
						   .append(transition.triggeringClock)
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
	transitions_by_clock_[transition.triggeringClock].emplace_back(ptr);
}


void
ModuleInstance::add_transition(Transition&& transition)
{
#ifndef NDEBUG
	if (0 <= globalIndex_ || 0 <= firstClock_)
		throw FigException("this module has already been added to the network");
	if (!is_our_clock(transition.triggeringClock))
		throw FigException(std::string("triggering clock \"")
						   .append(transition.triggeringClock)
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
	transitions_by_clock_[transition.triggeringClock].emplace_back(ptr);
}


const Label&
ModuleInstance::jump(const std::string& clockName,
					 const CLOCK_INTERNAL_TYPE& elapsedTime,
					 Traial& traial) const
{
	if (!sealed_)
#ifndef NDEBUG
		throw FigException("this module hasn't been sealed yet");
#else
		return;
#endif
	assert(is_our_clock(clockName));
	auto transitions = transitions_by_clock_.at(clockName);
	for (auto& tr_ptr: transitions) {
		if (tr_ptr->pre(traial.state)) { // If the traial satisfies this precondition
			tr_ptr->pos(traial.state);   // apply postcondition to its state
			tr_ptr->handle_clocks(       // and update all our clocks.
				traial,
				begin(lClocks_),
				end(lClocks_),
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
					 Traial& traial) const
{
	if (!sealed_)
#ifndef NDEBUG
		throw FigException("this module hasn't been sealed yet");
#else
		return;
#endif
	assert(label.is_output());
	if (label.is_tau())
		return;
#ifndef NDEBUG
	try {
#endif
	auto transitions = transitions_by_label_.at(label.str);
	for (auto& tr_ptr: transitions) {
		if (tr_ptr->pre(traial.state)) { // If the traial satisfies this precondition
			tr_ptr->pos(traial.state);   // apply postcondition to its state
			tr_ptr->handle_clocks(       // and update all our clocks.
				traial,
				begin(lClocks_),
				end(lClocks_),
				firstClock_,
				elapsedTime);
			break;  // Only one transition could've been enabled, we trust Raúl
		}
	}
#ifndef NDEBUG
	} catch (std::out_of_range) {
		throw FigException(std::string("output label \"").append(label.str)
						   .append("\" wasn't found among the transitions ")
						   .append("of module \"").append(name).append("\""));
	}
#endif
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
	if (sealed_)
#ifndef NDEBUG
		throw FigException("this module has already been sealed");
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
		throw FigException("this module has already been sealed");
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
