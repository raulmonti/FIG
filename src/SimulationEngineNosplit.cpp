//==============================================================================
//
//  SimulationEngineNosplit.cpp
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


#include <core_typedefs.h>
#include <SimulationEngineNosplit.h>
#include <StoppingConditions.h>
#include <FigException.h>
#include <PropertyTransient.h>


namespace fig
{

double
SimulationEngineNosplit::simulate(const size_t&) const
{
	double result(0.0);

	if (!loaded())
#ifndef NDEBUG
		throw FigException("engine wasn't loaded, can't simulate");
#else
		return;
#endif

	return result;
}


bool
SimulationEngineNosplit::eventTriggered(const Traial& traial) const
{
	switch (property->type) {
	case PropertyType::TRANSIENT:
	{
		auto prop = static_cast<const PropertyTransient*>(property);
		if (prop->is_goal(traial.state) ||
			prop->is_stop(traial.state))
			return true;
	}
		break;
	case PropertyType::THROUGHPUT:
	case PropertyType::RATE:
	case PropertyType::PROPORTION:
	case PropertyType::BOUNDED_REACHABILITY:
		throw FigException("property type isn't supported yet");
		break;
	default:
		throw FigException("invalid property type");
		break;
	}
	return false;
}

} // namespace fig
