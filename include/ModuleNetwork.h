//==============================================================================
//
//  ModuleNetwork.h
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


#ifndef MODULENETWORK_H
#define MODULENETWORK_H

// C++
#include <vector>
// FIG
#include <core_typedefs.h>
#include <Module.h>
#include <State.h>
#include <Variable.h>
#include <Clock.h>


namespace fig
{


/**
 * @brief Network of \ref ModuleInstance "module instances" synchronized
 *        through input/output \ref Label "labels".
 *
 *        This class holds a memory-contiguous view of the global \ref State
 *        "state": a vector with <i>a copy of</i> the variables from all the
 *        modules composing the network. The same is done with the \ref Clock
 *        "clocks".
 *
 * @todo TODO fill this doxygen doc, and implement class!!!
 */
class ModuleNetwork : public Module
{
	/// Unified, memory-contiguous global vector of \ref Variable "variables"
	State< STATE_INTERNAL_TYPE > gState;

public:
	virtual inline void accept(ImportanceFunction& ifun)
		{ ifun.assess_importance(this); }
};

} // namespace fig

#endif // MODULENETWORK_H

