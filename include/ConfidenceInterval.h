//==============================================================================
//
//  ConfidenceInterval.h
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


#ifndef CONFIDENCEINTERVAL_H
#define CONFIDENCEINTERVAL_H

#include <algorithm>  // std::max(), std::min()


namespace fig
{

/**
 * Abstract base class for the confidence interval which is to be built
 * around the probability's estimated value.
 */
class ConfidenceInterval
{
public:  // Attributes: CI fingerprint

	/// Desired precision half-width
	const double errorMargin;

	/// Is the precision a percentage of the estimate?
	const bool percent;

	/// Desired confidence coefficient
	const double confidence;

	/// Probit value for chosen confidence
	const double quantile;

private:  // Attributes: estimation thus far

	/// Number of samples fed so far via update()
	long numSamples_;

	/// Property's point value estimate
	double estimate_;

	/// Variance corresponding to the current estimation
	double variance_;

	/// Semi-precision corresponding to the current estimation
	double halfWidth_;

public:  // Attributes: estimation correction factors

	/// Minimum required number of "successfull" simulations
	double statOversample;

	/// Calibration of the relative weight of simulation runs
	double varCorrection;

public:  // Ctors/Dtor

	/**
	 * Data ctor
	 * @param confidence       Interval's confidence coefficient ∈ (0.0, 1.0)
	 * @param precision        Interval's desired full width > 0.0
	 * @param dynamicPrecision Is the precision a percentage of the estimate?
	 * @throw FigException if either 'confidence' or 'precision' is invalid
	 */
	ConfidenceInterval(double confidence, double precision, bool dynamicPrecision = false);

public:  // Utils

	/// Update CI with a new value estimated for the property under study
	virtual void update(const double& newEstimate) = 0;

	/**
	 * Do we have enough measurements to apply the theory?
	 *
	 * Each kind of confidence iterval follows some statistical theory, e.g.
	 * the Central Limit Theorem. In all cases there's a minimum number of
	 * samples to be considered before the theory applies. This method says
	 * whether that minimum has been reached.
	 *
	 * @note This is necessary yet <i>insufficient</i> to guarantee
	 *       the satisfaction of the confidence criteria.
	 *
	 * @see is_valid()
	 */
	virtual bool min_samples_covered() = 0;

	/**
	 * Does current estimation satisfy the interval's creation criteria?
	 *
	 * If the minimum # of samples to satisfy the theory assumptions hasn't
	 * been covered yet, this method will return <b>false</b>. Ohterwise the
	 * result will depend on wether the desired precision has been reached.
	 *
	 * @return Whether the desired precision has been reached for the
	 *         confidence coefficient requested
	 *
	 * @see min_samples_covered()
	 */
	bool is_valid() const noexcept;

	/// @copydoc numSamples_
	inline const long& num_samples() const noexcept { return numSamples_; }

	/// @copydoc estimate_
	inline const double& point_estimate() const noexcept { return estimate_; }

	/// @copydoc variance_
	inline const double& estimation_variance() const noexcept { return variance_; }

	/// Theoretical width for creation's confidence coefficient
	/// @copydoc value_simulations_
	inline double precision() const noexcept
		{ return 2.0 * errorMargin * (percent ? estimate_ : 1.0); }

	/// Achieved width for requested confidence coefficient
	/// @copydoc time_simulations_
	virtual double precision(double confidence) const = 0;

	/// Theoretical lower limit for creation's confidence coefficient
	/// @copydoc value_simulations_
	inline double lowerLimit() const
		{ return std::max(0.0, estimate_ - precision()/2.0); }

	/// Achieved lower limit for requested confidence coefficient
	/// @copydoc time_simulations_
	inline double lowerLimit(double confidence) const
		{ return std::max(0.0, estimate_ - precision(confidence)/2.0); }

	/// Theoretical upper limit for creation's confidence coefficient
	/// @copydoc value_simulations_
	inline double upperLimit() const
		{ return std::min(1.0, estimate_ + precision()/2.0); }

	/// Achieved upper limit for requested confidence coefficient
	/// @copydoc time_simulations_
	inline double upperLimit(double confidence) const
		{ return std::min(1.0, estimate_ + precision(confidence)/2.0); }

protected:

	/**
	 * Compute the quantile of given confidence coefficient.
	 *
	 * Given a desired confidence coefficient 'cc', and the corresponding
	 * significance level 'a' := 1-cc, the confidence interval for an
	 * estimated sample mean 'x' is computed as:
	 * \code
	 * x ± z_a * s / sqrt(n)
	 * \endcode
	 * where 'n' is the number of observations, 's' is the sample standard
	 * deviation, and 'z_a' is the "1-a/2" quantile of a unit normal variate,
	 * i.e. a random variable with standard normal distribution. The quantile
	 * is the inverse CDF of the unit normal variate, usually called
	 * <a href="http://en.wikipedia.org/wiki/Probit">probit function</a>.
	 *
	 * @param cc Confidence coefficient for the desired confidence interval
	 *
	 * @return Confidence coefficient's quatile
	 *
	 * @note Since this uses the
	 *       <a href="http://goo.gl/9jtSU3">Central Limit Theorem</a>,
	 *       it is only valid for "sufficiently large" samples,
	 *       typically n > 30.
	 *
	 * @throw FigException
	 */
	static double confidence_quantile(const double& cc);

private:

	/// @note Typically used for "value simulations", viz. when estimations
	///       finish as soon as certain confidence criteria is met.
	static bool value_simulations_;  // defined only to copy the doc!

	/// @note Typically used for "time simulations", viz. when estimations
	///       run indefinitely until they're externally interrupted.
	static bool time_simulations_;   // defined only to copy the doc!
};

} // namespace fig

#endif // CONFIDENCEINTERVAL_H

