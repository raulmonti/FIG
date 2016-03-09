//==============================================================================
//
//  ThresholdsBuilderAdaptive.h
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

#ifndef THRESHOLDSBUILDERADAPTIVE_H
#define THRESHOLDSBUILDERADAPTIVE_H

#include <ThresholdsBuilder.h>


namespace fig
{

/**
 * @brief Asbtract base "adaptive builder" of importance thresholds.
 *
 *        Adaptive threshold builders take into consideration the semantics
 *        of the user model to choose the (precomputed) \ref ImportanceValue
 *        "importance values" which will play the role of thresholds.
 *        In general the final resulting number of thresholds built is a
 *        random variable of the probability of reaching the highest
 *        ImportanceValue provided.
 */
class ThresholdsBuilderAdaptive : public ThresholdsBuilder
{
protected:

	/// Number of simulations to launch for each new threshold construction
	unsigned n_;

	/// Number of surviving simulations to consider (less than n_)
	unsigned k_;

public:

	inline bool adaptive() const noexcept override final { return true; }

	/// Stub to build_thresholds(const unsigned&, const unsigned&, const unsigned&, const ImportanceFunction&)
	/// for automatically computed values of 'n' and 'k'
	inline std::vector< ImportanceValue >
	build_thresholds(const unsigned& splitsPerThreshold,
					 const ImportanceFunction& impFun) override final
		{ build_thresholds(0u, 0u, splitsPerThreshold, impFun); }

	/// Implement ThresholdsBuilder::build_thresholds() using k/n
	/// as the adaptive probability of threshold level-up
	virtual std::vector< ImportanceValue >
	build_thresholds(const unsigned& n,
					 const unsigned& k,
					 const unsigned& splitsPerThreshold,
					 const ImportanceFunction& impFun) = 0;

protected:

	/**
	 * @brief Choose values for n_ and k_ depending on the nature of the Module
	 *        (states and transitions space size) and the simulation.
	 * @param numStates      Size of the concrete state space in the Module
	 * @param numTrans       Number of (symbolic) transitions in the Module
	 * @param maxImportance  Maximum ImportanceValue computed
	 * @param splitsPerThr   Number of splits upon a threshold level-up
	 */
	void tune(const size_t& numStates,
			  const size_t& numTrans,
			  const ImportanceValue& maxImportance,
			  const unsigned& splitsPerThr);
};

} // namespace fig

#endif // THRESHOLDSBUILDERADAPTIVE_H
