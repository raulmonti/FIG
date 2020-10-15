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

#include <string>
#include <algorithm>  // std::max(), std::min()


namespace fig
{

/**
 * @brief Abstract base class for the confidence interval which is to be built
 *        around the probability's estimated value.
 *
 * @details ConfidenceInterval has two roles:
 *          <li>
 *          <ul> offer a common interface to its derived classes,
 *               which should implement confidence intervals from the literature
 *               (e.g. a CI for binomial proportions using Agresti-Coull);
 *          <ul> serve as observer for estimation results,
 *               which can be passed to the user for evaluation.
 *          </li>
 */
class ConfidenceInterval
{
public:  // Attributes: CI fingerprint
	
	/// Some name identifying which kind of CI this is
	const std::string name;

	/// Desired precision half-width
	const double errorMargin;

	/// Is the precision a percentage of the estimate?
	const bool percent;

	/// Desired confidence coefficient
	const double confidence;

	/// Whether the interval will never be considered \ref is_valid() "valid"
	/// @note Intended for time bound simulations which should keep on updating
	///       the interval regardless of the confidence and precision achieved
	const bool alwaysInvalid;

protected:  // Attributes: estimation thus far

	/// Number of samples fed so far via update()
	long numSamples_;

	/// Property's point value estimate
	double estimate_;

	/// @copydoc estimate_ in previous update
	double prevEstimate_;

	/// Variance corresponding to the current estimation
	double variance_;

	/// Probit value for chosen confidence
	double quantile;

	/// Semi-precision corresponding to the current estimation
	double halfWidth_;

protected:  // Attributes: estimation correction factors

	/// Min # of samples needed to cover this interval's theoretical hypothesis
	/// @see min_samples_covered()
	double statOversample_;

	/// Calibration of the relative weight of the experimental samples fed
	double varCorrection_;

public:  // Ctor

	/**
	 * Only data ctor provided (interface role)
	 * @param name             @copybrief name
	 * @param confidence       Interval's confidence coefficient ∈ (0.0, 1.0)
	 * @param precision        Interval's desired full width > 0.0
	 * @param dynamicPrecision Is the precision a percentage of the estimate?
	 * @param neverStop        @copybrief alwaysInvalid
	 * @throw FigException if either 'confidence' or 'precision' is invalid
	 */
	ConfidenceInterval(const std::string& name,
					   double confidence,
					   double precision,
					   bool dynamicPrecision = false,
					   bool neverStop = false);

	virtual ~ConfidenceInterval() {}

public:  // Accessors

	/// @copydoc numSamples_
	inline const long& num_samples() const noexcept { return numSamples_; }

	/// @copydoc estimate_
	inline const double& point_estimate() const noexcept { return estimate_; }

	/// @copydoc variance_
	inline const double& estimation_variance() const noexcept { return variance_; }

	/// @copydoc statOversample_
	/// @deprecated Used by binomial proportion CIs (e.g. Wilson score interval)
	inline const double& statistical_oversampling() const noexcept { return statOversample_; }

	/// @copydoc varCorrection_
	/// @deprecated Used by binomial proportion CIs (e.g. Wilson score interval)
	inline const double& variance_correction() const noexcept { return varCorrection_; }

public:  // Modifyers

	/// Increase the statistical oversampling factor for event counting.
	/// @note Typically needed when rare events can occur in any threshold level
	/// @see statistical_oversampling()
	/// @deprecated Used by binomial proportion CIs (e.g. Wilson score interval)
	virtual void set_statistical_oversampling(const double& statOversamp);

	/// Set the variance correction factor for interval precision computation.
	/// @note Typically needed when rare events can occur in any threshold level
	/// @see variance_correction()
	/// @deprecated Used by binomial proportion CIs (e.g. Wilson score interval)
	virtual void set_variance_correction(const double& varCorrection);

	/**
	 * @brief Update current estimation with a new sample value.
	 * @param newSample Result obtained from the last simulation experiment
	 * @note Considered as one single new value fed into the estimation,
	 *       i.e. only one experiment was run to come up with 'newEstimate'
	 * @throw FigException if detected possible overflow
	 * @see update(const double&, const double&)
	 */
	virtual void update(const double& newSample) = 0;

public:  // Utils

	/**
	 * Do we have enough measurements to apply the theory?
	 *
	 * Each kind of confidence iterval follows some statistical theory, e.g.
	 * the Central Limit Theorem. In all cases there's a minimum number of
	 * samples to be considered before the theory applies. This method says
	 * whether that minimum has been reached.
	 *
	 * @param safeguard Be extra conservative if possible
	 *
	 * @note This is necessary yet <i>insufficient</i> to guarantee
	 *       the satisfaction of the confidence criteria.
	 *
	 * @see is_valid()
	 */
	virtual bool min_samples_covered(bool safeguard) const noexcept = 0;

