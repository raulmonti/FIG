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
#include <ModelSuite.h>


namespace fig
{

// Available engine names in SimulationEngine::names
SimulationEngineNosplit::SimulationEngineNosplit(
    std::shared_ptr<const ModuleNetwork> network) :
		SimulationEngine("nosplit", network)
{ /* Not much to do around here */ }


double
SimulationEngineNosplit::simulate(const Property &property,
                                  const size_t& numRuns) const
{
	assert(numRuns > 0u);
	double result(0.0);

	if (!bound())
#ifndef NDEBUG
		throw_FigException("engine isn't bound to any importance function");
#else
		return -1.0;
#endif

    switch (property.type) {

	case PropertyType::TRANSIENT: {
        auto transientProp = dynamic_cast<const PropertyTransient&>(property);
		size_t numSuccesses(0u);
//		#pragma omp parallel  // we MUST parallelize this, it's stupid not to
		Traial& traial = TraialPool::get_instance().get_traial();
		for (size_t i = 0 ; i < numRuns ; i++) {
			traial.initialize(network_, impFun_);
            network_->simulation_step(traial, *this, property);
			if (transientProp.is_goal(traial.state))
				numSuccesses++;
		}
		TraialPool::get_instance().return_traial(std::move(traial));
		result = static_cast<double>(numSuccesses) / numRuns;
		} break;

	case PropertyType::THROUGHPUT:
	case PropertyType::RATE:
	case PropertyType::PROPORTION:
	case PropertyType::BOUNDED_REACHABILITY:
		throw_FigException(std::string("property type isn't supported by ")
						   .append(name_).append(" simulation yet"));
		break;

	default:
		throw_FigException("invalid property type");
		break;
	}

	return result;
}


bool
SimulationEngineNosplit::event_triggered(const Property &property,
                                         const Traial& traial) const
{
    switch (property.type) {

	case PropertyType::TRANSIENT: {
        auto transientProp = dynamic_cast<const PropertyTransient&>(property);
		if (transientProp.is_goal(traial.state) ||
			transientProp.is_stop(traial.state))
			return true;
		} break;

	case PropertyType::THROUGHPUT:
	case PropertyType::RATE:
	case PropertyType::PROPORTION:
	case PropertyType::BOUNDED_REACHABILITY:
		throw_FigException("property type isn't supported yet");
		break;

	default:
		throw_FigException("invalid property type");
		break;
	}
	return false;
}

} // namespace fig
