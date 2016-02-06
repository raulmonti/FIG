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
                                               const size_t& numRuns) const
{
	long numSuccesses(0);
    assert(0u < numRuns);
    Traial& traial = TraialPool::get_instance().get_traial();
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
    TraialPool::get_instance().return_traial(std::move(traial));
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
                              &SimulationEngineNosplit::transient_event);
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
                              &SimulationEngineNosplit::transient_event_concrete);
    // Last call to "transient_event_concrete()" updated "lastEvents_"
	return IS_RARE_EVENT(lastEvents_) ? 1 : 0;
}

} // namespace fig
