//==============================================================================
//
//  ThresholdsBuilderFixed.h
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

#ifndef THRESHOLDSBUILDERFIXED_H
#define THRESHOLDSBUILDERFIXED_H

#include <ThresholdsBuilder.h>
#include <core_typedefs.h>


namespace fig
{

/**
 * @brief "Fixed builder" of importance thresholds.
 *
 *        In order to choose the thresholds among the \ref ImportanceValue
 *        "importance values" of the ImportanceFunction provided, this class
 *        uses a policy which is oblivious of the underlying user model.<br>
 *        The final resulting number of thresholds built is fully determined
 *        by the max_value() of the ImportanceFunction and the splitting value
 *        chosen by the user.
 *
 * @see ThresholdsBuilder
 * @see ThresholdsBuilderAdaptive
 */
class ThresholdsBuilderFixed : public virtual ThresholdsBuilder
{
public:

	/// Default ctor
	ThresholdsBuilderFixed() : ThresholdsBuilder("fix") {}

	inline bool adaptive() const noexcept override { return false; }

	std::vector< ImportanceValue >
	build_thresholds(const unsigned& splitsPerThreshold,
					 const ImportanceFunction& impFun) override;

protected:  // Utils for the class and its kin

	/**
	 * Start building the thresholds 'margin' values of importance above the
	 * \ref ImportanceFunction::initial_value() "initial importance", setting
	 * thresholds at 'stride' importance values of distance.
	 *
	 * @param impFun The ImportanceFunction with internal
	 *               \ref ImportanceFunction::has_importance_info()
	 *               "importance information" to use for the task
	 * @param thresholds Vector where the importance_to_thresholds map
	 *                   will be built
	 * @param margin How many importance values above the initial state's
	 *               importance value will the construction take place
	 * @param stride Number of importance values to jump per threshold
	 */
	void
	build_thresholds(const ImportanceFunction& impFun,
					 ImportanceVec& thresholds,
					 const unsigned& margin,
					 const unsigned& stride);
};

} // namespace fig

#endif // THRESHOLDSBUILDERFIXED_H
