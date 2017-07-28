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


namespace fig
{

/// Observer class for ConfidenceInterval,
/// used e.g. to show resulting estimates to the user.
/// @warning Immutable: instances of this class are unfit for estimations
class ConfidenceIntervalResult : public ConfidenceInterval
{
	/// (Derived) Instance of ConfidenceInterval we're wrapping
	std::shared_ptr< ConfidenceInterval > instance_;

public:  // Ctors from the other derived classes of ConfidenceInterval

	/// Ctor from shared_ptr
	/// @todo Can we make a ctor taking a const ConfidenceInterval& ?
	ConfidenceIntervalResult(std::shared_ptr<ConfidenceInterval> ci = nullptr) :
	    ConfidenceInterval(*ci),
	    instance_(ci)
	{ /* Not much to do around here... */ }

public:  // Methods linked to the real functions of our creation class

	/// Stub to method of creation class
	bool min_samples_covered(bool considerEpsilon = false) const noexcept override
	    { assert(nullptr != instance_); return instance_->min_samples_covered(considerEpsilon); }

	/// Stub to method of creation class
	double precision(const double& confco) const override
	    { assert(nullptr != instance_); return instance_->precision(confco); }

private:  // This class implements an observer: non-const methods are banned

	void set_statistical_oversampling(const double&) override {}
	void set_variance_correction(const double&)      override {}
	void update(const double&)                       override {}
	void reset(bool) noexcept                        override {}
};

} // namespace fig

#endif // CONFIDENCEINTERVALRESULT_H
