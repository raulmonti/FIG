//==============================================================================
//
//  SimulationEngineFixedEffort.h
//
//  Copyleft 2017-
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

#ifndef SIMULATIONENGINEFIXEDEFFORT_H
#define SIMULATIONENGINEFIXEDEFFORT_H

// C++
#include <vector>
#include <functional>
// FIG
#include <SimulationEngine.h>
#include <ImportanceFunction.h>
#include <core_typedefs.h>
#include <FigException.h>
#include <Traial.h>


namespace fig
{

class PropertyRate;

/**
 * @brief Engine for Fixed Effort importance-splitting simulations
 *
 *        This is an abstract base class for \ref SimulationEngine
 *        "simulation engines" based on the importance splitting strategy
 *        named "Fixed Effort" in Marnix Garvel's PhD thesis.<br>
 *        Generally speaking the approach is to compute, <i>as independently
 *        as possible</i>, the conditional probabilities of visiting upper
 *        threshold levels from lower threshold levels. Then the rare event
 *        estimate is a product of such conditional probabilities.
 */
class SimulationEngineFixedEffort : public SimulationEngine
{
	friend class ThresholdsBuilderES;

public:

	typedef std::pair< ImportanceValue, double >  ThresholdLvlUpProb;
	typedef std::vector< ThresholdLvlUpProb >     ThresholdsPathProb;
	typedef std::vector< ThresholdsPathProb >     ThresholdsPathCandidates;

protected:

	/// Min number of simulations to launch per threshold-level
	static constexpr unsigned MIN_LEVEL_EFFORT = (1u)<<(3u);  // 8

	/// Default number of simulations launched per threshold-level;
	/// this is the global variant, where the same effort is used on all levels
	static constexpr unsigned DEFAULT_GLOBAL_EFFORT = MIN_LEVEL_EFFORT;

	/// Basis for the number of simulations run on each ("threshold-") level
	/// @note #(sims) launched on level 'l' ∝ effort(l)*BASE_NUM_SIMS
	static constexpr unsigned BASE_NUM_SIMS = 3u;

	/// When the engine is intended for thresholds building,
	/// this replaces the (yet unbuilt) thresholds
	std::function<unsigned long(const unsigned&)> arbitrary_effort;

	/// When the engine is intended for threshold building, this might be used
	unsigned long arbitraryMaxLevel;

	/// Stack of \ref Traial "traials" for a batch means mechanism
	mutable std::vector< Reference< Traial > > traials_;

	/// Property currently being estimated
	mutable const Property* property_;

public:

	/// Default ctor
	SimulationEngineFixedEffort(const std::string& simEngineName,
								std::shared_ptr<const ModuleNetwork> model,
								bool thresholds = false);

	~SimulationEngineFixedEffort() override;

public:  // Accessors

	inline bool isplit() const noexcept override final { return true; }

	/// @copydoc DEFAULT_GLOBAL_EFFORT
	inline unsigned global_effort_default() const noexcept override { return DEFAULT_GLOBAL_EFFORT; }

	/// @copydoc MIN_LEVEL_EFFORT
	static inline unsigned lvl_effort_min() noexcept { return MIN_LEVEL_EFFORT; }

	/// @copydoc BASE_NUM_SIMS
	static inline unsigned base_nsims() noexcept { return BASE_NUM_SIMS; }

protected:  // Engine setup

	void bind(std::shared_ptr< const ImportanceFunction >) override;

private:  // Simulation helper functions

	std::vector<double>
	transient_simulations(const PropertyTransient& property,
						  const size_t& numRuns) const override;

	/// @todo TODO implement
	inline double
	rate_simulation(const PropertyRate&, const size_t&, bool) const override
		{ throw_FigException("TODO: implement!"); }

protected:  // Utils for the class and its kin

	/// Retrieve the member function, wrapped as std::function via std::bind(),
	/// to be used as TraialMonitor by ModuleNetwork::simulation_step()
	/// in the internal pilot runs
	/// @see EventWatcher
	/// @example transient_event(), rate_event()
	virtual const EventWatcher& get_event_watcher(const Property&) const = 0;

	/**
	 * @brief Perform <i>one sweep</i> of the Fixed Effort algorithm.
	 *
	 *        Starting from the initial system state, for every <i>importance
	 *        region</i> (i.e. states between two threshold levels)
	 *        run a number of simulations equal to the corresponding
	 *        \ref Threshold "effort of the threshold" multiplied by the
	 *        \ref base_nsims() "basic number of simulations".<br>
	 *        Each simulation ends when it reaches either:
	 *        an upper threshold; a stop event; or a rare event.<br>
	 *        The \ref property_ "property class member" is used to determine
	 *        which states represent a stop/rare event.<br>
	 *        When the uppermost threshold is reached (rare event boundary),
	 *        or when there are no initial states to start the Traials from in
	 *        the current step, computations stop.
	 *
	 * @param result     Array where the estimated conditional probabilities
	 *                   of threshold-level-up will be stored.
	 * @param watch_events Function determining when concludes a
	 *                     \ref ModuleNetwork::simulation_step "simulation step"
	 *
	 * @note What exactly is meant by <i>next</i> or <i>upper threshold level</i>
	 *       depends on the class implementing this method
	 * @note [SO] Virtual/template in C++: https://stackoverflow.com/q/2354210,
	 *       https://stackoverflow.com/q/7968023
	 * @note [SO] Member function passing with std::functional and std::bind:
	 *       https://stackoverflow.com/a/12663020
	 */
	virtual void fixed_effort(ThresholdsPathCandidates& result,
	                          const EventWatcher& watch_events) const = 0;
};

} // namespace fig

#endif // SIMULATIONENGINEFIXEDEFFORT_H
