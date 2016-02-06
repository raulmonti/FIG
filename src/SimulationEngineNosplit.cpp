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
#include <ImportanceFunctionConcrete.h>
#include <PropertyTransient.h>
#include <ModelSuite.h>
#include <ConfidenceInterval.h>


namespace fig
{

// Available engine names in SimulationEngine::names
SimulationEngineNosplit::SimulationEngineNosplit(
    std::shared_ptr<const ModuleNetwork> network) :
		SimulationEngine("nosplit", network)
{ /* Not much to do around here */ }


double
SimulationEngineNosplit::transient_simulations(const PropertyTransient& property,
											   const size_t& numRuns,
											   Traial& traial) const
{
	long numSuccesses(0);
	// For the sake of efficiency, distinguish when operating with a concrete ifun
	if (impFun_->concrete()) {
//		#pragma omp parallel  // we MUST parallelize this, it's stupid not to
		for (size_t i = 0u ; i < numRuns ; i++) {
			traial.initialize(*network_, *impFun_);
			numSuccesses += transient_simulation_concrete(property, traial);
		}
	} else {
//		#pragma omp parallel  // we MUST parallelize this, it's stupid not to
		for (size_t i = 0u ; i < numRuns ; i++) {
			traial.initialize(*network_, *impFun_);
			numSuccesses += transient_simulation_generic(property, traial);
		}
	}
	return static_cast<double>(numSuccesses) / numRuns;
}


long
SimulationEngineNosplit::transient_simulation_generic(
	const PropertyTransient& property,
	Traial& traial) const
{
	network_->simulation_step(traial,
							  property,
							  *this,
							  &SimulationEngineNosplit::event_triggered);
	// Check current state events via the property
	return property.is_goal(traial.state) ? 1 : 0;
}


long
SimulationEngineNosplit::transient_simulation_concrete(
	const PropertyTransient& property,
	Traial& traial) const
{
	network_->simulation_step(traial,
							  property,
							  *this,
							  &SimulationEngineNosplit::event_triggered_concrete);
	// Last call to "event_triggered_concrete()" updated "lastEvents_"
	return IS_RARE_EVENT(lastEvents_) ? 1 : 0;
}


bool
SimulationEngineNosplit::event_triggered(const Property &property,
										 Traial& traial) const
{
    switch (property.type) {

    case PropertyType::TRANSIENT: {
		auto transientProp = static_cast<const PropertyTransient&>(property);
		return transientProp.is_goal(traial.state) || transientProp.is_stop(traial.state);
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


bool
SimulationEngineNosplit::event_triggered_concrete(const Property &property,
												  Traial& traial) const
{
    globalState_.copy_from_state_instance(traial.state);
    lastEvents_ = cImpFun_->events_of(globalState_);

    switch (property.type) {

	case PropertyType::TRANSIENT:
		return IS_RARE_EVENT(lastEvents_) || IS_STOP_EVENT(lastEvents_);
		break;

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
