//==============================================================================
//
//  Variable.h
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
//	along with FIG; if not, write to the Free Software Foundation,
//	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//==============================================================================


#ifndef VARIABLE_H
#define VARIABLE_H

// C++
#include <type_traits>  // std::is_integral<>
#include <string>
#include <vector>
#include <tuple>
// C
#include <cassert>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

namespace fig
{

/**
 * @brief Abstract variable concept.
 *
 *        The State of the system is composed of these.
 *
 * @todo Describe fresh/named variable duality
 */
template< typename T_ >
class Variable
{
	// Friend template class: http://stackoverflow.com/a/8967610
	template< typename TT_ > friend class State;

protected:  // Attributes

	/// Name can only be assigned once (fresh variable concept)
	std::string name_;
	/// Minimum value for this variable
	T_ min_;
	/// Maximum value for this variable
	T_ max_;
	/// Initial value for this variable
	T_ ini_;
	/// Number of distinct values this variable can take
	size_t range_;
	/// Position in [0, 1, ..., range) for the "current" value of the Variable
	size_t offset_;

public:  // Ctors/Dtor

	// Fresh variable (aka unnamed)
	Variable() {}
	// Named variable
	Variable(const std::string& thename, T_ themin, T_ themax, T_ theini);
	Variable(std::string&& thename, T_ themin, T_ themax, T_ theini);
	/// Copy and move ctors, only applicable to fresh variables
	Variable(const Variable<T_>& that);
	Variable(Variable<T_>&& that);
	/// Copy assignment, only applicable to fresh variables
	Variable& operator=(const Variable<T_>& that);
	Variable& operator=(Variable<T_>&& that);

	virtual ~Variable() {}

public:  // Accessors

	/// @copydoc name_
	inline const std::string& name() const noexcept { return name_; }
	/// @copydoc min_
	inline const T_& min() const noexcept { return min_; }
	/// @copydoc max_
	inline const T_& max() const noexcept { return max_; }
	/// @copydoc ini_
	inline const T_& ini() const noexcept { return ini_; }
	/// @copydoc range_
	inline const size_t& range() const noexcept { return range_; }
	/// Current value
	virtual T_ val() const noexcept = 0;
	/// Value corresponding to passed offset
	virtual T_ val(const size_t& offset) const = 0;

public:  // Modifiers


	/// @brief Value assignment
	/// @note  Only applicable to named variables
	virtual Variable& operator=(const T_& value) = 0;


	/// @brief Value assignment with prior validity check
	/// @note  Only applicable to named variables
	/// @throw FigException if 'value' is invalid, see is_valid_value()
	virtual void assign(const T_& value) = 0;

	/// @brief Increase Variable's current value if possible
	/// @details In the internal order of the possible Variable values,
	///          set as new value the one following the \ref val() "current
	///          value". If the current value is \ref max() "the maximum" for
	///          this variable then do nothing.
	/// @return Old value of the variable, i.e. the one prior the increase
	/// @note Only applicable to named variables
	T_ inc();

	/// @brief Decrease Variable's current value if possible
	/// @note Analogous to increase() in the opposite sense
	T_ dec();

public:  // Relational operators

	/// @note Also compares "current value", i.e. offset_
	virtual bool operator==(const Variable<T_>& that) const = 0;
	inline  bool operator!=(const Variable<T_>& that) const { return !(*this == that);}

	/// @brief Is 'val' a valid value for this Variable?
	virtual bool is_valid_value(const T_& val) const = 0;

public:  // Invariant

#ifndef NDEBUG
	/**
	 * @brief Is this instance a "ready to use" variable?
	 * @note False for fresh variables!
	 */
	inline void assert_invariant() const
		{
			assert(!name_.empty());
			assert(min_ <= ini_);
			assert(ini_ <= max_);
			assert(offset_ < range_);
		}
#else
	inline void assert_invariant() const {}
#endif
};

} // namespace fig

#endif // VARIABLE_H
