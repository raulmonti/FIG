//==============================================================================
//
//  ConfidenceIntervalWilson.h
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

#ifndef CONFIDENCEINTERVALWILSON_H
#define CONFIDENCEINTERVALWILSON_H

#include <ConfidenceInterval.h>


namespace fig
{

/**
 * @brief Improved ConfidenceInterval for estimations of binomial proportions.
 *
 * The <a href="http://goo.gl/B86Dc">Wilson score interval</a>
 * is an improvement over the \ref ConfidenceIntervalProportion "usual
 * approximation interval for binomial proportions".
 * In particular, this kind of interval has good properties when dealing with
 * extreme probability values, e.g. for rare events whose probability is ~0.
 */
class
#if __cplusplus >= 201402L
[[
    deprecated(
    "Real value coverage and performance issues rendered this class useless. "
    "ConfidenceIntervalTransient is now used to build the interval "
    "around the estimates of transient-like properties.")
]]
#endif
ConfidenceIntervalWilson: public ConfidenceInterval
{
	/// Squared quantile of the confidence coefficient
	const double squantile_;

	/// Count of the successes registered so far
	double numRares_;

	/// Logarithmic value of the sample size
	double logNumSamples_;

public:  // Ctor

	/// @copydoc ConfidenceInterval::ConfidenceInterval()
	ConfidenceIntervalWilson(double confidence,
							 double precision,
							 bool dynamicPrecision = false,
							 bool neverStop = false);

public:  // Modifyers

	/// @copydoc ConfidenceIntervalProportion::update(const double&)
	void update(const double& newResult) override;

	/// @copydoc ConfidenceIntervalProportion::update(const double&, const double&)
	void update(const double& newResults,
				const double& logNumNewExperiments);

public:  // Utils

	bool min_samples_covered(bool) const noexcept override;

	double precision(const double& confco) const override;

	void reset(bool fullReset = false) noexcept override;
};

} // namespace fig

#endif // CONFIDENCEINTERVALWILSON_H
