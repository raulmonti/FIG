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
#include <ImportanceFunctionConcrete.h>


namespace fig
{

class PropertyTransient;
class PropertyRate;

/**
 * @brief Engine for RESTART importance-splitting simulations
 *
 *		This engine implements the importance splitting strategy developed
 *		by Manuel and José Villén-Altamirano.<br>
 *		In RESTART a Traial advances until it reaches a predefined importance
 *		threshold and tries to cross it "upwards", i.e. gaining on importance.
 *		At that point the state is saved and the Traial is replicated some
 *		predefined number of times. Each replica follows from them own its
 *		own independent simulation path.<br>
 *		To lighten the overhead in the number of simulation runs,
 *		all Traials going down the importance threshold where they were
 *		created, i.e. loosing on importance, are discarded.
 */
class SimulationEngineRestart : public SimulationEngine
{

	/// Default number of replicas (minus one)
	/// made of a Traial when it crosses a threshold upwards
	static constexpr unsigned DEFAULT_GLOBAL_EFFORT = 3u;

	/// Number of importance thresholds a simulation run must cross downwards
	/// (i.e. loosing on importance) to be discarded
	decltype(Traial::depth) dieOutDepth_;

	/// Minimum importance value of the ImportanceFunction currently bound
	ImportanceValue minImportance_;

	/// Maximum importance value of the ImportanceFunction currently bound
	ImportanceValue maxImportance_;

	/// Original Traial for a batch means mechanism
	Traial& oTraial_;

	/// Stack of \ref Traial "traials" for a batch means mechanism,
	/// typically used for steady-state simulations
	mutable std::stack< Reference< Traial > > ssstack_;

	/// For floating point operations (to reduce precision loss)
	mutable CLOCK_INTERNAL_TYPE currentSimLength_;

	/// Whether to resample clock values every time a Traial is split
	bool resampleOnSplit_;

public:  // Ctor

	/// Data ctor
	SimulationEngineRestart(std::shared_ptr<const ModuleNetwork> model,
	                        bool thresholds = false,
	                        bool resampling = true);

	~SimulationEngineRestart() override;

public:  // Accessors

	const std::string& name() const noexcept override;

	inline bool isplit() const noexcept override final { return true; }

	/// @copydoc DEFAULT_GLOBAL_EFFORT
	inline unsigned global_effort_default() const noexcept override { return DEFAULT_GLOBAL_EFFORT; }

	/// @copydoc dieOutDepth_
	inline const decltype(dieOutDepth_)& die_out_depth() const noexcept { return dieOutDepth_; }

	/// @copydoc resampleOnSplit_
	inline bool get_resampling() const noexcept { return resampleOnSplit_; };

public:  // Engine setup

	/// @copydoc SimulationEngine::bind()
	/// @note Reinits the \ref ssstack_ "internal ADT" used for batch means
	/// @see reinit_stack()
	void bind(std::shared_ptr< const ImportanceFunction >) override;

	/// @copydoc SimulationEngine::unbind()
	/// @note Deletes local information about the ImportanceFunction
	void unbind() override;

	/// @see die_out_depth()
	/// @throw FigException if the value is invalid
	/// @throw FigException if the engine was \ref lock() "locked"
	void set_die_out_depth(unsigned dieOutDepth);

	/// @copydoc resampleOnSplit_
	void set_resampling(bool resampling = true);

private:  // Simulation helper functions

	void reset() const override;

	/// Flush the \ref ssstack_ "internal ADT" used for batch means,
	/// forcing the next simulation to be <i>fresh</i>.
	void reinit_stack() const;

	/// Fill \a stack with copies of \a traial due to level-up splitting
	/// @note Can handle several-levels-up situations
	template< typename TraialCopier >
	void handle_lvl_up(const Traial &traial,
	                   TraialCopier copy_traials,
	                   std::stack< Reference< Traial > >& stack) const;

	std::vector<double>
	transient_simulations(const PropertyTransient& property,
						  const size_t& numRuns) const override;

	double rate_simulation(const PropertyRate& property,
						   const size_t& runLength,
						   bool reinit = false) const override;

	double tbound_ss_simulation(const PropertyTBoundSS&) const override;

