//==============================================================================
//
//  SimulationEngineSFE.h
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

#ifndef SIMULATIONENGINESFE_H
#define SIMULATIONENGINESFE_H

// C++
#include <vector>
// FIG
#include <SimulationEngineFixedEffort.h>
#include <PropertyTransient.h>

namespace fig
{

class PropertyRate;

/**
 * @brief Engine for Standard Fixed Effort importance-splitting simulations
 *
 *        This engine implements the importance splitting strategy
 *        named "Fixed Effort" in Marnix Garvel's PhD thesis.<br>
 *        A fixed number of simulations is run on each threshold-level,
 *        counting how many make it to the <i>next</i> level and how many don't.
 *        The resulting proportion is the probability of "going up"
 *        from that threshold level (<tt>i</tt>) to the next (<tt>i+1</tt>),
 *        i.e. the conditional probability of reaching level <tt>i+1</tt>
 *        given simulations start on level <tt>i</tt>.<br>
 *        An estimate of the rare event probability is a product
 *        of such conditional probabilities computed for all threshold-levels.
 *
 * @note Standard Fixed Effort assumes all paths to the rare event visit
 *       monotonically-increasing importance values (and threshold levels)
 *       and that there is no importance skipping.
 *       That is, it relies on the assumption that any simulation path reaching
 *       level <tt>i+1</ttt> has visited level <tt>i</tt>.
 *
 * @see SimulationEngineBFE
 * @see SimulationEngineFixedEffort
 */
class SimulationEngineSFE : public SimulationEngineFixedEffort
{
public:

	SimulationEngineSFE();

protected:  // Utils for the class and its kin

	virtual void fixed_effort(const DerivedProperty& property,
							  const Simulator& engine,
							  TraialMonitor watch_events,
							  const ThresholdsVec& thresholds,
							  std::vector< double >& Pup) const override;

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

#endif // SIMULATIONENGINESFE_H