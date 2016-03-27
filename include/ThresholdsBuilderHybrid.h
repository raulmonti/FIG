//==============================================================================
//
//  ThresholdsBuilderHybrid.h
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

#ifndef THRESHOLDSBUILDERHYBRID_H
#define THRESHOLDSBUILDERHYBRID_H

#include <ThresholdsBuilderFixed.h>
#include <ThresholdsBuilderSMC.h>


namespace fig
{

/**
 * @brief "Hybrid builder" of importance thresholds.
 *
 *        This member of the ThresholdsBuilder family combines adaptive
 *        techniques, studying the semantics of the user model, with fixed
 *        thresholds selection based on the splitting value chosen by the user.
 *        The goal is to ensure termination of the thresholds building routine,
 *        resorting to a fixed, semantics-oblivious heuristic whenever the
 *        adaptive algorithms fail to terminate within predefined bounds.
 *        The resulting number of thresholds built is a random variable of
 *        the probability of reaching the highest ImportanceValue, also
 *        influenced by the user-specified splitting value.
 */
class ThresholdsBuilderHybrid : public ThresholdsBuilderFixed,
								public ThresholdsBuilderSMC
{
public:

	/// Ctor
	ThresholdsBuilderFixed() : ThresholdsBuilder("hyb") {}

	inline bool adaptive() const noexcept override final { return true; }

	std::vector< ImportanceValue >
	build_thresholds(const unsigned& splitsPerThreshold,
					 const ImportanceFunction& impFun) override final;

	/// @todo TODO implement build_thresholds():
	///       run SMC's "build_thresholds_vector()" and catch exceptions.
	///       If it does fail then resort to fixed thresholds building
	///       for what's left of the ImportancValues, i.e. starting from
	///       the last threshold found my SMC create one thresholds for every
	///       (or every other) ImportanceValue until the max_value().
};

} // namespace fig

#endif // THRESHOLDSBUILDERHYBRID_H
