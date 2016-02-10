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

	virtual unsigned splits_per_threshold() const noexcept;

	/// @copydoc dieOutDepth_
	inline const unsigned& die_out_depth() const noexcept
		{ return dieOutDepth_; }

public:  // Engine setup

	virtual void bind(std::shared_ptr< const ImportanceFunction >);

	/// @see splits_per_threshold()
	/// @throw FigException if the value is invalid
	void set_splits_per_threshold(unsigned splitsPerThreshold);

	/// @see die_out_depth()
	/// @throw FigException if the value is invalid
	void set_die_out_depth(unsigned dieOutDepth);

protected:  // Simulation helper functions

	virtual double log_experiments_per_sim() const;

	virtual double transient_simulations(const PropertyTransient& property,
										 const size_t& numRuns) const;

public:  // Traial observers/updaters

	/// @copydoc SimulationEngine::transient_event()
	/// @note Makes no assumption about the ImportanceFunction altogether
	/// @note Attempted inline in a desperate need for speed
	inline virtual bool transient_event(const PropertyTransient& property,
										Traial& traial,
										Event& e) const
		{
			// Event marking is done in accordance with the checks performed
			// in the transient_simulations() virtual member function
			if (property.is_stop(traial.state)) {
				e = EventType::STOP;
			} else {
				ImportanceValue newThr = impFun_->importance_of(traial.state);
				if (newThr < traial.importance) {
					// Went down... too far?
					if (++traial.depth > static_cast<short>(dieOutDepth_))
						e = EventType::THR_DOWN;
				} else if (property.is_rare(traial.state)) {
					e = EventType::RARE;
				} else if (newThr > traial.importance) {
					// Store in 'depth' the negative # of thresholds crossed
					traial.depth = traial.importance - newThr;
					e = EventType::THR_UP;
				}
				traial.importance = newThr;
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
			// in the transient_simulations() virtual member function
			auto newStateInfo = impFun_->info_of(traial.state);
			e = MASK(newStateInfo);
			if (!IS_STOP_EVENT(e)) {
				const ImportanceValue newThr = UNMASK(newStateInfo);
				if (newThr < traial.importance) {
					// Went down... too far?
					if (++traial.depth > static_cast<short>(dieOutDepth_))
						SET_THR_DOWN_EVENT(e);
				} else if (newThr > traial.importance) {
					// Store in 'depth' the negative # of thresholds crossed
					traial.depth = traial.importance - newThr;
					SET_THR_UP_EVENT(e);
				}
				traial.importance = newThr;
				// rare event info is already marked inside 'e'
			}
			return EventType::NONE != e;
		}
};

} // namespace fig

#endif // SIMULATIONENGINERESTART_H
