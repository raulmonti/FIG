//==============================================================================
//
//  ConfidenceIntervalResult.h
//
//  Copyleft 2017-
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


#ifndef CONFIDENCEINTERVALRESULT_H
#define CONFIDENCEINTERVALRESULT_H

#include <ConfidenceIntervalTransient.h>
#include <ConfidenceIntervalRate.h>


namespace fig
{

/// Observer class for ConfidenceInterval,
/// used e.g. to show the results to the user.
/// @warning Immutable: instances of this class are unfit for estimations
class ConfidenceIntervalResult// : public ConfidenceInterval
        /// @todo TODO this must be an observer class for ConfidenceInterval
        ///       that has the "double precision(double)" member function
        ///       from either ConfidenceIntervalRate or ConfidenceIntervalTransient,
        ///       depending on which does it derive from.
        ///       Multiple inheritance doesn't help because both classes define
        ///       the same methods, and we get "more than one final overrider"
        ///       The union solution used in the Transition class is hideous.
        ///       HELP!
{
	union {
		ConfidenceIntervalTransient tr;
		ConfidenceIntervalRate ss;
	} ci_instance_;
	enum { TRANSIENT, RATE } ci_type_;

public:

	ConfidenceIntervalResult() :
	    ConfidenceInterval("immutable", .9, 1.0) {}

	ConfidenceIntervalResult(const ConfidenceIntervalTransient& that) :
	ConfidenceIntervalResult(const ConfidenceIntervalRate& that) :
//	    ConfidenceInterval("immutable",
//	                       that.confidence,
//	                       that.errorMargin*2.0,
//	                       that.percent,
//	                       that.alwaysInvalid)
//	{ /* Not much to do around here... */ }

private:

	void update(const double&) override {}
//	bool min_samples_covered(bool) const noexcept override { return false; }
//	double precision(const double&) const override { return 0.0; }
	void reset(bool) noexcept override {}
};

} // namespace fig

#endif // CONFIDENCEINTERVALRESULT_H
