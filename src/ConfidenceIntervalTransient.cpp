//==============================================================================
//
//  ConfidenceIntervalTransient.cpp
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
#include <cmath>
#include <cassert>
// C++
#include <limits>
// FIG
#include <ConfidenceIntervalTransient.h>
#include <FigException.h>


using std::abs;
using std::exp;
using std::log;
using std::log1p;
using std::sqrt;

namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ConfidenceIntervalTransient::ConfidenceIntervalTransient(double confidence,
														 double precision,
														 bool dynamicPrecision,
														 bool neverStop) :
	ConfidenceInterval("transient", confidence, precision, dynamicPrecision, neverStop),
	M2(0.0),
	logNumSamples_(0.0),
    logVariance_(std::numeric_limits<decltype(logVariance_)>::infinity())
{ /* Not much to do around here... */ }


void
ConfidenceIntervalTransient::update(const double& weighedNRE)
{
	assert(false);  // NOTE deprecated
	prevEstimate_ = estimate_;
	// Incremental (stable) computation of mean and variance (http://goo.gl/ytk6B)
	const double delta = abs(weighedNRE - estimate_),
				 sgn   = weighedNRE < estimate_ ? -1.0 : 1.0;
	assert(!(std::isnan(delta)||std::isinf(delta)));
	if (++numSamples_ <= 0l)
		throw_FigException("numSamples_ became negative, overflow?");
	logNumSamples_ = log(numSamples_);
	estimate_ += sgn * exp(log(delta)-logNumSamples_);
	M2 += sgn * delta * (weighedNRE-estimate_);
	logVariance_ = log(M2)-logNumSamples_;  // should use "numSamples_-1" for unbiasedness...
	assert(!std::isnan(logVariance_));
	if (0.0 < M2 && std::isinf(logVariance_))
		throw_FigException("invalid internal value, overflow?");
	else
		variance_ = exp(logVariance_);
	// Half-width of the new confidence interval
	halfWidth_ = quantile * sqrt(exp(logVariance_-logNumSamples_));
}


void
ConfidenceIntervalTransient::update(const std::vector<double>& weighedNREs)
{
	prevEstimate_ = estimate_;
	// Incremental (stable) computation of mean and variance (http://goo.gl/ytk6B)
	for (const double& weighedNRE: weighedNREs) {
		const double delta = abs(weighedNRE - estimate_),
					 sgn   = weighedNRE < estimate_ ? -1.0 : 1.0;
		assert(!(std::isnan(delta)||std::isinf(delta)));
		if (++numSamples_ <= 0l)
			throw_FigException("numSamples_ became negative, overflow?");
		estimate_ += sgn * exp(log(delta)-log(numSamples_));
		M2 += sgn * delta * (weighedNRE-estimate_);
		assert(!(std::isnan(M2)||std::isinf(M2)));
	}
	logNumSamples_ = log(numSamples_);
	logVariance_ = log(M2)-logNumSamples_;  // should use "numSamples_-1" for unbiasedness...
	assert(!std::isnan(logVariance_));
	if (0.0 < M2 && std::isinf(logVariance_))
		throw_FigException("invalid internal value, overflow?");
	else
		variance_ = exp(logVariance_);
	// Half-width of the new confidence interval
	halfWidth_ = quantile * sqrt(exp(logVariance_-logNumSamples_));
}


bool
ConfidenceIntervalTransient::min_samples_covered(bool considerEpsilon) const noexcept
{
	// Rule of thumb for the lower bound of the magnitude "n*p":
	static const double THEORETICAL_LBOUND(log(0.25));
	// - THEORETICAL_LBOUND is 5% of the usual "n*p > 5" rule for the CI of
	//   binomial proportions using normal approximation;
	// - for queueing and high reliability systems, it is the variance
	//   rather than the num_samples what prolongues estimations.
	const bool
	    // 4K-CLT or considerations for binomial proportions
	    theoreticallySound = (1l<<12l)*30l <= numSamples_ ||
	                         THEORETICAL_LBOUND < logNumSamples_+log(estimate_),
		// If requested, ask also for little change w.r.t. the last estimate
	    practicallySound = considerEpsilon ? std::abs(prevEstimate_-estimate_) < 0.05*estimate_
	                                       : true;
	return theoreticallySound && practicallySound;
}


double
ConfidenceIntervalTransient::precision(const double& confco) const
{
	if (0.0 >= confco || 1.0 <= confco)
		throw_FigException("requires confidence coefficient ∈ (0.0, 1.0)");
	return 2.0 * halfWidth_ * (confidence_quantile(confco)/quantile);
//	return 2.0 * confidence_quantile(confco)
//			   * sqrt(exp(logVariance_-logNumSamples_));
}


void
ConfidenceIntervalTransient::reset(bool fullReset) noexcept
{
	ConfidenceInterval::reset(fullReset);
	M2 = 0.0;
	logNumSamples_ = 0.0;
	logVariance_ = std::numeric_limits<decltype(logVariance_)>::infinity();
}

}  // namespace fig  // // // // // // // // // // // // // // // // // // // //

