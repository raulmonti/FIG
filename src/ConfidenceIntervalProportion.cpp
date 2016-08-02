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

using std::sqrt;
using std::exp;
using std::log;
using std::log1p;


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
	update(newResult, log1p(0.0));  // don't reinvent the wheel
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

	// Compute logarithm of the updated # of samples ( old + new )
	// See the wiki: https://goo.gl/qfDfKQ. Notice the use of std::log1p()
	logNumSamples_ += log1p(exp(logNumNewExperiments - logNumSamples_));
	if (std::isinf(logNumSamples_) || std::isnan(logNumSamples_))
		throw_FigException("failed updating logNumSamples_, overflow?");

	numRares_ += newResults;
	if (0.0 >= numRares_)
		return;  // nothing to work with yet
	else if (0.0 < newResults)
		numSamples_++;

	// Compute the updated estimate, variance, and interval half width
	estimate_ = exp(log(numRares_) - logNumSamples_);
	variance_ = estimate_ * (1.0 - estimate_);
	halfWidth_ = quantile * sqrt( exp(
					 log(variance_) - (logNumSamples_ + log(varCorrection_))));
}


bool
ConfidenceIntervalProportion::min_samples_covered() const noexcept
{
	static const long MIN_NUM_HITS = 18l;
	// Even though the interval's lower bounds are based on the CLT,
	// they've been increased to meet experimental quality standards
	return numSamples_ > MIN_NUM_HITS &&
			(numRares_ > std::min(20.0*MIN_NUM_HITS, 30.0*statOversample_) ||
			 (logNumSamples_+log(estimate_)    > log(10.0*MIN_NUM_HITS) &&  // n*p > 180
			  logNumSamples_+log1p(-estimate_) > log(10.0*MIN_NUM_HITS)));  // n*(1-p) > 180
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
	ConfidenceInterval::reset();
    numRares_ = 0.0;
    logNumSamples_ = 0.0;
}

} // namespace fig
