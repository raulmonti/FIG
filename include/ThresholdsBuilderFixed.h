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
 *        between the max value and the initial value), the splitting selected
 *        by the user, and the post-processing applied to the importance values
 *        after their computation (if any).
 *
 * @see ThresholdsBuilder
 * @see ThresholdsBuilderAdaptive
 */
class ThresholdsBuilderFixed : public virtual ThresholdsBuilder
{
protected:

	/// Number of \ref ImportanceValue "importance values" to group in a
	/// single threshold level. So for instance stride==2 means there will be
	/// two importance values per threshold level, i.e. a threshold will be set
	/// every two importance values.
	/// @note This is automatically updated during build_thresholds() according
	///       to the splitting and the details of the ImportanceFunction
	ImportanceValue stride_;

	/// Minimal importance range (ifun.maxVal() - ifun.minVal())
	/// If there are less values avaliable then every ImportanceValue
	/// above ifun.minVal() will be considered a threshold.
	ImportanceValue minImpRange_;

	/// The chosen stride_ will be expanded times the ceiling of
	/// '(impFun.max_value() - impFun.initial_value()) / expandEvery_'
	/// @note stride_ is also affected by the splitting and
	///       the postProcessing specified in build_thresholds()
	ImportanceValue expandEvery_;

	/// Post-processing applied to the importance values after the last
	/// importance assessment, if any.
	PostProcessing postProcessing_;

public:

	/// Default ctor
	ThresholdsBuilderFixed() : ThresholdsBuilder("fix"),
							   stride_(1),
							   minImpRange_(5),
							   expandEvery_(42),
							   postProcessing_(std::make_pair("",.0f))
		{ /* Not much to do around here */ }

	inline bool adaptive() const noexcept override { return false; }

	std::vector< ImportanceValue >
	build_thresholds(const unsigned& splitsPerThreshold,
					 const ImportanceFunction& impFun,
					 const PostProcessing& postProcessing) override;

public:  // Accessors

	/// @copydoc stride_
	const ImportanceValue& stride() const noexcept;

	/// @copydoc minImpRange_
	const ImportanceValue& min_imp_range() const noexcept;

protected:  // Utils for the class and its kin

	/**
	 * Choose threshold and store them in given ImportanceVec
	 *
	 * Starting 'margin' places above the \ref ImportanceFunction::initial_value()
	 * "initial importance", choose thresholds considering 'stride' and the
	 * current postProcessing_, and store them in the 'thresholds' vector
	 * following the format described in ThresholdsBuilder::build_thresholds().
	 *
	 * @param impFun The ImportanceFunction with internal
	 *               \ref ImportanceFunction::has_importance_info()
	 *               "importance information" to use for the task
	 * @param thresholds Where the thresholds-to-importance map will be built
	 * @param margin How many importance values above the initial state's
	 *               importance value will the construction start
	 * @param stride Number of importance values to jump per threshold
	 *
	 * @note The 'thresholds' vector is a parameter instead of a return value
	 *       for ease of use by derived classes, e.g. ThresholdsBuilderHybrid
	 */
	void
	build_thresholds(const ImportanceFunction& impFun,
					 ImportanceVec& thresholds,
					 const unsigned& margin,
					 const unsigned& stride);

	/// Choose stride based solely on the splitting selected by the user
	virtual unsigned
	choose_stride(const unsigned& splitsPerThreshold) const;

	/// Choose stride based on all information available,
	/// including internal info like expandEvery_ and postProcessing_
	virtual unsigned
	choose_stride(const unsigned& splitsPerThreshold,
				  const unsigned& impRange) const;
};

} // namespace fig

#endif // THRESHOLDSBUILDERFIXED_H
