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
protected:

	/// Property to estimate, for which the thresholds will be selected
	std::shared_ptr<const Property> property_;

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

public:  // Traial observers/updaters
//	inline bool transient_sim(const PropertyTransient)
};

} // namespace fig

#endif // THRESHOLDSBUILDERES_H
