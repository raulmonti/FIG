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

// C++
#include <vector>
// FIG
#include <ThresholdsBuilder.h>
#include <core_typedefs.h>


namespace fig
{

class Traial;

/**
 * @brief Asbtract base "adaptive builder" of importance thresholds.
 *
 *        Adaptive threshold builders take into consideration the semantics
 *        of the user model to choose the (precomputed) \ref ImportanceValue
 *        "importance values" which will play the role of thresholds.<br>
 *        In general the final resulting number of thresholds built is a
 *        random variable of the probability of reaching the highest
 *        ImportanceValue provided.
 *
 * @see ThresholdsBuilder
 * @see ThresholdsBuilderFixed
 */
class ThresholdsBuilderAdaptive : public virtual ThresholdsBuilder
{
public:

	/// Vector of references to Traial instances taken from the TraialPool
	typedef  std::vector< fig::Reference< fig::Traial > >  TraialsVec;

public:

	/// Default max number of thresholds
	static constexpr size_t MAX_NUM_THRESHOLDS = 200ul;

protected:

	/// Min number of pilot runs to launch for finding thresholds
	static const unsigned MIN_N;

	/// Max number of pilot runs to launch for finding thresholds
	static const unsigned MAX_N;

	/// Number of simulations to launch for each new threshold construction
	unsigned n_;

	/// Number of surviving simulations to consider (less than n_)
	unsigned k_;

	/// Thresholds importance values
	ImportanceVec thresholds_;

	/// Allow derived classes to halt computations via parallel threads
	bool halted_;

public:

	/// Ctor
	ThresholdsBuilderAdaptive(const std::string& name = "",
							  const unsigned& n = 0u,
							  const unsigned& k = 0u);

	inline bool adaptive() const noexcept override { return true; }

	/// Stub to build_thresholds(const unsigned&, const ImportanceFunction&, const float&, const unsigned&)
	/// for automatically computed values of 'p' and 'n'
	inline ImportanceVec
	build_thresholds(const unsigned& splitsPerThreshold,
					 const ImportanceFunction& impFun,
					 const PostProcessing&) override
		{ return build_thresholds(splitsPerThreshold, impFun, 0.0f, 0u); }

	/// Implement ThresholdsBuilder::build_thresholds() using 'p' as the
	/// adaptive probability of threshold level-up and running 'n' independet
	/// simulations for the selection of each threshold
	ImportanceVec
	build_thresholds(const unsigned& splitsPerThreshold,
					 const ImportanceFunction& impFun,
					 const float& p,
					 const unsigned& n);

protected:  // Utils for the class and its kin

	/**
	 * @brief Build thresholds based on given importance function
	 *
	 *        Build a thresholds-to-importance map as described in
	 *        ThresholdsBuilder::build_thresholds(), saving it in the
	 *        internal vector 'thresholds_'<br>
	 *        As a result the states corresponding to the i-th threshold level
	 *        are those to which 'impFun' assigns an ImportanceValue between
	 *        the values at positions i (included) and i+1 (not included)
	 *        of the resulting 'thresholds_'
	 *
	 * @param impFun ImportanceFunction with internal
	 *               \ref ImportanceFunction::has_importance_info()
	 *               "importance information" to use for the task
	 *
	 * @note The size of thresholds_ will be   <br>
	 *       == 1 + number of threshold levels <br>
	 *       == 2 + number of thresholds built
	 * @note The first value in thresholds_ == initial state importance
	 * @note The last  value in thresholds_ > impFun.max_value()
	 *
	 * @throw FigException if thresholds building failed
	 */
	virtual void
	build_thresholds_vector(const ImportanceFunction& impFun) = 0;

	/**
	 * @brief Choose values for n_ and k_ depending on the nature of the Module
	 *        (states and transitions space size) and the simulation.
	 * @param numStates      Size of the concrete state space in the Module
	 * @param numTrans       Number of (symbolic) transitions in the Module
	 * @param maxImportance  Maximum ImportanceValue computed
	 * @param splitsPerThr   Number of splits upon a threshold level-up
	 */
	virtual void tune(const size_t& numTrans,
	                  const ImportanceValue& maxImportance,
	                  const unsigned& splitsPerThr);

	/**
	 * Get initialized Traial instances
	 * @param numTraials Number of traials to retrieve from the TraialPool
	 * @param impFun     ImportanceFunction with \ref ImportanceFunction::has_importance_info()
	 *                   "importance info" to use for initialization
	 * @return std::vector of references to initialized Traial instances
	 */
	TraialsVec
	get_traials(const unsigned& numTraials,
				const fig::ImportanceFunction& impFun);
};

} // namespace fig

#endif // THRESHOLDSBUILDERADAPTIVE_H
