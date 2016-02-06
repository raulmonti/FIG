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

class SimulationEngineRestart : public SimulationEngine
{
	/// 1 + Number of replicas made of a Traial when it crosses
	/// an importance threshold upwards (i.e. gaining on importance)
	/// @see ThresholdsBuilder
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

	/// @copydoc splitsPerThreshold_
	inline const unsigned& get_splits_per_threshold() const noexcept
		{ return splitsPerThreshold_; }

	/// @copydoc dieOutDepth_
	inline const unsigned& get_die_out_depth() const noexcept
		{ return dieOutDepth_; }

public:  // Engine setup

	virtual void bind(std::shared_ptr< const ImportanceFunction >);

	/// @see splitsPerThreshold_
	/// @throw FigException if the value is invalid
	void set_splits_per_threshold(unsigned splitsPerThreshold);

	/// @see dieOutDepth_
	/// @throw FigException if the value is invalid
	void set_die_out_depth(unsigned dieOutDepth);

protected:  // Simulation helper functions

	virtual double transient_simulations(const PropertyTransient& property,
										 const size_t& numRuns) const;

	/**
	 * Run single transient simulation starting from given Traials stack,
	 * making no assumptions about the internal ImportanceFunction whatsoever.
	 * @param property PropertyTransient with events of interest (goal & stop)
	 * @param stack    Stack of initialized Traials for RESTART simulation
	 * @param numHits  Register updated with the number of goal events
	 *                 found on each threshold level
	 */
	void transient_simulation_generic(const PropertyTransient& property,
									  std::stack< Reference <Traial> >& stack,
									  std::vector< long >& numHits);

	/**
	 * Run single transient simulation starting from given Traials stack,
	 * assuming we're bound to an ImportanceFunctionConcrete.
	 * @param property PropertyTransient with events of interest (goal & stop)
	 * @param stack    Stack of initialized Traials for RESTART simulation
	 * @param numHits  Register updated with the number of goal events
	 *                 found on each threshold level
	 */
	void transient_simulation_concrete(const PropertyTransient& property,
									   std::stack< Reference <Traial> >& stack,
									   std::vector< long >& numHits);

public:  // Traial observers/updaters

	/// @copydoc SimulationEngine::transient_event()
	/// @note Makes no assumption about the ImportanceFunction altogether
	/// @note Attempted inline in a desperate seek of efficiency
	inline virtual bool transient_event(const PropertyTransient& property,
										Traial& traial) const
		{
			throw_FigException("TODO: implement");
			return property.is_goal(traial.state) ||
					property.is_stop(traial.state);
		}

	/// @copydoc SimulationEngine::transient_event()
	/// @note This function assumes a \ref ImportanceFunctionConcrete
	///       "concrete importance function" is currently bound to the engine
	/// @note Attempted inline in a desperate seek of efficiency
	inline bool transient_event_concrete(const PropertyTransient&,
										 Traial& traial) const
		{
			globalState_.copy_from_state_instance(traial.state);
			lastEvents_ = cImpFun_->events_of(globalState_);
			throw_FigException("TODO: implement");
			return IS_RARE_EVENT(lastEvents_) || IS_STOP_EVENT(lastEvents_);
		}
};

} // namespace fig

#endif // SIMULATIONENGINERESTART_H
