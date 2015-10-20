//==============================================================================
//
//  State.h
//
//  Copyleft 2015-
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
//	along with PRISM; if not, write to the Free Software Foundation,
//	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//==============================================================================


#ifndef STATE_H
#define STATE_H

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

// C++
#include <string>
// C
#include <cassert>


namespace fig
{

/// State
template< typename T_ > using State = std::vector< T_ >;
// State is an instantiation of values for each Variable in 'variables_g'
// Each Traial will have its unique State, which can be compared to
// the global vector 'variables_g' for consistency check.

template<typename T> class Variable;

///////////////////////////////////////////////////////////////////////////////
// This global-RO-vector should be the unique instance in the whole program
//////////////////////////////             ////////////////////////////////////
std::vector< Variable< int > > variables_g;
//////////////////////////////             ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


/// General Variable concept, the one to manipulate in higher abstraction levels
/// Variable exists only for value validation of State instances
template< typename T_ >
class Variable
{
	static_assert(std::is_integral<T_>::value,
				  "ERROR: class Variable<T> can only be instantiated "
				  "with integral types, e.g. int, short, unsigned.");

	Variable(const std::string& thename) : name(thename)
		{ assert(!name.empty()); }

	Variable(std::string&& thename) : name(std::move(thename))
		{ assert(!name.empty()); }

	/// @brief Get range, viz number of distinct values this Variable can take
	inline const T_& range() const noexcept { return range_; }

	/// @brief Tell whether 'val' is a valid value for this Variable
	virtual bool valid_value(const T_& val) const = 0;

public:
	const std::string& name;
protected:
	T_ range_;
};

/// Variable defined by an interval: [ min_value , max_value ]
template< typename T_ >
class VariableInterval : Variable<T_>
{
	T_ min_;
	T_ max_;

public:

	VariableInterval(const std::string& thename, const T_& min, const T_& max) :
		Variable(thename),
		min_(min),
		max_(max)
	{
		assert(min_ < max_);
		range_ = max_ - min_;
	}

	inline virtual bool valid_value(const T_& val) const
		{ return min <= val && val <= max; }
};

/// Variable defined by a set of possible values: { val1, val2, ..., valN }
template< typename T_ >
class VariableSet : Variable<T_>
{
	std::unique_ptr< const std::unordered_set< T_ > > values;

public:

	inline virtual bool valid_value(const T_& val) const
		{
			for (const auto& e: *values)
				if (val==e) return true;
			return false;
		}
};

} // namespace fig

#endif // VARIABLE_H