	/**
	 * Does current estimation satisfy the interval's confidence criteria?
	 *
	 * If the minimum # of samples to satisfy the theory assumptions hasn't
	 * been covered yet, this method will return <b>false</b>. Ohterwise the
	 * result will depend on wether the desired precision has been reached.
	 *
	 * @param safeguard Be extra conservative w.r.t. the min # of samples
	 *                  requested to support the theory
	 *
	 * @return Whether the desired precision has been reached for the
	 *         confidence coefficient passed on creation
	 *
	 * @see min_samples_covered()
	 */
	bool is_valid(bool safeguard = false) const noexcept;

	/// Theoretical width for creation's confidence coefficient
	/// @copydoc value_simulations_
	/// @see precision(const double&)
	double precision() const noexcept;

	/// Achieved width for requested confidence coefficient
	/// @copydoc time_simulations_
	/// @see precision()
	virtual double precision(const double& confco) const = 0;

	/// Theoretical lower limit for creation's confidence coefficient
	/// @copydoc value_simulations_
	double lower_limit() const;

	/// Achieved lower limit for requested confidence coefficient
	/// @copydoc time_simulations_
	double lower_limit(const double& confco) const;

	/// Theoretical upper limit for creation's confidence coefficient
	/// @copydoc value_simulations_
	double upper_limit() const;

	/// Achieved upper limit for requested confidence coefficient
	/// @copydoc time_simulations_
	double upper_limit(const double& confco) const;

	/// Discard all estimation info and start anew
	/// @param fullReset Erase also the \ref statOversample_
	///                  "statistical oversampling" and \ref varCorrection_
	///                  "variance correction" values currently held
	virtual void reset(bool fullReset = false) noexcept;

	/// Print current estimate, variance, and precision in a line of \p out
	/// @param out              Out-stream to use for printing
	/// @param printScientific  Use scientific notation print (ignores printPrecision)
	/// @param printPrecision   Print with specified floating point precision
	void print(std::ostream& out,
			   bool printScientific = true,
			   int printPrecision = 6u);

	/**
	 * Compute the quantile of given confidence coefficient.
	 *
	 * Given a confidence coefficient \p cc and corresponding significance level
	 * a == 1-cc, the confidence interval for an estimated sample mean x is:
	 * \code
	 * 		x ± z_a * s / sqrt(n)
	 * \endcode
	 * where 'n' is the number of observations, 's' is the sample standard
	 * deviation, and 'z_a' is the "1-a/2" quantile.<br>
	 * When the population (not the sample) variance is unknown, which is most
	 * likely the case, the quantile is computed from a student-T distribution.
	 * This functions thus returns the inverse of a student-T CDF evaluated
	 * on 1-(1-cc)/2.
	 *
	 * @param cc Confidence coefficient for the desired confidence interval
	 * @param N  Number of samples (degrees of freedom == N-1)
	 *
	 * @return Confidence coefficient quantile == inverse_CDF(1-(1-cc)/2)
	 *
	 * @warning Since this relies on the
	 *          <a href="http://goo.gl/9jtSU3">Central Limit Theorem</a>,
	 *          it is only valid for "sufficiently large" samples,
	 *          typically n > 30.
	 *
	 * @throw FigException if quantile couldn't be correctly computed
	 *
	 * @note Previous versions of this function used a unit normal variate for
	 *       "1-a/2," i.e. a random variable with standard normal distribution.
	 *       The returned quantile was thus the inverse CDF of the unit normal
	 *       variate (called <a href="http://en.wikipedia.org/wiki/Probit">
	 *       probit function</a>).<br>
	 *       If for some reason the inverse CDF of the student-T distribution
	 *       cannot be computed, this is actually the fallback behaviour.
	 */
	static double confidence_quantile(const double& cc,
									  const unsigned long& N = 0ul);

private:

	/// @note Typically used for "value simulations", viz. when estimations
	///       finish as soon as certain confidence criterion is met.
	static bool value_simulations_;  // defined only to copy the doc!

	/// @note Typically used for "time simulations", viz. when estimations
	///       run indefinitely until they're externally interrupted.
	static bool time_simulations_;   // defined only to copy the doc!
};

} // namespace fig

#endif // CONFIDENCEINTERVAL_H

