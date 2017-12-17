//==============================================================================
//
//  SimulationEngineBFE.h
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

#ifndef SIMULATIONENGINEBFE_H
#define SIMULATIONENGINEBFE_H


// C++
#include <vector>
// FIG
#include <SimulationEngineFixedEffort.h>
#include <ImportanceFunction.h>
#include <PropertyTransient.h>
#include <core_typedefs.h>
#include <FigException.h>
#include <Traial.h>


namespace fig
{

class PropertyRate;

/**
 * @brief Engine for Branching Fixed Effort importance-splitting simulations
 *
 *        This engine implements an importance splitting strategy
 *        that generalises "Fixed Effort" to consider branching paths
 *        in the importance (or thresholds) space.<br>
 *        In contrast to \ref SimulationEngineSFE "Standard Fixed Effort",
 *        BFE <i>does not assume</i> that threshold-level <tt>i+1</tt>
 *        can only be reached from below by simualtions coming from
 *        threshold-level <i>i</i>.
 *        Instead, all conditional probabilities <tt>P(j|i)</tt> for
 *        <tt>j>i</tt> are considered, and the probability of the rare event
 *        is estimated by the equation:
 *        <p>&prod;<tt><sub>i=0</sub><sup>N</sup></tt>
 *           &sum;<tt><sub>j=i+1</sub><sup>N</sup></tt>
 *           <tt>P(j|i)</tt>,</p>
 *        i.e. the product of the sum of the conditional probabilities of
 *        reaching any threshold level higher than <tt>i</tt> from <tt>i</tt>.
 *
 * @see SimulationEngineSFE
 * @see SimulationEngineFixedEffort
 */
class SimulationEngineBFE : public SimulationEngineFixedEffort
{
	friend class ThresholdsBuilderES;

public:

	/// Default ctor
	SimulationEngineBFE(std::shared_ptr<const ModuleNetwork> network,
								unsigned effortPerLevel = effort_per_level_default());

protected:  // Simulation helper functions

	/**
	 * @brief Perform <i>one sweep</i> of the Branching Fixed Effort algorithm.
	 *
	 *        Starting from the initial system state, for every <i>importance
	 *        region</i> (i.e. states between two consecutive threhold levels)
	 *        run effortPerLevel_ simulations. Each simulation ends when either:
	 *        the next threshold is reached; a stop event is reached; a rare
	 *        event is reached. Do as many steps as there are importance
	 *        regions.<br>
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
	 */
	template< typename DerivedProperty,
			  class Simulator,
			  class TraialMonitor >
	void fixed_effort(const DerivedProperty& property,
					  const Simulator& engine,
					  TraialMonitor watch_events,
					  const ThresholdsVec& thresholds,
					  std::vector< double >& Pup) const;

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

#endif // SIMULATIONENGINEBFE_H
