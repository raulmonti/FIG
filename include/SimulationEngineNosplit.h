//==============================================================================
//
//  SimulationEngineNosplit.h
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


#ifndef SIMULATIONENGINENOSPLIT_H
#define SIMULATIONENGINENOSPLIT_H

#include <SimulationEngine.h>
#include <ModuleNetwork.h>


namespace fig
{

class PropertyTransient;

/**
 * @brief Engine for classical Monte Carlo simulation
 *
 *        This engine implements the standard "blind strategy", where
 *        each Traial is pushed forward following the model dynamics and
 *        without any kind of splitting. The importance function is thus
 *        disregarded. Only the property provides the most basic guiding
 *        information: whether the simulation should keep going or not.
 */
class SimulationEngineNosplit : public SimulationEngine
{
public:  // Ctor

	/// Data ctor
	SimulationEngineNosplit(std::shared_ptr<const ModuleNetwork> network);

public:  // Engine setup

//	virtual void bind(...);  // We can hook up with any, no check needed

protected:  // Simulation helper functions

	virtual double transient_simulations(const PropertyTransient& property,
										 const size_t& numRuns,
										 Traial& traial) const;

	/// Run single transient simulation starting from Traial's current state,
	/// making no assumptions about the internal ImportanceFunction whatsoever.
	/// @return 1 if reached a 'goal' state, 0 if reached a 'stop' state
	long transient_simulation_generic(const PropertyTransient& property,
									  Traial& traial) const;

	/// Run single transient simulation starting from Traial's current state,
	/// assuming we're bound to an ImportanceFunctionConcrete.
	/// @return 1 if reached a 'goal' state, 0 if reached a 'stop' state
	long transient_simulation_concrete(const PropertyTransient& property,
									   Traial& traial) const;

public:  // Traial observers/updaters

	/// @copydoc SimulationEngine::event_triggered()
	/// @note Makes no assumption about the ImportanceFunction altogether
	virtual bool event_triggered(const Property& property,
								 Traial& traial) const;

	/// @copydoc SimulationEngine::event_triggered()
	/// @note This function assumes a \ref ImportanceFunctionConcrete
	///       "concrete importance function" is currently bound to the engine
	bool event_triggered_concrete(const Property& property,
								  Traial& traial) const;
};

} // namespace fig

#endif // SIMULATIONENGINENOSPLIT_H

