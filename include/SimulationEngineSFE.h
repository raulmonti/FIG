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
#include <Property.h>

namespace fig
{

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
 * @note The <i>original</i> Fixed Effort algorithm assumes that no
 *       simulation path can peform importance skipping, i.e. all simulations
 *       reaching level <tt>i+1</tt> must have visited level <tt>i</tt>.<br>
 *       This implementation is more general and <i>tolerates importance
 *       skipping in simulations</i>, so the "next level" after threshold-level
 *       <tt>i</tt> can be any <tt>j>i</tt>.
 *
 * @note This algorithm only uses the thresholds in <i>the</i> (hopefully)
 *       likeliest path towards the rare event. If importance skipping exhibits
 *       branching behaviour, e.g. the rare event can be reached doing
 *       <tt>0-->1-->3-->RARE</tt> or doing <tt>0-->2-->3-->RARE</tt>,
 *       the \ref SimulationEngineBFE "branching variant of Fixed Effort"
 *       may be better suited.
 *
 * @see SimulationEngineBFE
 * @see SimulationEngineFixedEffort
 */
class SimulationEngineSFE : public SimulationEngineFixedEffort
{
	/// Internal Traials for fixed_effort() computations
	mutable std::vector< Reference< Traial > > traials_;

public:

	SimulationEngineSFE(std::shared_ptr<const ModuleNetwork> model,
						const bool thresholds = false);
	virtual ~SimulationEngineSFE();

protected:  // Utils for the class and its kin

//	void fetch_internal_traials(const size_t& N) const override;
	const EventWatcher& get_event_watcher(const Property&) const override;

	/// @brief Run <i>once</i> the Standard Fixed Effort algorithm,
	///        generalised to cope with importance skipping.
	/// @copydetails SimulationEngineFixedEffort::fixed_effort()
	/// @note The \p result will <i>always>/i> contain a single path,
	///       since this is a <i>greedy version</i> of Fixed Effort which
	///       ignores paths to the rare event other than the easiest to find.
	///       Is no path is found, \p result will contain a single empty path.
	void fixed_effort(ThresholdsPathCandidates& result,
	                  const EventWatcher& watch_events) const override;

private:  // Traial observers/updaters

	/// @copydoc SimulationEngine::transient_event()
	/// @note Makes no assumption about the ImportanceFunction altogether
	inline bool
	transient_event(const Property& property, Traial& traial, Event&) const override
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
	rate_event(const Property&, Traial&, Event&) const override
		{ throw_FigException("TODO: implement!"); }
};

} // namespace fig

#endif // SIMULATIONENGINESFE_H
