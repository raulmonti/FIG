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
#include <PropertyRate.h>
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
	/// Original Traial for a batch means mechanism
	Traial& oTraial_;
//	thread_local Traial& oTraial_;

public:  // Ctor

	/// Data ctor
	SimulationEngineNosplit(std::shared_ptr<const ModuleNetwork> model,
	                        bool thresholds = false);

	~SimulationEngineNosplit() override;

public:  // Accessors

	inline bool isplit() const noexcept override final { return false; }

//	inline unsigned global_effort() const noexcept override { return global_effort_default(); }

	inline unsigned global_effort_default() const noexcept override { return 1u; }

public:  // Engine setup

//	void bind(...) override;  // We can hook up with any, no check needed

//	/// Irrelevant for standard Monte Carlo
//	void set_global_effort(unsigned) override {}

protected:  // Simulation helper functions

	std::vector<double>
	transient_simulations(const PropertyTransient& property,
						  const size_t& numRuns) const override;

	double rate_simulation(const PropertyRate& property,
						   const size_t& runLength,
						   bool reinit = false) const override;

public:  // Traial observers/updaters

	/// @copydoc SimulationEngine::transient_event()
	/// @note Makes no assumption about the ImportanceFunction altogether
	inline bool transient_event(const Property& property,
								Traial& traial,
								Event& e) const override
		{
		    e = property.is_stop(traial.state) ? EventType::STOP
			                                   : EventType::NONE;
			if (property.is_rare(traial.state))
				SET_RARE_EVENT(e);
			return interrupted ||
			(
			    EventType::NONE != e
			);
		}

	/// @copydoc SimulationEngine::transient_event()
	/// @note This function assumes a \ref ImportanceFunctionConcrete
	///       "concrete importance function" is currently bound to the engine
	inline bool transient_event_concrete(const Property&,
										 Traial& traial,
										 Event& e) const
		{
			e = MASK(cImpFun_->info_of(traial.state));
			return interrupted ||
			(
			    IS_RARE_EVENT(e) || IS_STOP_EVENT(e)
			);
		}

	/// @copydoc SimulationEngine::rate_event()
	/// @note Makes no assumption about the ImportanceFunction altogether
	inline bool rate_event(const Property& property,
						   Traial& traial,
						   Event& e) const override
		{
		    e = property.is_rare(traial.state) ? EventType::RARE
			                                   : EventType::NONE;
			if (traial.lifeTime > SIM_TIME_CHUNK) {  // reduce fp precision loss
				traial.lifeTime -= SIM_TIME_CHUNK;
				simsLifetime -= SIM_TIME_CHUNK;
			}
			return interrupted ||
			(
			    traial.lifeTime > simsLifetime || IS_RARE_EVENT(e)
			);
		}

	/// @copydoc SimulationEngine::transient_event()
	/// @note This function assumes a \ref ImportanceFunctionConcrete
	///       "concrete importance function" is currently bound to the engine
	inline bool rate_event_concrete(const Property&,
									Traial& traial,
									Event& e) const
		{
			e = MASK(cImpFun_->info_of(traial.state));
			if (traial.lifeTime > SIM_TIME_CHUNK) {  // reduce fp precision loss
				traial.lifeTime -= SIM_TIME_CHUNK;
				simsLifetime -= SIM_TIME_CHUNK;
			}
			return interrupted ||
			(
			    traial.lifeTime > simsLifetime || IS_RARE_EVENT(e)
			);
		}

	/// Simulate (accumulating time) as long as we remain in rare states.
	/// Used for time registration in rate simulations.
	/// @note Makes no assumption about the ImportanceFunction altogether
	inline bool count_time(const Property& prop, Traial& t, Event&) const
	    { return !prop.is_rare(t.state); }

	/// Simulate (accumulating time) as long as we remain in rare states.
	/// Used for time registration in rate simulations.
	/// @note This function assumes a \ref ImportanceFunctionConcrete
	///       "concrete importance function" is currently bound to the engine
	inline bool count_time_concrete(const Property&, Traial& t, Event&) const
		{ return !IS_RARE_EVENT(cImpFun_->info_of(t.state)); }
};

} // namespace fig

#endif // SIMULATIONENGINENOSPLIT_H

