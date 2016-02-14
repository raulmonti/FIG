//==============================================================================
//
//  ConfidenceIntervalProportion.cpp
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


// C
#include <cmath>  // log(), exp(), sqrt()
// FIG
#include <ConfidenceIntervalProportion.h>
#include <FigException.h>


namespace fig
{

ConfidenceIntervalProportion::ConfidenceIntervalProportion(
	double confidence,
	double precision,
	bool dynamicPrecision) :
        ConfidenceInterval("proportion_std", confidence, precision, dynamicPrecision),
		numRares_(0.0),
		logNumSamples_(0.0)
{ /* Not much to do around here */ }


void
ConfidenceIntervalProportion::update(const double& newResult)
{
	update(newResult, log(1));  // don't reinvent the wheel
}


void
ConfidenceIntervalProportion::update(const double& newResults,
									 const double& logNumNewExperiments)
{
	// Check for possible overflows
	if (0.0 < newResults && numRares_ + newResults == numRares_)
		throw_FigException("can't increase numRares_ count, overflow?");
	if (std::isinf(logNumNewExperiments) || std::isnan(logNumNewExperiments))
		throw_FigException("invalid logNumNewExperiments, overflow?");

	numRares_ += newResults;

	// Compute logarithm of the updated # of samples ( old + new )
	logNumSamples_ += log(1.0 + exp(logNumNewExperiments - logNumSamples_));
	if (std::isinf(logNumSamples_) || std::isnan(logNumSamples_))
		throw_FigException("failed updating logNumSamples_, overflow?");

	// Compute the updated estimate, variance and interval half width
	estimate_ = exp(log(numRares_) - logNumSamples_);
	variance_ = estimate_ * (1.0 - estimate_);
	halfWidth_ = quantile * sqrt( exp(
					 log(variance_) - (logNumSamples_ + log(varCorrection_))));
}


bool
ConfidenceIntervalProportion::min_samples_covered() const noexcept
{
    return numRares_ > 30 * statOversample_;
}


double
ConfidenceIntervalProportion::precision(const double &confco) const
{
	if (0.0 >= confco || 1.0 <= confco)
		throw_FigException("requires confidence coefficient ∈ (0.0, 1.0)");
	return 2.0 * ConfidenceInterval::confidence_quantile(confco)
			   * sqrt( exp( log(variance_) - (logNumSamples_ + log(varCorrection_))));
}


void
ConfidenceIntervalProportion::reset() noexcept
{
    numRares_ = 0.0;
    logNumSamples_ = 0.0;
}

} // namespace fig
