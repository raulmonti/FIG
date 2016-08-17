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
// FIG
#include <ConfidenceIntervalTransient.h>
#include <FigException.h>


using std::abs;
using std::exp;
using std::log;
using std::log1p;

namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ConfidenceIntervalTransient::ConfidenceIntervalTransient(double confidence,
														 double precision,
														 bool dynamicPrecision,
														 bool neverStop) :
	ConfidenceInterval("transient", confidence, precision, dynamicPrecision, neverStop),
	M2(0.0),
	numRares_(0.0),
	logNumSamples_(0.0)
{ /* Not much to do around here... */ }


void
ConfidenceIntervalTransient::update(const double& weighedNRE)
{
	// Incremental computation of mean and variance (http://goo.gl/ytk6B)
	double delta = weighedNRE - estimate_;
	if (++numSamples_ <= 0l)
		throw_FigException("numSamples_ became negative, overflow?");
	prevEstimate_ = estimate_;
	estimate_ += delta/numSamples_;
	M2 += delta*(weighedNRE-estimate_);
	variance_ = numSamples_ < 2l ? variance_ : M2/(numSamples_-1l);
	// Half-width of the new confidence interval
	halfWidth_ = quantile * std::sqrt(variance_/numSamples_);
}


void
ConfidenceIntervalTransient::update(const double& numRE, const double& numNewExp)
{
	/// @todo TODO inline in header in class description
	if (numNewExp > 1.1)
		throw_FigException("multiple values feeding isn't supported");
	update(numRE);
}


void
ConfidenceIntervalTransient::update(const std::vector<double> &numREs)
{

	// Compute logarithm of the updated # of samples ( new + old )
	// See the wiki: https://goo.gl/qfDfKQ. Notice the use of std::log1p()
	const double logNumNewExperiments(log(numREs.size()));
	logNumSamples_ = logNumNewExperiments + log1p(exp(logNumSamples_-logNumNewExperiments));
	prevEstimate_ = estimate_;
	double numNewRares(0.0);
	// Incremental (stable) computation of mean and variance (http://goo.gl/ytk6B)
	for (auto numRE: numREs) {
		numNewRares += numRE;
		double delta = abs(numRE - estimate_), sgn(numRE > estimate_ ? 1.0 : -1.0);
		assert(!std::isnan(delta));
		assert(!std::isinf(delta));
		if (++numSamples_ <= 0l)
			throw_FigException("numSamples_ became negative, overflow?");
		estimate_ += exp(log(delta)-log(numSamples_)) * sgn;
		M2 += delta*sgn*(numRE-estimate_);
		assert(!std::isnan(M2));
		assert(!std::isinf(M2));
	}
	numRares_ += numNewRares;
//	estimate_ = exp(log(numRares_)-logNumSamples_);  // more precise (?)
	variance_ = exp(log(M2)-logNumSamples_);
	assert(!std::isnan(variance_));
	assert(!std::isinf(variance_));

//	const double logNumNewExperiments(log(numREs.size()));
//	prevEstimate_ = estimate_;
//	numSamples_ += numREs.size();
//	// Compute logarithm of the updated # of samples ( new + old )
//	// See the wiki: https://goo.gl/qfDfKQ. Notice the use of std::log1p()
//	logNumSamples_ = logNumNewExperiments + log1p(exp(logNumSamples_-logNumNewExperiments));
//	double numNewRares(0.0);
//	for (const auto& numRE: numREs)
//		numNewRares += numRE;
//	numRares_ += numNewRares;
//	estimate_ = exp(log(numRares_)-logNumSamples_);
//	for (const auto& numRE: numREs)
//		M2 += (numRE-prevEstimate_)*(numRE-prevEstimate_);  // not quite correct: prevEstimate should be updated on every step...
//	variance_ = exp(log(M2)-logNumSamples_);

	// Half-width of the new confidence interval
	halfWidth_ = quantile * std::sqrt(exp(log(variance_)-logNumSamples_));
}


bool
ConfidenceIntervalTransient::min_samples_covered() const noexcept
{
	/// @todo TODO define!
	static const double TH_LBOUND(log(1.0));
	return 30l < numSamples_ &&
			TH_LBOUND < log(numSamples_)+log(estimate_);
}


double
ConfidenceIntervalTransient::precision(const double& confco) const
{
	if (0.0 >= confco || 1.0 <= confco)
		throw_FigException("requires confidence coefficient ∈ (0.0, 1.0)");
	// Array update method:
	return 2.0 * confidence_quantile(confco)
			   * std::sqrt(exp(log(variance_)-logNumSamples_));
//  // Single value update method:
//	return 2.0 * confidence_quantile(confco)
//			   * std::sqrt(variance_/numSamples_);
}


void
ConfidenceIntervalTransient::reset(bool fullReset) noexcept
{
	ConfidenceInterval::reset(fullReset);
	M2 = 0.0;
	numRares_ = 0.0;
	logNumSamples_ = 0.0;
}

}  // namespace fig  // // // // // // // // // // // // // // // // // // // //

