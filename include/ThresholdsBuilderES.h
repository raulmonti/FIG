//==============================================================================
//
//  ThresholdsBuilderES.h
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

#ifndef THRESHOLDSBUILDERES_H
#define THRESHOLDSBUILDERES_H

#include <ThresholdsBuilderAdaptive.h>
#include <ImportanceFunction.h>
#include <Property.h>
#include <Traial.h>


namespace fig
{

class ModuleNetwork;
class SimulationEngineFixedEffort;

/**
 * @brief ThresholdsBuilder implementing Expected Success (ES)
 *
 *        ES is an <i>adaptive</i> thresholds building technique inspired in
 *        implementations of RESTART by Villén-Altamirano et al.
 *        ES was devised (as implemented here and in the Modest Toolset)
 *        by Budde, D'Argenio, Hartmanns, in "Better Automated Importance
 *        Splitting for Transient Rare Events", SETTA 2017.
 *        <br>
 *        ES exploits the discrete nature of the importance function, as opposed
 *        to \ref ThresholdsBuilderAMS "Adaptive Multilevel Splitting" and
 *        \ref ThresholdsBuilderSMC "Sequential Monte Carlo" which assume a
 *        continuous importance range. ES performs Fixed Effort between every
 *        two adjacent importance values, choosing thresholds so that there is
 *        at least one simulation expected to reach the upper threshold.
 *        This defines the thresholds and also the optimal splitting to perform
 *        in each threshold.
 *
 * @see ThresholdsBuilderAdaptiveSimple
 * @see ThresholdsBuilderAdaptive
 */
class ThresholdsBuilderES : public ThresholdsBuilderAdaptive
{
	/// Min # pilot runs launched in the internal Fixed Effort
	static constexpr size_t MIN_NSIMS = (1ul)<<(8ul);
	/// Max # pilot runs launched in the internal Fixed Effort
	static constexpr size_t MAX_NSIMS = (1ul)<<(10ul);

	/// Min # steps allowed for each internal Fixed Effort pilot run
	static constexpr decltype(Traial::numLevelsCrossed) MIN_SIM_LEN = (1ul)<<(7ul);
	/// Max # steps allowed for each internal Fixed Effort pilot run
	static constexpr decltype(Traial::numLevelsCrossed) MAX_SIM_LEN = (1ul)<<(10ul);

protected:

	/// #(FE-sims) launched per iteration of the internal Fixed Effort
	size_t nSims_;

	/// #(steps) allowed for each internal Fixed Effort pilot run
	decltype(Traial::numLevelsCrossed) maxSimLen_;

	/// Highest ImportanceValue observed in internal simulations
	ImportanceValue maxImportanceReached_;

	/// Property to estimate, for which the thresholds will be selected
	std::shared_ptr< const Property > property_;

	/// Model currently built
	std::shared_ptr< const ModuleNetwork > model_;

	/// Importance function currently built
	std::shared_ptr< const ImportanceFunction > impFun_;

	/// Simulator for the internal Fixed Effort runs
	std::unique_ptr< SimulationEngineFixedEffort > internalSimulator_;

	/// (Temporal) Map from importance to threshold-levels,
	/// storing the thresholds currently under consideration
	mutable ThresholdsVec currentThresholds_;

public:

	/// Data & default ctor
	/// @param n Number of pilot simulations used per importance level, see Budde et al.
	ThresholdsBuilderES(std::shared_ptr< const ModuleNetwork > model,
						const size_t& n = (1ul<<8ul));

	bool uses_global_effort() const noexcept override final { return false; }

	/// Register the Property being estimated, which may affect
	/// the internal Fixed Effort runs of the thresholds selection algorithm.
	void
	setup(std::shared_ptr<const Property> property, const unsigned) override;

	ThresholdsVec
	build_thresholds(std::shared_ptr<const ImportanceFunction>) override;

private:  // Class utils

	/**
	 * @brief Return (a set of) importance values that simulations can reach.
	 *
	 *        Probe the model and the importance space to determine reachable
	 *        importance values among which thresholds could be selected.
	 *
	 * @param forceRealMax Include in the result, forcefully if necessary,
	 *                     the max value of the current importance function.
	 *
	 * @return Ordered vector of reachable importance values
	 *
	 * @note If \p forceRealMax is \p false, some very high importance values
	 *       may be missing from the result although they might be
	 *       theoretically reachable
	 */
	ImportanceVec
	reachable_importance_values(bool forceRealMax = false);

