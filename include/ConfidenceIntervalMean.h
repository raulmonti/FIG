//==============================================================================
//
//  ConfidenceIntervalMean.h
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

#ifndef CONFIDENCEINTERVALMEAN_H
#define CONFIDENCEINTERVALMEAN_H

#include <ConfidenceInterval.h>


namespace fig
{

/**
 * @brief ConfidenceInterval for unknown magnitudes or proportions (default)
 *
 * Using the <a href="http://goo.gl/9jtSU3">Central Limit Theorem</a>,
 * the confidence interval for a sample mean \f$x\f$ is given by
 * $$ x \pm z_a \frac{s}{\sqrt{n}} $$
 * where \f$z_a\f$ is the \f$1-\frac{a}{2}\f$ quantile of a student-T or
 * unit normal variate, \f$s^2\f$ is the sample variance, and \f$n\f$ is the
 * sample size.<br>
 * For the \f$z_a\f$ quantile, \f$a=1-cc\f$ is the <em>significance level</em>
 * for the confidence coefficient \f$cc\f$ inherent to the confidence interval.
 */
class ConfidenceIntervalMean : public virtual ConfidenceInterval
{
	double M2;

public:  // Ctor

	/// @copydoc ConfidenceInterval::ConfidenceInterval()
	ConfidenceIntervalMean(double confidence,
						   double precision,
						   bool dynamicPrecision = false,
						   bool neverStop = false);

public:  // Modifyers

	/**
	 * @brief Update current estimation with a new sampled mean
	 * @param newMean Fresh mean value estimated for the property under study
	 * @note Each measurement, i.e. each value fed, represents the mean value
	 *       of some (any) sampled distribution.
	 * @throw FigException if detected possible overflow
	 */
	void update(const double& newMean) override;

public:  // Utils

	bool min_samples_covered(bool considerEpsilon = false) const noexcept override;

	double precision(const double& confco) const override;

	void reset(bool fullReset = false) noexcept override;
};

} // namespace fig

#endif // CONFIDENCEINTERVALMEAN_H
