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
// FIG
#include <FigException.h>
#include <ModuleInstance.h>


namespace fig
{

void
ModuleInstance::add_transition(const Transition& transition)
{

#ifndef NDEBUG
	auto clockFound = std::find_if(lClocks_.begin(),
								   lClocks_.end(),
								   [&] (const Clock& c)
								   { c.name == transition.triggeringClock(); });
	if (lClocks_.end() == clockFound)
		throw FigException(std::string("triggering clock \"")
						   .append(transition.triggeringClock())
						   .append("\" does not reside in module \"")
						   .append(name).append("\""));
#endif
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
				firstClock_,
				numClocks_,
				elapsedTime);
			break;  // Only one transition could've been enabled, we trust Raúl
		}
	}
}


bool
ModuleInstance::is_our_clock(const Clock& c)
{
	auto clockFound = std::find_if(lClocks_.begin(),
								   lClocks_.end(),
								   [&] (const Clock& clk)
								   { c.name == clk.name; });
	return lClocks_.end() != clockFound;
}

} // namespace fig
