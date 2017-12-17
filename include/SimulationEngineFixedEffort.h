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
protected:

	/// Number of simulations launched per threshold-level;
	/// this is the global variant, where the same effort is used on all levels
	/// @deprecated Now each threshold level has its own effort;
	///             this class member isn't used anymore
	unsigned effortPerLevel_;

	/// Default value for effortPerLevel_
	static constexpr decltype(effortPerLevel_) DEFAULT_GLOBAL_EFFORT = (1u)<<(4u);  // 16
	static inline decltype(effortPerLevel_) effort_per_level_default() { return DEFAULT_GLOBAL_EFFORT; }

	/// Basis for the number of simulations run on each ("threshold-") level
	/// @note #(sims) launched on level 'l' ∝ effort(l)*BASE_NUM_SIMS
	static constexpr decltype(effortPerLevel_) BASE_NUM_SIMS = 3ul;

	/// Stack of \ref Traial "traials" for a batch means mechanism
	mutable std::vector< Reference< Traial > > traials_;

public:

	/// Default ctor
	SimulationEngineFixedEffort(const std::string& simEngineName,
								std::shared_ptr<const ModuleNetwork> network,
								unsigned effortPerLevel = effort_per_level_default());

	~SimulationEngineFixedEffort();

public:  // Accessors

	inline bool isplit() const noexcept override final { return true; }

	unsigned global_effort() const noexcept override;

	/// @copydoc DEFAULT_GLOBAL_EFFORT
	inline unsigned global_effort_default() const noexcept override { return DEFAULT_GLOBAL_EFFORT; }

	/// @copydoc BASE_NUM_SIMS
	static inline unsigned base_nsims() noexcept { return BASE_NUM_SIMS; }

public:  // Engine setup

	void bind(std::shared_ptr< const ImportanceFunction >) override;

	/**
	 * Set the number of independent simulations launched on each
	 * threshold-level, i.e. the (global) fixed effort from which the
	 * techniques takes its name.
	 * @param epl @copydoc effortPerLevel_
	 * @throw FigException if the value is invalid
	 * @throw FigException if the engine was \ref lock() "locked"
	 */
	void set_global_effort(unsigned epl = effort_per_level_default()) override;

private:  // Simulation helper functions

	std::vector<double>
	transient_simulations(const PropertyTransient& property,
						  const size_t& numRuns) const override;

	/// @todo TODO implement
	inline double
	rate_simulation(const PropertyRate&, const size_t&, bool) const override
		{ throw_FigException("TODO: implement!"); }

protected:  // Utils for the class and its kin

	/**
	 * @brief Perform <i>one sweep</i> of the Fixed Effort algorithm.
	 *
	 *        Starting from the initial system state, for every <i>importance
	 *        region</i> (i.e. states between two threshold levels)
	 *        run effortPerLevel_ simulations. A simulation ends when it reaches
	 *        either: an upper threshold; a stop event; or a rare event.<br>
	 *        When the uppermost threshold is reached (rare event boundary),
	 *        or when there are no initial states to start the Traials from in
	 *        the current step, computations stop.
	 *
	 * @param property Property whose value is currently being estimated
	 * @param engine   Instance of the SimulationEngine/ThresholdsBuilder
	 *                 that is performing the fixed effort
	 * @param watch_events Member function of \a engine telling when finishes a
	 *                     \ref ModuleNetwork::simulation_step "simulation step"
	 * @param thresholds Thresholds (and effort of each threshold) which
	 *                   delimit the importance regions considered on each step
	 * @param Pup        Array where the estimated conditional probabilities
	 *                   of threshold-level-up will be stored.
	 *
	 * @note What exactly is meant by <i>next</i> or <i>upper threshold level</i>
	 *       depends on the class implementing this method
	 */


// [SO] Virtual/template in C++:
// - https://stackoverflow.com/q/2354210
// - https://stackoverflow.com/q/7968023
//
//	template< typename DerivedProperty,
//			  class Simulator,
//			  class TraialMonitor >
	virtual void fixed_effort(const DerivedProperty& property,
							  const Simulator& engine,
							  TraialMonitor watch_events,
							  const ThresholdsVec& thresholds,
							  std::vector< double >& Pup) const = 0;

};

} // namespace fig

#endif // SIMULATIONENGINEFIXEDEFFORT_H