	/**
	 * @brief Standard RESTART run, i.e. long run method
	 *
	 *        Run a single RESTART importance-splitting simulation for
	 *        as many time units as previously set in simsLifetime.
	 *        Simulations start from the last saved ssstack_.
	 *
	 *  @note Indended for steady-state-like simulations, e.g.
	 *        \ref rate_simulation() "rate" and
	 *        \ref tbound_ss_simulation() "time bounded steady-state"
	 */
	template< typename SSProperty >
	double RESTART_run(const SSProperty& property,
					   const EventWatcher& watch_events,
					   const EventWatcher& register_time) const;

private:  // Traial observers/updaters

	/// @copydoc SimulationEngine::transient_event()
	/// @note Makes no assumption about the ImportanceFunction altogether
	/// @note Attempted inline in a desperate need for speed
	inline bool transient_event(const Property& property,
								Traial& traial,
								Event& e) const override
		{
			// Event marking is done in accordance with the checks performed
			// in the transient_simulations() overriden member function
		    e = EventType::NONE;
			if (property.is_stop(traial.state)) {
				e = EventType::STOP;
			} else {
				const auto newThrLvl = static_cast<long>(impFun_->level_of(traial.state));
				traial.numLevelsCrossed = static_cast<int>(newThrLvl - static_cast<long>(traial.level));
				traial.depth -= traial.numLevelsCrossed;
				traial.level = static_cast<decltype(traial.level)>(newThrLvl);
				if (traial.numLevelsCrossed < 0 && traial.depth > dieOutDepth_)
					e = EventType::THR_DOWN;
				else if (traial.numLevelsCrossed > 0 && traial.depth < 0)
					e = EventType::THR_UP;
				else if (property.is_rare(traial.state))
					e = EventType::RARE;
			}
			return interrupted ||
			(
			    EventType::NONE != e
			);
		}

	/// @copydoc SimulationEngine::transient_event()
	/// @note This function assumes a \ref ImportanceFunctionConcrete
	///	   "concrete importance function" is currently bound to the engine
	/// @note Attempted inline in a desperate need for speed
	inline bool transient_event_concrete(const Property&,
										 Traial& traial,
										 Event& e) const
		{
			// Event marking is done in accordance with the checks performed
			// in the transient_simulations() overriden member function
			auto newStateInfo = cImpFun_->info_of(traial.state);
			e = MASK(newStateInfo);
			if (!IS_STOP_EVENT(e)) {
				const auto newThrLvl = static_cast<long>(UNMASK(newStateInfo));
				traial.numLevelsCrossed = static_cast<int>(newThrLvl - static_cast<long>(traial.level));
				traial.depth -= traial.numLevelsCrossed;
				traial.level = static_cast<decltype(traial.level)>(newThrLvl);
				if (traial.numLevelsCrossed < 0 && traial.depth > dieOutDepth_)
					SET_THR_DOWN_EVENT(e);
				else if (traial.numLevelsCrossed > 0 && traial.depth < 0)
					SET_THR_UP_EVENT(e);
				// else: rare event info is already marked inside 'e'
			}
			return interrupted ||
			(
			    EventType::NONE != e
			);
	    }

