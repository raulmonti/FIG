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
#include <State.h>
#include <PropertyTransient.h>
#include <ImportanceFunctionConcrete.h>


namespace fig
{

class PropertyTransient;

/**
 * @brief Engine for standard Monte Carlo simulations
 *
 *        This engine implements the classical "blind strategy", where
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

public:  // Accessors

	virtual unsigned splits_per_threshold() const noexcept { return 1u; }

public:  // Engine setup

//	virtual void bind(...);  // We can hook up with any, no check needed

protected:  // Simulation helper functions

	virtual double log_experiments_per_sim() const;

	virtual double transient_simulations(const PropertyTransient& property,
										 const size_t& numRuns) const;

public:  // Traial observers/updaters

	/// @copydoc SimulationEngine::transient_event()
	/// @note Makes no assumption about the ImportanceFunction altogether
	/// @note Attempted inline in a desperate seek of efficiency
	inline virtual bool transient_event(const PropertyTransient& property,
										Traial& traial,
										Event& e) const
		{
			e = !property.expr1(traial.state) ? EventType::STOP
											  : EventType::NONE;
			if (property.expr2(traial.state))
				SET_RARE_EVENT(e);
			return EventType::NONE != e;
		}

	/// @copydoc SimulationEngine::transient_event()
	/// @note This function assumes a \ref ImportanceFunctionConcrete
	///       "concrete importance function" is currently bound to the engine
	/// @note Attempted inline in a desperate seek of efficiency
	inline bool transient_event_concrete(const PropertyTransient&,
										 Traial& traial,
										 Event& e) const
		{
			e = MASK(cImpFun_->info_of(traial.state));
			return IS_RARE_EVENT(e) || IS_STOP_EVENT(e);
		}
};

} // namespace fig

#endif // SIMULATIONENGINENOSPLIT_H

