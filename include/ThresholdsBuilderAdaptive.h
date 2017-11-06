//==============================================================================
//
//  ThresholdsBuilderAdaptive.h
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

protected:

	/// Number of pilot simulations to launch for each new threshold construction
	unsigned n_;

	/// Min value for n_
	static const unsigned MIN_N;

	/// Max value for n_
	static const unsigned MAX_N;

	/// Thresholds: the importance values that defines them,
	/// and the effort to perform on each ("threshold-") level
	ThresholdsVec thresholds_;

	/// Allow derived classes to halt computations via parallel threads
	bool halted_;

public:

	ThresholdsBuilderAdaptive(const unsigned& n = 0u);

	inline bool adaptive() const noexcept override { return true; }

	ThresholdsVec
	build_thresholds(const ImportanceFunction&,
	                 const PostProcessing&,
	                 const unsigned& globalEffort = 0u) override = 0;

protected:  // Utils for the class and its kin

	/**
	 * @brief Choose values for internal parameters depending on the user model
	 *        (states and transitions space size) and the simulation.
	 * @param numTrans       Number of (symbolic) transitions in the system model
	 * @param maxImportance  Maximum ImportanceValue computed
	 * @param globalEffort   <i>(Optional)</i> Global splitting/effort per level
	 */
	virtual void tune(const size_t& numTrans,
	                  const ImportanceValue& maxImportance,
	                  const unsigned& globalEffort = 0u);

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
