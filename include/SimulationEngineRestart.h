//==============================================================================
//
//  SimulationEngineRestart.h
//
//  Copyleft 2016-
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

#ifndef SIMULATIONENGINERESTART_H
#define SIMULATIONENGINERESTART_H

#include <SimulationEngine.h>
#include <core_typedefs.h>
#include <Traial.h>
#include <PropertyRate.h>
#include <PropertyTransient.h>
#include <ImportanceFunctionConcrete.h>


namespace fig
{

class PropertyTransient;

/**
 * @brief Engine for RESTART importance-splitting simulations
 *
 *        This engine implements the importance splitting strategy developed
 *        by Manuel and José Villén-Altamirano.<br>
 *        In RESTART a Traial advances until it reaches a predefined importance
 *        threshold and tries to cross it "upwards", i.e. gaining on importance.
 *        At that point the state is saved and the Traial is replicated some
 *        predefined number of times. Each replica follows from them own its
 *        own independent simulation path.<br>
 *        To lighten the overhead in the number of simulation runs,
 *        all Traials going down the importance threshold where they were
 *        created, i.e. loosing on importance, are discarded.
 */
class SimulationEngineRestart : public SimulationEngine
{
	/// @copydoc SimulationEngine::splits_per_threshold()
	unsigned splitsPerThreshold_;

	/// Number of importance thresholds a simulation run must cross downwards
	/// (i.e. loosing on importance) to be discarded
	unsigned dieOutDepth_;

public:  // Ctor

	/// Data ctor
	SimulationEngineRestart(std::shared_ptr<const ModuleNetwork> network,
							const unsigned& splitsPerThreshold = 2u,
							const unsigned& dieOutDepth = 0u);

public:  // Accessors

	unsigned splits_per_threshold() const noexcept override;

	/// @copydoc dieOutDepth_
	const unsigned& die_out_depth() const noexcept;

public:  // Engine setup

	void bind(std::shared_ptr< const ImportanceFunction >) override;

	/**
	 * Set the number of replicas made of a Traial when it crosses
	 * an importance threshold upwards (i.e. gaining on importance)
	 * @param spt 1 + number of replicas made; e.g. if spt == 2 then
	 *            one replica is made, so that when a Traial crosses
	 *            a threshold then two continue: the original Traial
	 *            plus the newly created replica
	 * @throw FigException if the value is invalid
	 * @throw FigException if the engine was \ref lock() "locked"
	 * @see splits_per_threshold()
	 */
	void set_splits_per_threshold(unsigned spt);

	/// @see die_out_depth()
	/// @throw FigException if the value is invalid
	/// @throw FigException if the engine was \ref lock() "locked"
	void set_die_out_depth(unsigned dieOutDepth);

protected:  // Simulation helper functions

	double log_experiments_per_sim() const override;

	std::vector<double>
	transient_simulations(const PropertyTransient& property,
						  const size_t& numRuns) const override;

	double rate_simulation(const PropertyRate& property,
						   const size_t& runLength,
						   bool reinit = false) const override;

public:  // Traial observers/updaters

	/// @copydoc SimulationEngine::transient_event()
	/// @note Makes no assumption about the ImportanceFunction altogether
	/// @note Attempted inline in a desperate need for speed
	inline bool transient_event(const PropertyTransient& property,
								Traial& traial,
								Event& e) const override
		{
			// Event marking is done in accordance with the checks performed
			// in the transient_simulations() overriden member function
			if (!property.expr1(traial.state)) {
				e = EventType::STOP;
			} else {
				ImportanceValue newThrLvl = impFun_->level_of(traial.state);
				traial.numLevelsCrossed = newThrLvl - traial.level;
				traial.depth -= traial.numLevelsCrossed;
				traial.level = newThrLvl;
				if (traial.numLevelsCrossed < 0 &&
					traial.depth > static_cast<short>(dieOutDepth_))
					e = EventType::THR_DOWN;
				else if (traial.numLevelsCrossed > 0 && traial.depth < 0)
					e = EventType::THR_UP;
				else if (property.expr2(traial.state))
					e = EventType::RARE;
			}
			return EventType::NONE != e;
		}

