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
 *        by the "useful range" of the ImportanceFunction (i.e. the difference
 *        between the max value and the initial value), the global effort
 *        selected by the user, and the post-processing applied to the
 *        importance values after their computation (if any).
 *
 * @see ThresholdsBuilder
 * @see ThresholdsBuilderAdaptive
 */
class ThresholdsBuilderFixed : public virtual ThresholdsBuilder
{
protected:

	/// Minimal importance range (ifun.maxVal() - ifun.minVal())
	/// If there are less values avaliable then every ImportanceValue
	/// above ifun.minVal() will be considered a threshold.
	const ImportanceValue MIN_IMP_RANGE;

	/// The chosen stride_ will be expanded times the ceiling of
	/// '(impFun.max_value() - impFun.initial_value()) / EXPAND_EVERY'
	/// @note stride_ is also affected by the global effort and
	///       the postProcessing specified in build_thresholds()
	const ImportanceValue EXPAND_EVERY;

	/// Global effort used during simulations
	unsigned globEff_;

	/// PostProcessing applied to the importance function values,
	/// which may affect the distance between values and is hence
	/// relevant during a non-adaptive selection of thresholds.
	PostProcessing postPro_;

	/// Number of \ref ImportanceValue "importance values" to group in a
	/// single threshold level. So for instance stride==2 means there will be
	/// two importance values per threshold level, i.e. a threshold will be set
	/// every two importance values.
	/// @note This is automatically updated during build_thresholds() according
	///       to the global effort and the details of the ImportanceFunction
	ImportanceValue stride_;

public:

	/// Data & default ctor
	ThresholdsBuilderFixed(ImportanceValue minImpRange = 3,
	                       ImportanceValue expandEvery = 64);

	inline bool adaptive() const noexcept override { return false; }

	bool uses_global_effort() const noexcept override { return true; }

	/// Register the post-processing and the global effort (if any)
	/// @param pp PostProcessing that will be applied to the ImportanceValue s
	/// @param ge Global splitting/effort to use in all ("threshold-") levels
	void setup(const PostProcessing& pp,
	           std::shared_ptr<const Property>,
	           const unsigned ge = 2ul) override;

	ThresholdsVec
	build_thresholds(std::shared_ptr<const ImportanceFunction> impFun) override;

public:  // Accessors

	/// @copydoc stride_
	const ImportanceValue& stride() const noexcept;

	/// @copydoc MIN_IMP_RANGE
	const ImportanceValue& min_imp_range() const noexcept;

protected:  // Utils for the class and its kin

	/// Choose a stride based on all information available
	/// @note Relies on data previously set by setup()
	virtual ImportanceValue choose_stride(const size_t& impRange) const;

	/**
	 * Choose threshold and store them in given ImportanceVec
	 *
	 * Starting 'margin' places above the \ref ImportanceFunction::initial_value()
	 * "initial importance", choose thresholds considering 'stride' and the
	 * 'strideType', and store them in the 'thresholds' vector following the
	 * format described in ThresholdsBuilder::build_thresholds().
	 *
	 * @param impFun The ImportanceFunction with internal
	 *               \ref ImportanceFunction::has_importance_info()
	 *               "importance information" to use for the task
	 * @param thresholds Where the thresholds-to-importance map will be built
	 * @param margin Start 'margin' importance values above the initial import.
	 * @param stride Number of importance values to jump per threshold
	 *
	 * @note Any previous content in 'thresholds' is left untouched:
	 *       the vector is resized and the thresholds selection is stored
	 *       past the last valid element of 'thresholds' as it was given.
	 */
	void
	build_thresholds(const ImportanceFunction& impFun,
					 const unsigned& margin,
					 const unsigned& stride,
					 ImportanceVec& thresholds);
};

} // namespace fig

#endif // THRESHOLDSBUILDERFIXED_H
