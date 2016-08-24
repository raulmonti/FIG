//==============================================================================
//
//  ConfidenceIntervalRate.h
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

#ifndef CONFIDENCEINTERVALRATE_H
#define CONFIDENCEINTERVALRATE_H

#include <ConfidenceIntervalMean.h>

namespace fig
{

/**
 * @brief Confidence interval for estimates of long-run simulations
 *
 *        The estimates this CI expects are time averages or proportions.
 *        It is irrelevant how these were measured, since no assumptions
 *        are made regarding the distribution of the samples.
 *        The <a href="https://goo.gl/wxYuzG">standard CLT theory for
 *        confidence intervals</a> with unknown standard deviation is used.
 *
 * @warning Useful for "few" updates, ideally < 2^11 samples.
 *          Feeding too many samples may incurr in floating-point precision
 *          loss, see <a href="https://goo.gl/wxYuzG">the wiki</a> on this.
 *
 * @see ConfidenceIntervalMean
 */
class ConfidenceIntervalRate : public ConfidenceIntervalMean
{
public:
	/// @copydoc ConfidenceInterval::ConfidenceInterval()
	ConfidenceIntervalRate(double confidence,
						   double precision,
						   bool dynamicPrecision = false,
						   bool neverStop = false) :
		ConfidenceInterval("rate", confidence, precision, dynamicPrecision, neverStop),
		ConfidenceIntervalMean(confidence, precision, dynamicPrecision, neverStop)
		{ /* Not much to do around here... */ }
};

}

#endif // CONFIDENCEINTERVALRATE_H
