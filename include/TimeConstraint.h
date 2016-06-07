//==============================================================================
//
//  TimeConstraint
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
//------------------------------------------------------------------------------
//
//  This file is an extension to the Templatized C++ Command Line Parser
//  by Michael E. Smoot (TCLAP library, Copyright (c) 2003-2011)
//  All credit regarding this single file should go to him.
//
//==============================================================================

#ifndef TIMECONSTRAINT_H
#define TIMECONSTRAINT_H

// C
#include <cctype>  // std::isalpha()
// C++
#include <string>
#include <type_traits>
// External code
#include <Constraint.h>


namespace TCLAP
{

/**
 * A Constraint meant for expressing time durations,
 * following the GNU coreutils 'timeout' interface.
 */
template< typename T_ >
class TimeConstraint : public Constraint<T_>
{
	static_assert(std::is_constructible< std::string, T_ >::value,
				  "ERROR: type mismatch. TimeConstraint class template can only"
				  " be instantiated from a string expressing a time duration.");
public:

	/// Ctor
	TimeConstraint() {}

	/// Dtor
	virtual ~TimeConstraint() {}

	inline std::string description() const override { return description_; }

	inline std::string shortID() const override { return description_; }

	inline bool check(const T_& value) const override {
		std::string time(value);
		char *err(nullptr), timeUnit(time.back());
		if (!(std::isdigit(timeUnit) || 's' == timeUnit
			  || 'm' == timeUnit || 'h' == timeUnit || 'd' == timeUnit))
			return false;
		else if (std::isalpha(timeUnit))
			time.resize(time.length()-1);
		std::strtoul(time.data(), &err, 10);
		if (nullptr != err && err[0] != '\0')
			return false;
		else
			return true;
	}

private:

	std::string description_ = "{0..9}+[<s/m/h/d>]";
};

} // namespace TCLAP

#endif // TIMECONSTRAINT_H
