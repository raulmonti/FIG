//==============================================================================
//
//  ConfidenceIntervalProportion.h
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

#ifndef CONFIDENCEINTERVALPROPORTION_H
#define CONFIDENCEINTERVALPROPORTION_H

#include <ConfidenceInterval.h>


namespace fig
{

/**
 * @brief Usual ConfidenceInterval for estimations of binomial proportions.
 *
 * Using the <a href="http://goo.gl/1hkixG">CLT for binomially distributed
 * samples</a>, the confidence interval for a proportion estimate 'p' is given
 * by the mathematical expression
 * \code
 *                   p ± z * sqrt(1/n * p * (1-p))
 * \endcode
 * where 'z' is the "1-a/2" quantile of a unit normal variate and 'n' is the
 * sample size. For the z quantile, 'a' is the significance level defined as
 * a=1-cc for the confidence coefficient 'cc' inherent to the confidence
 * interval.
 *
 * @see ConfidenceIntervalWilson
 */
class ConfidenceIntervalProportion : public ConfidenceInterval
{
	/// Count of the successes registered so far
	double numRares_;

	/// Logarithmic value of the sample size
	double logNumSamples_;

public:  // Ctor

	/// @copydoc ConfidenceInterval::ConfidenceInterval()
	ConfidenceIntervalProportion(double confidence,
								 double precision,
								 bool dynamicPrecision = false,
								 bool neverStop = false);

public:  // Modifyers

	/**
	 * @brief Update current estimation with a new experiment result
	 * @param newResult Whether last experiment (singular) was "successfull"
	 * @note Each fed value should be '1' if the experiment succeeded and '0'
	 *       if it failed, like in a Bernoulli trial.
	 * @see update(const double&, const double&)
	 * @throw FigException if detected possible overflow
	 */
	void update(const double& newResult) override;

	/**
	 * @brief Update current estimation with several new experiment results
	 * @param newResults Amount of successes observed in the new experiments ran
	 * @param logNumNewExperiments Natural logarithm of the # of expermients ran
	 * @note This updates the estimation by counting the number of successes
	 *       observed in several experiments ran, like in a Binomial trial.
	 * @note The logarithm is used to avoid commonplace overflows
	 * @throw FigException if detected possible overflow
	 */
	void update(const double& newResults,
				const double& logNumNewExperiments) override;

public:  // Utils

	bool min_samples_covered() const noexcept override;

	double precision(const double& confco) const override;

	void reset(bool fullReset = false) noexcept override;
};

} // namespace fig

#endif // CONFIDENCEINTERVALPROPORTION_H
