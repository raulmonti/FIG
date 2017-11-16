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
	/// Number of simulations launched per threshold-level
	static constexpr size_t EFFORT_PER_LEVEL = (1u)<<(8u);

	/// Stack of \ref Traial "traials" for a batch means mechanism
	mutable std::vector< Reference< Traial > > traials_;

public:

	/// Default ctor
	SimulationEngineFixedEffort(std::shared_ptr<const ModuleNetwork> network);

	~SimulationEngineFixedEffort();

public:  // Accessors

	/// @deprecated
	unsigned splits_per_threshold() const noexcept override { return 0; }

	/// @copydoc EFFORT_PER_LEVEL
	static size_t effort_per_level() { return EFFORT_PER_LEVEL; }

public:  // Engine setup

	void bind(std::shared_ptr< const ImportanceFunction >) override;

protected:  // Simulation helper functions

	/// Clean \ref stack_ "internal ADT" used for batch means,
	/// forcing the next simulation to be <i>fresh</i>.
	void reinit_stack() const;

	/// @todo TODO implement
	inline double
	log_experiments_per_sim() const override
		{ throw_FigException("TODO: implement!"); }

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
			const ImportanceValue newImp = impFun_->importance_of(traial.state);
			traial.depth -= newImp - traial.level;
			traial.level = newImp;
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
