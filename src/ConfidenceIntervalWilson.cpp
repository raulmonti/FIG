//==============================================================================
//
//  ConfidenceIntervalWilson.cpp
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
#include <cassert>
// FIG
#include <ConfidenceIntervalWilson.h>
#include <FigException.h>

using std::sqrt;
using std::exp;
using std::log;
using std::log1p;
using std::abs;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ConfidenceIntervalWilson::ConfidenceIntervalWilson(
	double confidence,
	double precision,
	bool dynamicPrecision,
	bool neverStop) :
		ConfidenceInterval("proportion_wilson",
						   confidence,
						   precision,
						   dynamicPrecision,
						   neverStop),
		squantile_(quantile*quantile),
		numRares_(0.0),
		logNumSamples_(0.0)
{ /* Not much to do around here */ }


void
ConfidenceIntervalWilson::update(const double& newResult)
{
	update(newResult, log1p(0.0));  // don't reinvent the wheel
}


void
ConfidenceIntervalWilson::update(const double& newResults,
								 const double& logNumNewExperiments)
{
	// Check for possible overflows
	if (0.0 < newResults && numRares_ + newResults == numRares_)
		throw_FigException("can't increase numRares_ count; overflow?");
	if (std::isinf(logNumNewExperiments) || std::isnan(logNumNewExperiments))
		throw_FigException("invalid logNumNewExperiments; overflow?");

	// Compute logarithm of the updated # of samples ( old + new )
	// See the wiki: https://goo.gl/qfDfKQ. Notice the use of std::log1p()
	logNumSamples_ += log1p(exp(logNumNewExperiments - logNumSamples_));
	if (std::isinf(logNumSamples_) || std::isnan(logNumSamples_))
		throw_FigException("failed updating logNumSamples_; overflow?");

	numRares_ += newResults;
	if (0.0 >= numRares_)
		return;  // nothing to work with yet
	else if (0.0 < newResults)
		numSamples_++;

	// Compute the updated estimate
	double logEstimate = log(numRares_ + squantile_/2.0) - logNumSamples_
						 - log1p(exp(log(squantile_)-logNumSamples_));
	if (std::isinf(logEstimate) || std::isnan(logEstimate))
		throw_FigException("failed computing logEstimate; overflow?");
	prevEstimate_ = estimate_;
	estimate_ = exp(logEstimate);
	assert(!std::isinf(estimate_) && !std::isnan(estimate_));

	// Compute the updated variance and interval half width
	const double phat = exp(log(numRares_) - logNumSamples_);
	variance_ = phat * (1.0-phat);
	const double numSamplesTimesVarCorrection = exp(logNumSamples_ + log(varCorrection_));
	if (std::isinf(numSamplesTimesVarCorrection) ||
		std::isnan(numSamplesTimesVarCorrection) ||
			0.0 > numSamplesTimesVarCorrection)
		throw_FigException("invalid internal variable value; overflow?");
	halfWidth_ = quantile * sqrt(numSamplesTimesVarCorrection)
			* sqrt(variance_ + squantile_/(4.0*numSamplesTimesVarCorrection))
			/ (numSamplesTimesVarCorrection + squantile_);
}


bool
ConfidenceIntervalWilson::min_samples_covered() const noexcept
{
	// Even though the Wilson score interval has lax lower bounds (http://goo.gl/B86Dc),
	// they've been increased to meet experimental quality standards
	static constexpr long LBOUND(1l<<7l), LBOUNDR(800l);
	static constexpr double LOG_LBOUNDR(log(LBOUNDR));
	const bool theoreticallySound =
			LBOUND < numSamples_ && LBOUNDR < numRares_ && (
			  log(30l*statOversample_) < logNumSamples_ || (
				LOG_LBOUNDR < logNumSamples_+log(estimate_) &&  // n*p
				LOG_LBOUNDR < logNumSamples_+log1p(-estimate_)  // n*(1-p)
			  )
			);
	/// @todo TODO consider removing "practical" stopping criterion
//	// Ask also for little change w.r.t. the last outcome
//	const bool practicallySound = abs(prevEstimate_-estimate_) < 0.02*estimate_;
//	// So, did we make it already?
//	return theoreticallySound && practicallySound;

	return theoreticallySound;
}


double
ConfidenceIntervalWilson::precision(const double &confco) const
{
	if (0.0 >= confco || 1.0 <= confco)
		throw_FigException("requires confidence coefficient ∈ (0.0, 1.0)");
	const double quantile = confidence_quantile(confco);
	const double numSamplesTimesVarCorrection = exp(logNumSamples_ + log(varCorrection_));
	return 2.0 * quantile * sqrt(numSamplesTimesVarCorrection)
			   * sqrt(variance_ + quantile*quantile/(4.0*numSamplesTimesVarCorrection))
			   / (numSamplesTimesVarCorrection + quantile*quantile);
}


void
ConfidenceIntervalWilson::reset(bool fullReset) noexcept
{
	ConfidenceInterval::reset(fullReset);
    numRares_ = 0.0;
    logNumSamples_ = 0.0;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
