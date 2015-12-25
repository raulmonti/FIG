//==============================================================================
//
//  SimulationEngine.h
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


#ifndef SIMULATIONENGINE_H
#define SIMULATIONENGINE_H

#include <core_typedefs.h>
#include <ImportanceFunction.h>
#include <Property.h>
#include <Traial.h>


namespace fig
{

/// @todo TODO define ConfidenceInterval class and erase this dummy
class ConfidenceInterval;
/// @todo TODO define StoppingCondition class and erase this dummy
class StoppingCondition;

/**
 * @brief Abstract base simulation engine
 *
 *        Simulation engines embody the semantics of the different
 *        simulation strategies offered by the FIG tool, such as the
 *        RESTART importance splitting technique.
 */
class SimulationEngine
{
protected:

    /// Importance function currently built
    ImportanceFunction* const currentIfun;

public:

    /**
     * @brief Simulate in model for given property making a specified effort
     * @param prop   Property whose value is trying to be estimated
     * @param ifun   Currently built importance function
     * @param effort Number of runs or simulation time for which
     *               this simulation will be run
     * @return Estimated value for the property at the end of this simulation
     */
    virtual double simulate(const Property& prop,
                            const ImportanceFunction& ifun,
                            const StoppingCondition& effort) const = 0;

    /**
     * @brief Simulate in model for given property until interrupted
     * @param prop Property whose value is trying to be estimated
     * @param ifun Currently built importance function
     * @param ci   ConfidenceInterval regularly updated with estimation info
     */
    virtual void simulate(const Property& prop,
                          const ImportanceFunction& ifun,
                          ConfidenceInterval& ci) const = 0;

    /**
     * @brief Were the last events triggered by the given Traial
     *        relevant for this simulation engine?
     * @param traial Embodiment of a simulation run through the system model
     */
    virtual bool eventTriggered(const Traial& traial) const = 0;

    /// @todo TODO define "Event inspect(const Traial&)"
    ///
    ///       It should use the current ImportanceFunction to assess
    ///       the importance of the current Traial.state.
    ///
    ///       It may also need info like the THRESHOLDS_DOWN_TOLERANCE
    ///       or the maximum simulation time, which we will store in the
    ///       SimulationEngine currently used, or something like that.
};

} // namespace fig

#endif // SIMULATIONENGINE_H

