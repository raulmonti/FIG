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

public:  // Inherited virtual setup functions

//	virtual void bind(...);  // We can hook up with any, no check needed

public:  // Inherited virtual simulation functions

	virtual double simulate(const Property& property,
							const size_t& numRuns = 1) const;

	virtual void simulate(const Property& property,
						  const size_t& batchSize,
						  ConfidenceInterval& interval) const;

	virtual bool event_triggered(const Property& property,
								 const Traial& traial) const;

private:  // Simulation helper functions

	double transient_simulation(const PropertyTransient& property,
								const size_t& numRuns,
								Traial& traial) const;
};

} // namespace fig

#endif // SIMULATIONENGINENOSPLIT_H

