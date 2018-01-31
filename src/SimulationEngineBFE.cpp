//==============================================================================
//
//  SimulationEngineBFE.cpp
//
//  Copyleft 2017-
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
#include <algorithm>   // std::fill(), std::move()
#include <functional>  // std::bind()
// FIG
#include <SimulationEngineBFE.h>
#include <PropertyTransient.h>
#include <TraialPool.h>
#include <ModelSuite.h>

// ADL
using std::begin;
using std::end;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

SimulationEngineBFE::SimulationEngineBFE(std::shared_ptr<const ModuleNetwork> model) :
        SimulationEngineFixedEffort("bfe", model)
{ /* Not much to do around here */ }


SimulationEngineFixedEffort::EventWatcher
SimulationEngineBFE::get_event_watcher(const Property& property) const
{
	using namespace std::placeholders;
	if (property.type == PropertyType::TRANSIENT)
		return std::bind(&SimulationEngineBFE::transient_event, *this, _1, _2, _3);
	else if (property.type == PropertyType::RATE)
		return std::bind(&SimulationEngineBFE::rate_event, *this, _1, _2, _3);
	else
		throw_FigException("unsupported property type: "+std::to_string(property.type));
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