	/// @copydoc SimulationEngine::transient_event()
	/// @note This function assumes a \ref ImportanceFunctionConcrete
	///       "concrete importance function" is currently bound to the engine
	/// @note Attempted inline in a desperate need for speed
	inline bool transient_event_concrete(const PropertyTransient&,
										 Traial& traial,
										 Event& e) const
		{
			// Event marking is done in accordance with the checks performed
			// in the transient_simulations() overriden member function
			auto newStateInfo = cImpFun_->info_of(traial.state);
			e = MASK(newStateInfo);
			if (!IS_STOP_EVENT(e)) {
				const ImportanceValue newThrLvl = UNMASK(newStateInfo);
				traial.numLevelsCrossed = newThrLvl - traial.level;
				traial.depth -= traial.numLevelsCrossed;
				traial.level = newThrLvl;
				if (traial.numLevelsCrossed < 0 &&
					traial.depth > static_cast<short>(dieOutDepth_))
					SET_THR_DOWN_EVENT(e);
				else if (traial.numLevelsCrossed > 0 && traial.depth < 0)
					SET_THR_UP_EVENT(e);
				// else: rare event info is already marked inside 'e'
			}
			return EventType::NONE != e;
		}

	/// @copydoc SimulationEngine::rate_event()
	/// @note Makes no assumption about the ImportanceFunction altogether
	inline bool rate_event(const PropertyRate& property,
						   Traial& traial,
						   Event& e) const override
		{
			// Event marking is done in accordance with the checks performed
			// in the rate_simulation() overriden member function
			ImportanceValue newThrLvl = impFun_->level_of(traial.state);
			traial.numLevelsCrossed = newThrLvl - traial.level;
			traial.depth -= traial.numLevelsCrossed;
			traial.level = newThrLvl;
			if (traial.numLevelsCrossed < 0 &&
				traial.depth > static_cast<short>(dieOutDepth_))
				e = EventType::THR_DOWN;
			else if (traial.numLevelsCrossed > 0 && traial.depth < 0)
				e = EventType::THR_UP;
			else if (property.expr(traial.state))
				e = EventType::RARE;
			if (traial.lifeTime > SIM_TIME_CHUNK
				&& simsLifetime > SIM_TIME_CHUNK) {
				// reduce fp precision loss
				traial.lifeTime -= SIM_TIME_CHUNK;
				simsLifetime    -= SIM_TIME_CHUNK;
			}
			return traial.lifeTime > simsLifetime || EventType::NONE != e;
		}

	/// @copydoc SimulationEngine::rate_event()
	/// @note This function assumes a \ref ImportanceFunctionConcrete
	///       "concrete importance function" is currently bound to the engine
	inline bool rate_event_concrete(const PropertyRate&,
									Traial& traial,
									Event& e) const
		{
			// Event marking is done in accordance with the checks performed
			// in the rate_simulation() overriden member function
			auto newStateInfo = cImpFun_->info_of(traial.state);
			e = MASK(newStateInfo);
			const ImportanceValue newThrLvl = UNMASK(newStateInfo);
			traial.numLevelsCrossed = newThrLvl - traial.level;
			traial.depth -= traial.numLevelsCrossed;
			traial.level = newThrLvl;
			if (traial.numLevelsCrossed < 0 &&
				traial.depth > static_cast<short>(dieOutDepth_))
				SET_THR_DOWN_EVENT(e);
			else if (traial.numLevelsCrossed > 0 && traial.depth < 0)
				SET_THR_UP_EVENT(e);
			// else: rare event info is already marked inside 'e'
			if (traial.lifeTime > SIM_TIME_CHUNK
				&& simsLifetime > SIM_TIME_CHUNK) {
				// reduce fp precision loss
				traial.lifeTime -= SIM_TIME_CHUNK;
				simsLifetime    -= SIM_TIME_CHUNK;
			}
			return traial.lifeTime > simsLifetime || EventType::NONE != e;
		}

	/// Turn off splitting and simulate (accumulating time) as long as we are
	/// among rare states. Used for time registration in rate simulations.
	/// @note Makes no assumption about the ImportanceFunction altogether
	inline bool count_time(const PropertyRate& prop, Traial& t, Event&) const
		{ return !prop.expr(t.state); }

	/// Turn off splitting and simulate (accumulating time) as long as we are
	/// among rare states. Used for time registration in rate simulations.
	/// @note This function assumes a \ref ImportanceFunctionConcrete
	///       "concrete importance function" is currently bound to the engine
	inline bool count_time_concrete(const PropertyRate&, Traial& t, Event&) const
		{ return !IS_RARE_EVENT(cImpFun_->info_of(t.state)); }
};

} // namespace fig

#endif // SIMULATIONENGINERESTART_H
