//==============================================================================
//
//  VariableSet.h
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


#ifndef VARIABLESET_H
#define VARIABLESET_H

// C++
#include <string>
#include <vector>
// Project code
#include <Variable.h>

namespace fig
{

/**
 * @brief Variable defined by a set of possible values: { v1, v2, ..., vN }
 * @note  Useful to handle non-integer values such as, for instance, floats,
 *        which the class VariableInterval can't handle.
 */
template< typename T_ >
class VariableSet : public Variable<T_>
{
	std::vector< T_ > values_;
	T_ min_;
	T_ max_;

public:  // Ctors/Dtor

	// Fresh variable
	VariableSet() {}
	// Named variable
	/// Copy content from any container with internal data type equal to T_
	template< class Set_ > VariableSet(const std::string& thename, const Set_& setOfValues);
	/// Move content from any container with internal data type equal to T_
	template< class Set_ > VariableSet(const std::string& thename, Set_&& setOfValues);
	/// Copy content between iterators 'from' and 'to' with internal data type equal to T_
	template< class Iter_ > VariableSet(const std::string& thename, Iter_ from, Iter_ to);
	/// Copy content from static array of specified size
	VariableSet(const std::string& thename, const T_ *array, size_t arraySize);

	// It should be safe to use compiler's both move and copy ctors
	VariableSet(const VariableSet& that) = default;
	VariableSet(VariableSet&& that) = default;

	/**
	 * @brief Copy assignment with copy&swap
	 * @note Only applicable to fresh variables
	 */
	VariableSet<T_>& operator=(VariableSet<T_> that);

	virtual ~VariableSet() { values_.clear(); }

public:  // Accessors

	inline T_ min() const noexcept { return min_; }
	inline T_ max() const noexcept { return max_; }
	inline T_ val() const noexcept { return values_[Variable<T_>::offset_]; }
	inline T_ val(const size_t& offset) const { return values_[offset]; }

public:  // Modifiers

	/**
	 * @brief Value assignment
	 * @note  Only applicable to named variables
	 * @throw FigException if value isn't valid, see is_valid_value()
	 */
	virtual VariableSet& operator=(const T_& value);

public:  // Relational operators

	virtual bool operator==(const Variable<T_>& that) const;
	virtual bool operator==(const VariableSet<T_>& that) const;
	virtual bool is_valid_value(const T_& val) const;

public:  // Invariant

#ifndef NDEBUG
	virtual void assert_invariant() const;
#else
	inline void assert_invariant() const {}
#endif
};

} // namespace fig

#endif // VARIABLESET_H
