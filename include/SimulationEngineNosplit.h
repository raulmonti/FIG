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


namespace fig
{

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
	/// @todo TODO should be invoked by the ModelSuite who provides 'network'
	SimulationEngineNosplit(std::shared_ptr<const ModuleNetwork> network) :
		SimulationEngine("No_split", network)
		{}

public:  // Inherited virtual simulation functions

	virtual double simulate(const size_t& numRuns) const;

	virtual void simulate(ConfidenceInterval&) const {}

	virtual bool eventTriggered(const Traial& traial) const;
};

} // namespace fig

#endif // SIMULATIONENGINENOSPLIT_H

