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

/// The standard CI suffices for rate-like simulations
/// @todo TODO improve this lame docstring
class ConfidenceIntervalRate : public virtual ConfidenceIntervalMean
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
