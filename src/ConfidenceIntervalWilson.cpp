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
	static const double log_s_quantile(log(squantile_));
	// Reinvent the wheel for the sake of efficiency...
	if (++numSamples_ <= 0l)
		throw_FigException("numSamples_ became negative, overflow?");
	numRares_ += newResult;
	prevEstimate_ = estimate_;
	estimate_ = numRares_/numSamples_;  // what about fp precision loss !!??
	variance_ = estimate_*(1.0-estimate_);
	const double
		logVarianceN(log(numSamples_-1)+log(varCorrection_)),
		z2_N = exp(log_s_quantile-logVarianceN),
		divisor = 1.0+z2_N,
		dividend = sqrt(exp(variance_-logVarianceN) + z2_N*exp(-log(4.0)-logVarianceN));
	assert(!(std::isnan(divisor)||std::isinf(divisor)));
	assert(!(std::isnan(dividend)||std::isinf(dividend)));
	halfWidth_ = quantile*dividend/divisor;
}


void
ConfidenceIntervalWilson::update(const double& newResults,
								 const double& logNumNewExperiments)
{
	static const double log_s_quantile(log(squantile_));
	if (++numSamples_ <= 0l)
		throw_FigException("numSamples_ became negative, overflow?");
	// Compute logarithm of the updated # of samples ( new + old )
	// See the wiki: https://goo.gl/qfDfKQ. Notice the use of std::log1p()
	logNumSamples_ = logNumNewExperiments + log1p(exp(logNumSamples_-logNumNewExperiments));
	if (std::isinf(logNumSamples_) || std::isnan(logNumSamples_))
		throw_FigException("failed updating logNumSamples_; overflow?");
	numRares_ += newResults;
	prevEstimate_ = estimate_;
	estimate_ = exp(log(numRares_) - logNumSamples_);
	variance_ = log(estimate_*(1.0-estimate_));
	const double
		logVarianceN(log(numSamples_-1)+log(varCorrection_)),
		z2_N = exp(log_s_quantile-logVarianceN),
		divisor = 1.0+z2_N,
		dividend = sqrt(exp(variance_-logVarianceN) + z2_N*exp(-log(4.0)-logVarianceN));
	assert(!(std::isnan(divisor)||std::isinf(divisor)));
	assert(!(std::isnan(dividend)||std::isinf(dividend)));
	halfWidth_ = quantile*dividend/divisor;
}


bool
ConfidenceIntervalWilson::min_samples_covered(bool) const noexcept
{
	// Even though the Wilson score interval has lax bounds (http://goo.gl/B86Dc),
	// they've been tailored to meet experimental quality standards
	return numSamples_ > 5l && numSamples_*estimate_ > 1.0;
//	static constexpr long LBOUND(1l<<7l), LBOUNDR(1l<<9l);
//	static const double LOG_LBOUNDR(log(LBOUNDR));
//	const bool theoreticallySound =
//			LBOUND < numSamples_ && LBOUNDR < numRares_ && (
//			  log(30l*statOversample_) < logNumSamples_ || (
//				LOG_LBOUNDR < logNumSamples_+log(estimate_) &&  // n*p
//				LOG_LBOUNDR < logNumSamples_+log1p(-estimate_)  // n*(1-p)
//			  )
//			);
//	// Ask also for little change w.r.t. the last outcome
//	const bool practicallySound = abs(prevEstimate_-estimate_) < 0.02*estimate_;
//	return theoreticallySound && practicallySound;
}


double
ConfidenceIntervalWilson::precision(const double &confco) const
{
	if (0.0 >= confco || 1.0 <= confco)
		throw_FigException("requires confidence coefficient ∈ (0.0, 1.0)");
	const double
		quantile = confidence_quantile(confco),
		logVarianceN(log(numSamples_-1)+log(varCorrection_)),
		z2_N = exp(2.0*log(quantile)-logVarianceN),
		divisor = 1.0+z2_N,
		dividend = sqrt(exp(variance_-logVarianceN) + z2_N*exp(-log(4.0)-logVarianceN));
	assert(!(std::isnan(divisor)||std::isinf(divisor)));
	assert(!(std::isnan(dividend)||std::isinf(dividend)));
	return 2.0 * quantile * dividend/divisor;
}


void
ConfidenceIntervalWilson::reset(bool fullReset) noexcept
{
	ConfidenceInterval::reset(fullReset);
    numRares_ = 0.0;
    logNumSamples_ = 0.0;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
