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

// C++
#include <chrono>
// FIG
#include <ThresholdsBuilderFixed.h>
#include <ThresholdsBuilderSMC.h>


namespace fig
{

/**
 * @brief "Hybrid builder" of importance thresholds.
 *
 *        This member of the ThresholdsBuilder family combines adaptive
 *        techniques, studying the semantics of the user model, with fixed
 *        thresholds selection based on the splitting value chosen by the user.<br>
 *        The goal is to ensure termination of the thresholds building routine,
 *        resorting to a fixed, semantics-oblivious heuristic whenever the
 *        adaptive algorithms fail to terminate within predefined bounds.
 *        The resulting number of thresholds built is a random variable of
 *        the probability of reaching the highest ImportanceValue, also
 *        influenced by the user-specified splitting value.
 *
 * @see ThresholdsBuilder
 * @see ThresholdsBuilderFixed
 * @see ThresholdsBuilderAdaptive
 */
class ThresholdsBuilderHybrid : public ThresholdsBuilderFixed,
                                public ThresholdsBuilderSMC

// TODO: create new class "ThresholdsBuilderAdaptive"
// TODO: ThresholdBuilderES will inherit from (the new) ThresholdsBuilderAdaptive
// TODO: ThresholdsBuilderHybrid will inherit from  ThresholdBuilderFixed and (the new) ThresholdsBuilderAdaptive

{
public:

	/// Execution time (minutes) granted to the adaptive technique.<br>
	/// If computations don't finish within this limit, resort to a fixed
	/// technique to choose the missing thresholds "instantaneously".
	static constexpr std::chrono::minutes ADAPTIVE_TIMEOUT = std::chrono::minutes(5u);

public:

	/// Default ctor
	ThresholdsBuilderHybrid() : ThresholdsBuilder("hyb"),
								ThresholdsBuilderFixed(6,32)  // formerly: (6,16)
		{ /* Not much to do around here */ }

	inline bool adaptive() const noexcept override { return true; }

	ImportanceVec
	build_thresholds(const unsigned& splitsPerThreshold,
					 const ImportanceFunction& impFun,
					 const PostProcessing& postProcessing) override;

protected:

	ImportanceValue
	choose_stride(const size_t& impRange,
				  const unsigned& splitsPerThreshold,
				  const PostProcessing& postProcessing) const override;
};

} // namespace fig

#endif // THRESHOLDSBUILDERHYBRID_H
