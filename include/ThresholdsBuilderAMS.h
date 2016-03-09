//==============================================================================
//
//  ThresholdsBuilderAMS.h
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

#ifndef THRESHOLDSBUILDERAMS_H
#define THRESHOLDSBUILDERAMS_H

// C++
#include <vector>
// FIG
#include <ThresholdsBuilderAdaptive.h>


namespace fig
{

/**
 * @brief ThresholdsBuilder implementing Adaptive Multilevel Splitting (AMS)
 *
 *        AMS is an <i>adaptive</i> thresholds building technique which takes
 *        its name from an article published by Cerou and Guyader in 2007.
 *        Given a state space and an importance function on it, AMS aims to
 *        locate the thresholds so that all the probabilities 'P_i' are roughly
 *        the same. Here 'P_i' is defined as the conditional probability of a
 *        simulation run traversing the i-th level upwards, that is, going up
 *        the i-th importance threshold having started at the (i-1)-th threshold.
 *
 * @see ThresholdsBuilderAdaptive
 * @see ThresholdsBuilderSMC
 */
class ThresholdsBuilderAMS : public ThresholdsBuilderAdaptive
{
public:

	/// Void ctor
	ThresholdsBuilderAMS();

	std::vector< ImportanceValue >
	build_thresholds(const unsigned& n,
					 const unsigned& k,
					 const unsigned& splitsPerThreshold,
					 const ImportanceFunction& impFun) override;

private:  // Class internal helper functions

	/**
	 * @brief Build thresholds based on given importance function
	 *
	 *        The work is saved in the internal vector 'thresholds_',
	 *        whose i-th position will hold the ImportanceValue chosen
	 *        as the i-th threshold. Also the lowest ImportanceValue of impFun
	 *        is stored in index zero. As a result the states corresponding
	 *        to the j-th threshold level are those to which 'impFun' assigns
	 *        an ImportanceValue between the values at positions j (included)
	 *        and j+1 (not included) of thresholds_.
	 *
	 * @param splitsPerThreshold 1 + Number of simulation-run-replicas upon a
	 *                           "threshold level up" event
	 * @param impFun ImportanceFunction with internal
	 *               \ref ImportanceFunction::has_importance_info()
	 *               "importance information" to use for the task
	 *
	 * @note The resulting size of thresholds_  <br>
	 *       == 1 + number of threshold levels  <br>
	 *       == 2 + number of thresholds built
	 * @note Last value in thresholds_ > impFun.max_importance()
	 *
	 * @throw FigException if thresholds building failed
	 */
	void
	build_thresholds_vector(const unsigned& splitsPerThreshold,
							const ImportanceFunction& impFun);
};

} // namespace fig

#endif // THRESHOLDSBUILDERAMS_H
