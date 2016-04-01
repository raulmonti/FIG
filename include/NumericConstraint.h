//==============================================================================
//
//  NumericConstraint.h
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

#ifndef NUMERICCONSTRAINT_H
#define NUMERICCONSTRAINT_H

// C++
#include <string>
#include <type_traits>
// External code
#include <Constraint.h>


namespace TCLAP
{

/**
 * A Constraint only valid for numeric types, that constrains the Arg
 * to any numerical condition imposed by the user.
 */
template< typename T_ >
class NumericConstraint : public Constraint<T_>
{
	static_assert(std::is_arithmetic< T_ >::value, "ERROR: type mismatch. "
				  "NumericConstraint class template can only be instantiated "
				  "with numeric types.");
public:

	/// Ctor
	/// @param constraint User defined function to constrain the values,
	///                   any with signature "bool operator()(const T_&)"
	/// @param description Description of what does 'constraint' constrain
	template< class FUN >
	NumericConstraint(FUN constraint, const std::string description) :
		constraint_(constraint),
		descritpion_(description)
		{}

	virtual ~NumericConstraint() {}

	inline std::string description() const override { return descritpion_; }

	inline std::string shortID() const override { return descritpion_; }

	inline bool check(const T_& value) const override { return constraint_(value); }

protected:

	/// The constraint per se, as defined by the user
	bool(*constraint_)(const T_&);

	/// User provided description of what does the constraint_ constrain
	const std::string descritpion_;
};

} // namespace TCLAP

#endif // NUMERICCONSTRAINT_H
