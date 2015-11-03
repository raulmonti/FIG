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
#include <limits>       // std::numeric_limits<>
#include <utility>      // std::move()
#include <iterator>     // std::distance()
#include <type_traits>  // std::is_same<>
#include <string>
#include <vector>
// Project code
#include <Variable.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


namespace fig
{

/**
 * @brief Variable defined by a set of possible values: { v1, v2, ..., vN }
 *        Useful to handle non-integer values, such as floats or strings,
 *        which the class VariableInterval can't cope with.
 * @note  Offers generic construction from the following STL containers:
 *        vector, list, forward_list, set, unordered_set, deque.
 * @note  Will not build from the following STL containers:
 *        queue, stack, array.
 * @note  Generic construction is achieved through variadic template templates,
 *        see: http://eli.thegreenplace.net/2014/variadic-templates-in-c/
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
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	VariableSet(const std::string& thename,
				const Container<ValueType, OtherContainerArgs...>& values);
	/// Move content from any container with internal data type equal to T_
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	VariableSet(const std::string &thename,
				Container<ValueType, OtherContainerArgs...>&& values);
	/// Copy content between iterators 'from' and 'to' pointing to data type equal to T_
	template< template< typename, typename... > class Iterator,
			  typename ValueType,
			  typename... OtherIteratorArgs >
	VariableSet(const std::string& thename,
				Iterator<ValueType, OtherIteratorArgs...> from,
				Iterator<ValueType, OtherIteratorArgs...> to);
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


// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of the source file

template< typename T_ >
template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerParameters >
// Variadic template templates and Schwarzenegger singing Merry Christmas
VariableSet<T_>::VariableSet(
	const std::string& thename,
	const Container< ValueType, OtherContainerParameters... >& values) :
		Variable<T_>(thename),
		min_(std::numeric_limits<T_>::max()),
		max_(std::numeric_limits<T_>::min())
{
	static_assert(std::is_same< T_, ValueType >::value,
				  "ERROR: type missmatch, container internal data type "
				  "must match the type of this template class");
	for(const auto& e: values) {
		values_.emplace_back(e);
		min_ = e < min_ ? e : min_;
		max_ = e > max_ ? e : max_;
	}
	Variable<T_>::range_ = values_.size();
	assert_invariant();
}


template< typename T_ >
template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
VariableSet<T_>::VariableSet(
	const std::string& thename,
	Container<ValueType, OtherContainerArgs...>&& values) :
		Variable<T_>(std::move(thename)),
		min_(std::numeric_limits<T_>::max()),
		max_(std::numeric_limits<T_>::min())
{
	static_assert(std::is_same< T_, ValueType >::value,
				  "ERROR: type missmatch, container internal data type "
				  "must match the type of this template class");
	for(const auto& e: values) {
		values_.emplace_back(std::move(e));
		min_ = e < min_ ? e : min_;
		max_ = e > max_ ? e : max_;
	}
	values.clear();
	Variable<T_>::range_ = values_.size();
	assert_invariant();
}


template< typename T_ >
template< template< typename, typename... > class Iterator,
		  typename ValueType,
		  typename... OtherIteratorArgs >
VariableSet<T_>::VariableSet(
	const std::string& thename,
	Iterator<ValueType, OtherIteratorArgs...> from,
	Iterator<ValueType, OtherIteratorArgs...> to) :
		Variable<T_>(thename),
		values_(std::distance(from,to)),
		min_(std::numeric_limits<T_>::max()),
		max_(std::numeric_limits<T_>::min())
{
	static_assert(std::is_same< T_, ValueType >::value,
				  "ERROR: type missmatch, iterator pointed-to data type "
				  "must match the type of this template class");
	Variable<T_>::range_ = values_.size();
	size_t i(0u);
	do {
		values_[i++] = *from;
		min_ = *from < min_ ? *from : min_;
		max_ = *from > max_ ? *from : max_;
	} while (++from != to);
	assert_invariant();
}


} // namespace fig

#endif // VARIABLESET_H
