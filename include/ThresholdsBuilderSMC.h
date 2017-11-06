//==============================================================================
//
//  ThresholdsBuilderSMC.h
//
//  Copyleft 2016-
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

#ifndef THRESHOLDSBUILDERSMC_H
#define THRESHOLDSBUILDERSMC_H

#include <ThresholdsBuilderAdaptiveSimple.h>


namespace fig
{

class ModuleNetwork;

/**
 * @brief ThresholdsBuilder implementing Sequential Monte Carlo (SMC)
 *
 *        SMC is an <i>adaptive</i> thresholds building technique deviced and
 *        perfected by Cerou, Del Moral, Furon and Guyader in the article
 *        "Sequential Monte Carlo for rare event estimation" from 2011.
 *        It could be regarded as a statistically improved version of
 *        Adaptive Multilevel Splitting, the orginal algorithm published by
 *        Cerou and Guyader in 2007.<br>
 *        Just like AMS, given a state space and an importance function on it
 *        SMC aims to choose importance values as "thresholds" trying to make
 *        all the partial probabilities 'P_i' roughly equal. SMC differs
 *        from AMS in the way the internal Monte Carlo simulations are handled,
 *        improving the statistical virtues of the algorithm's iterations.
 *
 * @see ThresholdsBuilderAdaptiveSimple
 * @see ThresholdsBuilderAMS
 */
class ThresholdsBuilderSMC : public ThresholdsBuilderAdaptiveSimple
{
	/// Min simulation length (in # of jumps) to find new thresholds
	static constexpr unsigned MIN_SIM_EFFORT = 1u<<6u;
	/// Max simulation length (in # of jumps) to find new thresholds
	static constexpr unsigned MAX_SIM_EFFORT = 1u<<9u;

	/// Min # of failures allowed when searching for a new threshold
	static constexpr unsigned MIN_NUM_FAILURES = 5u;
	/// Max # of failures allowed when searching for a new threshold
	static constexpr unsigned MAX_NUM_FAILURES = 7u;

public:

	ThresholdsBuilderSMC();

protected:  // Utils for the class and its kin

	void build_thresholds_vector(const ImportanceFunction& impFun) override;

	void tune(const size_t& numTrans,
			  const ImportanceValue& maxImportance,
	          const unsigned& splitsPerThr) override;
};

} // namespace fig

#endif // THRESHOLDSBUILDERSMC_H
