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
// FIG
#include <ConfidenceIntervalTransient.h>
#include <FigException.h>


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ConfidenceIntervalTransient::ConfidenceIntervalTransient(double confidence,
														 double precision,
														 bool dynamicPrecision,
														 bool neverStop) :
	ConfidenceInterval("transient", confidence, precision, dynamicPrecision, neverStop),
	M2(0.0)
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
	if (numNewExp > 1.1)
		throw_FigException("multiple values feeding isn't supported yet");
	/// @todo TODO Consider using incremental variance with weighted samples,
	///            which allows the use of M-sized batch runs of consecutive
	///            transient simulations before an update (https://goo.gl/EQPe46)
	update(numRE);
}


bool
ConfidenceIntervalTransient::min_samples_covered() const noexcept
{
	/// @todo TODO define!
	return numSamples_ > 30l && numSamples_*estimate_ > 2.0;
}


double
ConfidenceIntervalTransient::precision(const double& confco) const
{
	if (0.0 >= confco || 1.0 <= confco)
		throw_FigException("requires confidence coefficient ∈ (0.0, 1.0)");
	return 2.0 * confidence_quantile(confco)
			   * std::sqrt(variance_/numSamples_);
}


void
ConfidenceIntervalTransient::reset(bool fullReset) noexcept
{
	ConfidenceInterval::reset(fullReset);
	M2 = 0.0;
}

}  // namespace fig  // // // // // // // // // // // // // // // // // // // //

