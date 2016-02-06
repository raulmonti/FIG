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
										 const size_t& numRuns,
										 Traial& traial) const;

//	/// Run single transient simulation starting from Traial's current state,
//	/// making no assumptions about the internal ImportanceFunction whatsoever.
//	/// @return 1 if reached a 'goal' state, 0 if reached a 'stop' state
//	long transient_simulation_generic(const PropertyTransient& property,
//									  Traial& traial);
//
//	/// Run single transient simulation starting from Traial's current state,
//	/// assuming we're bound to an ImportanceFunctionConcrete.
//	/// @return 1 if reached a 'goal' state, 0 if reached a 'stop' state
//	long transient_simulation_concrete(const PropertyTransient& property,
//									   Traial& traial);

public:  // Traial observers/updaters

	/// @copydoc SimulationEngine::event_triggered()
	/// @note Makes no assumption about the ImportanceFunction altogether
	virtual bool event_triggered(const Property& property,
								 Traial& traial) const
	{ return false; /** @todo TODO implement */ }

	/// @copydoc SimulationEngine::event_triggered()
	/// @note This function assumes a \ref ImportanceFunctionConcrete
	///       "concrete importance function" is currently bound to the engine
	bool event_triggered_concrete(const Property& property,
								  Traial& traial) const
	{ return false; /** @todo TODO implement */ }
};

} // namespace fig

#endif // SIMULATIONENGINERESTART_H
