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

	/// Min value for the number of pilot simulations launched
	/// @see n_
	static const unsigned MIN_N;

	/// Max value for the number of pilot simulations launched
	/// @see n_
	static const unsigned MAX_N;

	/// @copydoc ModelSuite::highVerbosity_
	static bool highVerbosity;

	/// Thresholds: the importance values that define them,
	/// and the effort to perform on each ("threshold-") level
	ThresholdsVec thresholds_;

	/// Allow derived classes to halt computations via parallel threads
	bool halted_;

public:

	/// Data & default ctor
	/// @param n @copydoc n_
	ThresholdsBuilderAdaptive(const unsigned& n = MIN_N);

	inline bool adaptive() const noexcept override { return true; }

	/// @copydoc highVerbosity
	static inline void set_verbosity(bool verbose) noexcept { highVerbosity=verbose; }

	/// @copydoc MIN_N
	static inline unsigned min_n() noexcept { return MIN_N; }

	/// @copydoc MAX_N
	static inline unsigned max_n() noexcept { return MAX_N; }

protected:  // Utils for the class and its kin

	/**
	 * @brief Choose values for internal parameters depending on the model
	 *        (states and transitions space size) and simulation parameters.
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
	 *                   "importance info" to use for (optional) initialization
	 * @param initialise <i>(Optional)</i> Whether the traials should be initialised
	 * @return std::vector of references to initialized Traial instances
	 * @note Remember to return these traials to the TraialPool
	 */
	TraialsVec
	get_traials(const unsigned& numTraials,
	            const fig::ImportanceFunction& impFun,
	            bool initialise = true) const;
};

} // namespace fig

#endif // THRESHOLDSBUILDERADAPTIVE_H
