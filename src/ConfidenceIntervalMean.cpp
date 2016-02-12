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


namespace fig
{

ConfidenceIntervalMean::ConfidenceIntervalMean(double confidence,
											   double precision,
											   bool dynamicPrecision) :
	ConfidenceInterval(confidence, precision, dynamicPrecision),
	M2(0.0)
{ /* Not much to do around here... */ }


void
ConfidenceIntervalMean::update(const double &newMean)
{
	// Incremental computation of mean and variance (http://goo.gl/ytk6B)
	double delta = newMean - estimate_;
	if (++numSamples_ < 0)
		throw_FigException("numSamples_ became negative, overflow?");
	estimate_ += delta/numSamples_;
	M2 += delta*(newMean-estimate_);
	variance_ = numSamples_ < 2 ? variance_ : M2/(numSamples_-1);
	// Half-width of the new confidence interval
	halfWidth_ = quantile * sqrt(variance_/numSamples_);
}


bool
ConfidenceIntervalMean::min_samples_covered() const noexcept
{
	return numSamples_ >= 30;  // easy piece thanks to CLT
}


double
ConfidenceIntervalMean::precision(const double &confco) const
{
	if (0.0 >= confco || 1.0 <= confco)
		throw_FigException("requires confidence coefficient ∈ (0.0, 1.0)");
	return 2.0 * ConfidenceInterval::confidence_quantile(confco)
			   * sqrt(variance_/numSamples_);
}


void
ConfidenceIntervalMean::reset() noexcept
{
    M2 = 0.0;
}

} // namespace fig