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
//		#pragma omp parallel  // we MUST parallelize this, it's stupid not to
		Traial& traial = TraialPool::get_instance().get_traial();
		result = transient_simulation(dynamic_cast<const PropertyTransient&>(property),
									  numRuns,
									  traial);
		TraialPool::get_instance().return_traial(std::move(traial));
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


void
SimulationEngineNosplit::simulate(const Property& property,
								  const size_t& batchSize,
								  ConfidenceInterval& interval) const
{
	if (!bound())
#ifndef NDEBUG
		throw_FigException("engine isn't bound to any importance function");
#else
		return -1.0;
#endif

	switch (property.type) {

	case PropertyType::TRANSIENT: {
		assert (!interrupted);
		Traial& traial = TraialPool::get_instance().get_traial();
		while (!interrupted) {
			double newEstimate =
				transient_simulation(dynamic_cast<const PropertyTransient&>(property),
									 batchSize,
									 traial);
			if (!interrupted)
				interval.update(newEstimate);
		}
		TraialPool::get_instance().return_traial(std::move(traial));
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
}


bool
SimulationEngineNosplit::event_triggered_generic(const Property &property,
												 const Traial& traial) const
{
    switch (property.type) {

    case PropertyType::TRANSIENT: {
		auto transientProp = static_cast<const PropertyTransient&>(property);
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


bool
SimulationEngineNosplit::event_triggered_concrete(const Property &property,
                                                  const Traial& traial) const
{
    globalState_.copy_from_state_instance(traial.state);
    lastEvents_ = cImpFun_->events_of(globalState_);

    switch (property.type) {

	case PropertyType::TRANSIENT:
        if (IS_RARE_EVENT(lastEvents_) || IS_STOP_EVENT(lastEvents_))
			return true;
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


double
SimulationEngineNosplit::transient_simulation(const PropertyTransient& property,
											  const size_t& numRuns,
											  Traial& traial) const
{
	long numSuccesses(0);
	// For the sake of efficiency, distinguish when operating with a concrete ifun
    if (impFun_->concrete()) {
        for (size_t i = 0u ; i < numRuns ; i++) {
            traial.initialize(network_, impFun_);
			network_->simulation_step(traial,
									  property,
									  *this,
									  &SimulationEngine::event_triggered_concrete);
            // Last call to "event_triggered_concrete" updated lastEvents_
            if (IS_RARE_EVENT(lastEvents_))
                numSuccesses++;
        }
    } else {
        for (size_t i = 0u ; i < numRuns ; i++) {
            traial.initialize(network_, impFun_);
			network_->simulation_step(traial,
									  property,
									  *this,
									  &SimulationEngine::event_triggered_generic);
            if (property.is_goal(traial.state))
                numSuccesses++;
        }
    }
	return static_cast<double>(numSuccesses) / numRuns;
}


} // namespace fig