	/**
	 * @brief Run Fixed Effort to roughly estimate level-up probabilities
	 *
	 *        Do a Fixed Effort run where the threshold-levels are given
	 *        by the \p reachableImportanceValues.
	 *
	 * @param reachableImportanceValues Result from reachable_importance_values()
	 *
	 * @return Probabilities of going from each reachable importance value to the next
	 *
	 * @note The effort to used per level is read from nSims_
	 * @note We currently disregard rare events below max importance,
	 *       and we force Fixed Effort to reach the max importance value.<br>
	 *       This can be generalised to have "still successful Fixed Effort runs"
	 *       when they don't reach the next importance value but hit a rare event.
	 *
	 * @warning Hardcoded to work with SimulationEngineSFE as internalSimulator_
	 */
	std::vector<float>
	FE_for_ES(const ImportanceVec& reachableImportanceValues);

	/**
	 * @brief Selection/deletion of artificially chosen thresholds
	 *
	 *        When Expected Success can't reach the max importance,
	 *        this routine selects values for the effort of all levels
	 *        above the last successful level inspected by ES.<br>
	 *        When ImportanceValues were artificially selected during internal
	 *        simulations, but ES could finally reach the max importance,
	 *        this routine deletes any artificial threshold that is left.
	 *
	 * @param Pup Vector with the level-up probabilities that Expected Success
	 *            could compute (this guides the artificial selection/deletion)
	 *
	 * @note Selects effort for the \ref ImportanceValue "importance values"
	 *       from \p currentThresholds_ that (according to Pup) where not reached
	 */
	void
	process_artificial_thresholds(std::vector< float >& Pup) const;

	/// @brief Event-watcher for the internal Fixed Effort simulations
	/// @details Interpret and mark the events triggered by a Traial
	///          in its most recent traversal through the system model.
	inline bool
	FE_watcher(const Property& property, Traial& traial, Event&) const
	    {
			const auto newImp = impFun_->importance_of(traial.state);
			traial.depth -= static_cast<short>(current_level_of(newImp)
											   - current_level_of(traial.level));
			traial.level = newImp;
			traial.numLevelsCrossed++;  // encode here the # steps taken
			return /* level-up:     */ traial.depth < 0 ||
			       /* sim too long: */ traial.numLevelsCrossed > maxSimLen_ ||
			       /* stop event:   */ property.is_stop(traial.state);
	    }

	/// @brief Event-watcher for the ImportanceValue space exploration
	/// @details Similar to FE_watcher() but disregard the property
	inline bool
	importance_seeker(const Property&, Traial& traial, Event&) const
		{
			const long newImp = static_cast<long>(impFun_->importance_of(traial.state));
			traial.depth -= static_cast<short>(newImp - static_cast<long>(traial.level));
			traial.level = static_cast<ImportanceValue>(newImp);
			traial.numLevelsCrossed++;  // encode here the # steps taken
			return /* level-up:     */ traial.depth < 0 ||
			       /* sim too long: */ traial.numLevelsCrossed > maxSimLen_;
	    }

	/// Binary search in currentThresholds_ to find the threshold-level
	/// to which \p imp corresponds
	inline long
	current_level_of(const ImportanceValue& imp) const
	    {
			long tlvl(currentThresholds_.size()/2), step(tlvl/2);
			while (imp <  currentThresholds_[tlvl].first ||
			       imp >= currentThresholds_[tlvl+1].first) {
				if (imp < currentThresholds_[tlvl].first)
					tlvl -= step;
				else
					tlvl += step;
				step = std::max(1l, step/2l);
			}
			return std::max(0l, tlvl);
	    }

protected:  // Utils for the class and its kin

	/**
	 * @brief Tune the nature of the internal Fixed Effort pilot runs
	 *
	 *        The goal is to find good thresholds and do it fast.
	 *        The trade-off is between effort spent and quality achieved:
	 *        good thresholds require a lot of long simulations.
	 *        <br>
	 *        There are two parameters to decide on:
	 *        <ul>
	 *        <li>[1] #(FE-sims) to launch per (potential) threshold level, and</li>
	 *        <li>[2] #(steps) allowed to each of these simulations.</li>
	 *        </ul>
	 *        The number of importance values to test as potential thresholds
	 *        has a negative influence on [1],
	 *        because we may have to escalate through a lot of importance levels.
	 *        We disregard any influence of this factor on parameter [2].
	 *        <br>
	 *        The size of the fully composed model, here #(clocks)+#(variables),
	 *        has a negative influence on [2],
	 *        because each simulation step needs to update a lot of things.
	 *        We disregard any influence of this factor on parameter [1].
	 *
	 * @note All parameters are ignored; The relevant information is
	 *       extracted from the current model and importance function built.
	 *
	 * @see ThresholdsBuilderAdaptive::tune()
	 * @see ThresholdsBuilderAdaptiveSimple::tune()
	 */
	void
	tune(const size_t& = 0ul,
	     const ImportanceValue& = 0,
	     const unsigned& = 0u) override;
};

} // namespace fig

#endif // THRESHOLDSBUILDERES_H
