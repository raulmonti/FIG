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
#include <PropertyTransient.h>
#include <PropertyRate.h>
#include <Traial.h>


namespace fig
{

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
	/// Max # steps allowed for each internal Fixed Effort simulation
	static constexpr decltype(Traial::numLevelsCrossed) MAX_FE_SIM_LEN = 10000ul;

protected:

	/// Property to estimate, for which the thresholds will be selected
	std::shared_ptr< const Property > property_;

	/// Importance function currently built
	const ImportanceFunction* impFun_;

public:

	/// Data & default ctor
	/// @param n Number of pilot simulations used per importance level, see Budde et al.
	ThresholdsBuilderES(const size_t& n = (1ul<<8ul));

	/// Register the Property being estimated, which may affect
	/// the internal Fixed Effort runs of the thresholds selection algorithm.
	void
	setup(const PostProcessing&,
	      std::shared_ptr<const Property>,
	      const unsigned) override;

	ThresholdsVec
	build_thresholds(const ImportanceFunction&) override;

private:  // Class utils

	/// Probe the model and the importance space to determine reachable
	/// importance values among which thresholds could be selected.
	/// @return Ordered vector of reachable importance values
	/// @note Some *very high importance values* may be missing from the result
	///       although they might be theoretically reachable
	ImportanceVec
	reachable_importance_values() const;

	/**
	 * @brief Run Fixed Effort to roughly estimate level-up probabilities
	 *
	 *        Run a "fine" Fixed Effort where the threshold-levels are all
	 *        adjacent (reachable) importance values.<br>
	 *        The probabilities of going up from a reachable importance value
	 *        to the next are stored in the vector of floats.
	 *
	 * @param reachableImportanceValues Result from reachable_importance_values()
	 * @param traials Traials to perform the FE simulations
	 * @param Pup     Vector to fill with the level-up probabilities
	 *
	 * @note The effort used per level equals the number of traials provided
	 * @note We currently disregard rare events below max importance,
	 *       and we force Fixed Effort to reach the max importance value.<br>
	 *       This can be generalised to have "still successful Fixed Effort runs"
	 *       when they don't reach the next importance value but hit a rare event.
	 */
	void
	FE_for_ES(const ImportanceVec& reachableImportanceValues,
	          TraialsVec &traials,
	          std::vector<float> &Pup) const;

	/**
	 * @brief Artificial selection of thresholds when the algorithm fails
	 *
	 *        Last resort for when Expected Success can't reach the max imp:
	 *        this routine selects values for the effort of all levels
	 *        above the last successful level inspected by the ES.
	 *
	 * @param Pup Vector with the level-up probabilities that Expected Success
	 *            could compute (this will guide the artificial selection)
	 */
	void
	artificial_thresholds_selection(std::vector< float >& Pup) const;

	/// @brief Event-watcher for the internal Fixed Effort simulations
	/// @details Interpret and mark the events triggered by a Traial
	///          in its most recent traversal through the system model.
	inline bool
	FE_watcher(const Property& property, Traial& traial, Event&) const
	    {
		    const ImportanceValue newImp = impFun_->importance_of(traial.state);
			traial.depth -= newImp - traial.level;
			traial.level = newImp;
			traial.numLevelsCrossed++;  // encode here the # steps taken
			return /* level-up:     */ traial.depth < 0 ||
			       /* sim too long: */ traial.numLevelsCrossed > MAX_FE_SIM_LEN ||
			       /* stop event:   */ property.is_stop(traial.state);
	    }
};

} // namespace fig

#endif // THRESHOLDSBUILDERES_H
