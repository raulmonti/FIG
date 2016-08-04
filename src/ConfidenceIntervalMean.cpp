//==============================================================================
//
//  ConfidenceIntervalMean.cpp
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
#include <cmath>  // sqrt()
// FIG
#include <ConfidenceIntervalMean.h>
#include <FigException.h>

using std::abs;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ConfidenceIntervalMean::ConfidenceIntervalMean(double confidence,
											   double precision,
											   bool dynamicPrecision,
											   bool neverStop) :
	ConfidenceInterval("mean_std", confidence, precision, dynamicPrecision, neverStop),
	M2(0.0)
{ /* Not much to do around here... */ }


void
ConfidenceIntervalMean::update(const double& newMean)
{
	// Incremental computation of mean and variance (http://goo.gl/ytk6B)
	double delta = newMean - estimate_;
	if (++numSamples_ < 0l)
		throw_FigException("numSamples_ became negative, overflow?");
	prevEstimate_ = estimate_;
	estimate_ += delta/numSamples_;
	M2 += delta*(newMean-estimate_);
	variance_ = numSamples_ < 2l ? variance_ : M2/(numSamples_-1l);
	// Half-width of the new confidence interval
	halfWidth_ = quantile * std::sqrt(variance_/numSamples_);
}


bool
ConfidenceIntervalMean::min_samples_covered() const noexcept
{
	// Easy-peasy thanks to CLT:
	const bool theoreticallySound = numSamples_ >= 30l;
	// Ask also for little change w.r.t. the last outcome
	const bool practicallySound = abs(prevEstimate_-estimate_) < 0.15*estimate_;
	// So, did we make it already?
	return theoreticallySound && practicallySound;
}


double
ConfidenceIntervalMean::precision(const double& confco) const
{
	if (0.0 >= confco || 1.0 <= confco)
		throw_FigException("requires confidence coefficient ∈ (0.0, 1.0)");
	return 2.0 * confidence_quantile(confco)
			   * std::sqrt(variance_/numSamples_);
}


void
ConfidenceIntervalMean::reset(bool fullReset) noexcept
{
	ConfidenceInterval::reset(fullReset);
    M2 = 0.0;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
