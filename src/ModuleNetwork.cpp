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


#include <ModuleNetwork.h>


namespace fig
{

void
ModuleNetwork::add_module(ModuleInstance** module)
{
	modules.emplace_back(*module);
	auto state = (*module)->mark_added(modules.size()-1, lastClockIndex_);
	gState.append(state);
	lastClockIndex_ += (*module)->numClocks;
	*module = nullptr;
}


void
ModuleNetwork::add_module(std::shared_ptr< ModuleInstance >& module)
{
	modules.push_back(module);
	auto state = module->mark_added(modules.size()-1, lastClockIndex_);
	gState.append(state);
	lastClockIndex_ += module->numClocks;
	module = nullptr;
}

} // namespace fig
