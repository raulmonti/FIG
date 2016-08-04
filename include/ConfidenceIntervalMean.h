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
 * the confidence interval for a sample mean 'x' is given by
 * \code
 *                   x ± z_a * s / sqrt(n)
 * \endcode
 * where 'z_a' is the "1-a/2" quantile of a unit normal variate, 's^2' is the
 * sample variance and 'n' is the sample size. For the z_a quantile, 'a' is
 * the significance level defined as a=1-cc for the confidence coefficient
 * 'cc' inherent to the confidence interval.
 */
class ConfidenceIntervalMean : public ConfidenceInterval
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

	/**
	 * Stub to update(), since multiple value feeding is impossible
	 * for this kind of CI due to the variance measurement.
	 * @param newMeans Mean value of the sampled distribution
	 * @warning Second parameter is <b>ignored</b>
	 * @throw FigException if detected possible overflow
	 * @see update(const double&)
	 */
	inline void update(const double& newMeans, const double&) override { update(newMeans); }

public:  // Utils

	bool min_samples_covered() const noexcept override;

	double precision(const double& confco) const override;

	void reset(bool fullReset = false) noexcept override;
};

} // namespace fig

#endif // CONFIDENCEINTERVALMEAN_H
