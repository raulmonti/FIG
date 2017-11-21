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
class ThresholdsBuilderAdaptiveSimple : public ThresholdsBuilderAdaptive
{
protected:

	/// Global effort used during simulations
	unsigned globEff_;

	/// Number of surviving simulations per iteration,
	/// always less than ThresholdsBuilderAdaptive::n_
	unsigned k_;

	/// Thresholds importance values (there is one global effort for all levels)
	/// @note Intentionally obscures ThresholdsBuilderAdaptive::thresholds_
	ImportanceVec thresholds_;

public:

	/// Data & default ctor
	ThresholdsBuilderAdaptiveSimple(const unsigned& n = 0u,
	                                const unsigned& k = 0u);

	bool uses_global_effort() const noexcept override { return true; }

	/**
	 * @brief Register the global effort used for simulations
	 *
	 *        <i>Global effort</i> means different things depending on
	 *        the type of importance splitting used:
	 *        <ul>
	 *        <li>For RESTART it means the same splitting value is used
	 *            in all thresholds, i.e. @a globalEffort-1 replicas
	 *            will be created in a level-up;</li>
	 *        <li>For Fixed Effort it means launching the same number
	 *            of simulations (namely @a globalEffort)
	 *            in all ("threshold-") levels.</li>
	 *        </ul>
	 */
	void
	setup(const PostProcessing&,
	      std::shared_ptr<const Property>,
	      const unsigned globalEffort) override;

	ThresholdsVec
	build_thresholds(const ImportanceFunction& impFun) override;

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
	 * @note The size of the resulting vector  <br>
	 *       == 1 + number of threshold levels <br>
	 *       == 2 + number of thresholds built
	 * @note The first ImportanceValue in the map == initial state importance
	 * @note The last  ImportanceValue in the map == 1 + impFun.max_importance()
	 *
	 * @throw FigException if thresholds building failed
	 */
	virtual void
	build_thresholds_vector(const ImportanceFunction& impFun) = 0;

	/// Choose values for n_ and k_
	/// @copydetails ThresholdsBuilderAdaptive::tune()
	void
	tune(const size_t&, const ImportanceValue&, const unsigned&) override;
};

} // namespace fig

#endif // THRESHOLDSBUILDERADAPTIVESIMPLE_H