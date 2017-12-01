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
#include <PropertyTransient.h>
#include <core_typedefs.h>
#include <FigException.h>
#include <Traial.h>


namespace fig
{

class PropertyRate;

/**
 * @brief Engine for Fixed Effort importance-splitting simulations
 *
 *        This engine implements the importance splitting strategy
 *        named "Fixed Effort" in Marnix Garvel's PhD thesis.<br>
 *        A fixed number of simulations is run on each threshold-level,
 *        counting how many make it to the next level and how many don't.
 *        The resulting proportion is the probability of "going up" on that
 *        threshold level; an estimate of the rare event is a product of
 *        the proportions computed for all threshold-levels.
 *
 * @warning Hard-coded to use importance values as threshold-levels,
 *          i.e. every value of the ImportanceFunction is a threshold.
 */
class SimulationEngineFixedEffort : public SimulationEngine
{
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
	SimulationEngineFixedEffort(std::shared_ptr<const ModuleNetwork> network,
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

protected:  // Simulation helper functions

	/// Clean \ref stack_ "internal ADT" used for batch means,
	/// forcing the next simulation to be <i>fresh</i>.
	void reinit_stack() const;

	std::vector<double>
	transient_simulations(const PropertyTransient& property,
						  const size_t& numRuns) const override;

	/// @todo TODO implement
	inline double
	rate_simulation(const PropertyRate&, const size_t&, bool) const override
		{ throw_FigException("TODO: implement!"); }

private:  // Traial observers/updaters

	/// @copydoc SimulationEngine::transient_event()
	/// @note Makes no assumption about the ImportanceFunction altogether
	inline bool
	transient_event(const PropertyTransient& property, Traial& traial, Event&) const override
		{
		    const ImportanceValue newLvl = impFun_->level_of(traial.state);
			traial.depth -= newLvl - traial.level;
			traial.level = newLvl;
			return /* level-up:   */ traial.depth < 0 ||
				   /* rare event: */ property.is_rare(traial.state) ||
				   /* stop event: */ property.is_stop(traial.state);
		}

	/// @todo TODO implement
	inline bool
	rate_event(const PropertyRate&, Traial&, Event&) const override
		{ throw_FigException("TODO: implement!"); }
};

} // namespace fig

#endif // SIMULATIONENGINEFIXEDEFFORT_H
