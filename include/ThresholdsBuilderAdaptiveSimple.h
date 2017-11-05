//==============================================================================
//
//  ThresholdsBuilderAdaptiveSimple.h
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

#ifndef THRESHOLDSBUILDERADAPTIVESIMPLE_H
#define THRESHOLDSBUILDERADAPTIVESIMPLE_H

// C++
#include <vector>
// FIG
#include <ThresholdsBuilderAdaptive.h>
#include <core_typedefs.h>


namespace fig
{

/**
 * @brief Adaptive threshold builders with one global splitting/effort
 *        for all levels, based on the theory of Guyader & Cérou
 *        (<i>Adaptive Multilevel Splitting</i> and <i>Sequential Monte Carlo</i>)
 *
 * @see ThresholdsBuilderAdaptive
 * @see ThresholdsBuilderFixed
 */
class ThresholdsBuilderAdaptiveSimple : public virtual ThresholdsBuilderAdaptive
{
protected:

	/// Number of surviving simulations per iteration,
	/// always less than ThresholdsBuilderAdaptive::n_
	unsigned k_;

	/// Thresholds importance values (there is one global effort for all levels)
	ImportanceVec thresholds_;

public:

	/// Ctor
	ThresholdsBuilderAdaptiveSimple(const std::string& name = "",
	                                const unsigned& n = 0u,
	                                const unsigned& k = 0u);

	/// Stub to build_thresholds(const ImportanceFunction&, const unsigned&, const float&, const unsigned&)
	/// for automatically computed values of 'p' and 'n'
	inline ThresholdsVec
	build_thresholds(const ImportanceFunction& impFun,
	                 const PostProcessing&,
	                 const unsigned& globalEffort) override
	    { return build_thresholds(impFun, globalEffort, 0.0f, 0u); }

	/// Implement ThresholdsBuilder::build_thresholds() using 'p' as the
	/// adaptive probability of threshold level-up and running 'n' independet
	/// simulations for the selection of each threshold
	ThresholdsVec
	build_thresholds(const ImportanceFunction& impFun,
	                 const unsigned& globalEffort,
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
};

} // namespace fig

#endif // THRESHOLDSBUILDERADAPTIVESIMPLE_H
