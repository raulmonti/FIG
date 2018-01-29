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
 *        BFE <i>does not assume</i> the existence of <i>a single path</i>
 *        leading from threshold <tt>i</tt> to some higher threshold
 *        <tt>j>i</tt>.
 *        Instead all possible importance (or threshold) trails from the
 *        initial state to the rare event are considered.<br>
 *        The probability of the rare event is estimated by the equation:
 *        <p>&sum;<sub>l∊L</sub> <tt>Prob(path<sub>l</sub>)</tt>,</p>
 *        where the <tt>l</tt>-th path is a trail of
 *        <tt>N<sub>l</sub>>0</tt> thresholds:
 *        <p><tt>path<sub>l</sub> = T<sub>1<sub>l</sub></sub> ···
 *                                  T<sub>N<sub>l</sub></sub></tt>.</p>
 *
 * @note Correctness depends on path independence: each potential trail
 *       <tt>path<sub>l</sub></tt> must be disjoint of all others
 *       <tt>k∊L,k&ne;l</tt>. In other words, once a simulation "chooses"
 *       a path then it must follow <i>that path only</i> until the rare event
 *       is found or the simulation is truncated. If this condition is not met,
 *       and paths can merge, then the &sum; used as estimate for the
 *       rare event probability could e.g. yield a value > 1.
 *
 * @see SimulationEngineSFE
 * @see SimulationEngineFixedEffort
 */
class SimulationEngineBFE : public SimulationEngineFixedEffort
{
	/// Internal Traials for fixed_effort() computations
	std::vector< std::forward_list< Reference< Traial > > > traials_;

public:

	/// Default ctor
	SimulationEngineBFE(std::shared_ptr<const ModuleNetwork> model);

protected:  // Utils for the class and its kin

	/// @brief Run <i>once</i> the Branching Fixed Effort algorithm
	/// @copydetails SimulationEngineFixedEffort::fixed_effort()
	/// @note The \p result may contain several path candidates,
	///       since this version of Fixed Effort considers all potential
	///       paths from the initial state towards the rare event
	void fixed_effort(const ThresholdsVec&,
	                  ThresholdsPathCandidates&,
	                  EventWatcher) const override
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