	/// @copydoc SimulationEngine::rate_event()
	/// @note Makes no assumption about the ImportanceFunction altogether
	inline bool rate_event(const Property& property,
						   Traial& traial,
						   Event& e) const override
		{
			// Event marking is done in accordance with the checks performed
			// in the rate_simulation() overriden member function
		    e = EventType::NONE;
			//const auto newThrLvl = static_cast<long>(impFun_->level_of(traial.state));
			const auto newImportance = impFun_->importance_of(traial.state);
			const auto newThrLvl = static_cast<long>(impFun_->level_of(newImportance));
			traial.numLevelsCrossed = static_cast<int>(newThrLvl - static_cast<long>(traial.level));
			traial.depth -= traial.numLevelsCrossed;
			traial.level = static_cast<decltype(traial.level)>(newThrLvl);
			if (0 < traial.numLevelsCrossed &&
			        traial.nextSplitLevel <= static_cast<int>(traial.level)) {
					e = EventType::THR_UP;  // event B_i
					// NOTE: traial.nextSplitLevel is updated after splitting
			} else if (traial.numLevelsCrossed < 0) {
				if (traial.depth > die_out_depth()) {
					// Event D_i-j in traial [B_i,D_i-j): this retrial dies
					e = EventType::THR_DOWN;
				} else if (traial.level == impFun_->min_value()) {  // (newImportance == minImportance_)
					// Went down to threshold 0:
					if (traial.depth > 0)
						e = EventType::THR_DOWN;  // this retrial dies
					else
						traial.nextSplitLevel = 1;  // the main traial is reborn
				} else {
					// Event D_i-k in traial [B_i,D_i-j) where k<j:
					// it may be possible to generate new events B_k
					traial.nextSplitLevel = std::min(traial.nextSplitLevel,
					                                 static_cast<int>(traial.level)+die_out_depth()+1);
				}
			} else if (property.is_rare(traial.state)) {
				e = EventType::RARE;
			}
			return interrupted ||
			(
			    traial.lifeTime > simsLifetime || EventType::NONE != e
			);
		}

	/// @copydoc SimulationEngine::rate_event()
	/// @note This function assumes a \ref ImportanceFunctionConcrete
	///	      "concrete importance function" is currently bound to the engine
	inline bool rate_event_concrete(const Property&,
									Traial& traial,
									Event& e) const
		{
			// Event marking is done in accordance with the checks performed
			// in the rate_simulation() overriden member function
		    const auto newStateInfo = cImpFun_->info_of(traial.state);
			e = MASK(newStateInfo);
			const auto newThrLvl = static_cast<long>(UNMASK(newStateInfo));
			traial.numLevelsCrossed = static_cast<int>(newThrLvl - static_cast<long>(traial.level));
			traial.depth -= traial.numLevelsCrossed;
			traial.level = static_cast<decltype(traial.level)>(newThrLvl);
			if (traial.numLevelsCrossed > 0 &&
			    traial.nextSplitLevel <= static_cast<int>(traial.level)) {
				SET_THR_UP_EVENT(e);  // event B_i
				// NOTE: traial.nextSplitLevel is updated after splitting
			} else if (traial.numLevelsCrossed < 0) {
				if (traial.depth > die_out_depth()) {
					// Event D_i-j in traial [B_i,D_i-j): this retrial dies
					SET_THR_DOWN_EVENT(e);
				} else if (traial.level == cImpFun_->min_value()) {  // (cImpFun_->importance_of(traial.state) == minImportance_)
					// Went down to threshold 0:
					if (traial.depth > 0)
						SET_THR_DOWN_EVENT(e);  // this retrial dies
					else
						traial.nextSplitLevel = 1;  // the main traial is reborn
				} else {
					// Event D_i-k in traial [B_i,D_i-j) where k<j:
					// it may be possible to generate new events B_k
					traial.nextSplitLevel = std::min(traial.nextSplitLevel,
					                                 static_cast<int>(traial.level)+die_out_depth()+1);
				}
			}
			// else: rare event info is already marked inside 'e'
			return interrupted ||
			(
			    traial.lifeTime > simsLifetime || EventType::NONE != e
			);
		}

	/// Turn off splitting and simulate (accumulating time) as long as we are
	/// among rare states. Used for time registration in rate simulations.
	/// @note Makes no assumption about the ImportanceFunction altogether
	inline bool count_time(const Property& prop, Traial& t, Event&) const
		{
		    return interrupted ||
			(
			    currentSimLength_ + t.lifeTime > simsLifetime
			            || !prop.is_rare(t.state)
			);
		}

	/// Turn off splitting and simulate (accumulating time) as long as we are
	/// among rare states. Used for time registration in rate simulations.
	/// @note This function assumes a \ref ImportanceFunctionConcrete
	///	   "concrete importance function" is currently bound to the engine
	inline bool count_time_concrete(const Property&, Traial& t, Event&) const
		{
		    return interrupted ||
			(
			    currentSimLength_ + t.lifeTime > simsLifetime
			            || !IS_RARE_EVENT(cImpFun_->info_of(t.state))
			);
		}
};

} // namespace fig

#endif // SIMULATIONENGINERESTART_H
