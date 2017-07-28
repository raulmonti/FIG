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

// C++
#include <memory>
// FIG
#include <ConfidenceInterval.h>
#include <ConfidenceIntervalRate.h>
#include <ConfidenceIntervalTransient.h>


namespace fig
{

/// Observer class for ConfidenceInterval,
/// used e.g. to show resulting estimates to the user.
/// @warning Immutable: instances of this class are unfit for estimations
class ConfidenceIntervalResult : public ConfidenceInterval
{

//	/// Type of the instance (of the derived class of ConfidenceInterval)
//	/// from which we were created
//	enum {
//		TRANSIENT = 0,
//		RATE,
//		NUM_CI_TYPES
//	} ci_type_;

	/// (Derived) Instance of ConfidenceInterval we're wrapping
	std::shared_ptr< ConfidenceInterval > instance_;

//	//	/// Instances of ConfidenceIntervalResult can only be constructed
//	//	/// from an instance of one of the following classes:
//	//	union {
//	//		ConfidenceIntervalTransient trCI;
//	//		ConfidenceIntervalRate ssCI;
//	//	};
//
//	//	/// Fun. ptr. to overrider of ConfidenceInterval::update()
//	//	void (ConfidenceInterval::*fptr_update_) (const double&);
//
//		/// Fun. ptr. to overrider of ConfidenceInterval::min_samples_covered()
//	//	bool (*fptr_min_samples_covered_) (bool);
//		bool (ConfidenceInterval::*fptr_min_samples_covered_) (bool) const noexcept;
//
//		/// Fun. ptr. to overrider of ConfidenceInterval::precision(const double&)
//	//	double (*fptr_precision_) (const double&);
//		double (ConfidenceInterval::*fptr_precision_) (const double&) const;
//
//	//	/// Fun. ptr. to overrider of ConfidenceInterval::reset()
//	//	void (ConfidenceInterval::*fptr_reset_) (bool fullReset) noexcept;

private:

	/// Empty ctor to allow a vector of ConfidenceIntervalResult
	ConfidenceIntervalResult() :
	    ConfidenceInterval("immutable", .9, 1.0, false, true),
	    instance_(nullptr)
//	    fptr_update_(nullptr),
//	    fptr_reset_(nullptr),
//	    fptr_min_samples_covered_(nullptr),
//	    fptr_precision_(nullptr)
	{}

public:  // Ctors from the other derived classes of ConfidenceInterval

	/// Ctor for \ref ConfidenceIntervalTransient "transient confidence intervals"
	ConfidenceIntervalResult(const ConfidenceIntervalTransient& that) :
	    ConfidenceInterval(that),
	    instance_(std::make_shared<ConfidenceInterval>(that))
//	    trCI(that),
//	    fptr_min_samples_covered_(&trCI.min_samples_covered),
//	    fptr_precision_(&trCI.precision)
	{ /* Not much to do around here... */ }

	/// Ctor for \ref ConfidenceIntervalRate "rate confidence intervals"
	ConfidenceIntervalResult(const ConfidenceIntervalRate& that) :
	    ConfidenceInterval(that),
	    instance_(std::make_shared<ConfidenceInterval>(that))
//	    ssCI(that),
//	    fptr_min_samples_covered_(&ssCI.min_samples_covered),
//	    fptr_precision_(&ssCI.precision)
	{ /* Not much to do around here... */ }

public:  // Methods linked to the real functions of our creation class

	/// Stub to method of creation class
	/// @copydoc ConfidenceInterval::min_samples_covered()
	bool min_samples_covered(bool considerEpsilon = false) const noexcept override
	    {
		    return (nullptr == instance_)
			            ? false
			            : instance_->min_samples_covered(considerEpsilon);
	    }

	/// Stub to method of creation class
	/// @copydoc ConfidenceInterval::precision(const double&)
	double precision(const double& confco) const override
	    {
		    return (nullptr == instance_)
			            ? 0.0
			            : instance_->precision(confco);
	    }

//	ConfidenceIntervalResult(const ConfidenceIntervalRate& that) :
//	    ConfidenceInterval("immutable",
//	                       that.confidence,
//	                       that.errorMargin*2.0,
//	                       that.percent,
//	                       that.alwaysInvalid)
//	{ /* Not much to do around here... */ }

private:  // This is an observer class: the following methods are banned

	void update(const double&) override {}
//	double precision(const double&) const override { return 0.0; }
	void reset(bool) noexcept override {}
};

} // namespace fig

#endif // CONFIDENCEINTERVALRESULT_H
