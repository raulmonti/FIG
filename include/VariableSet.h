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
//	along with FIG; if not, write to the Free Software Foundation,
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
// FIG
#include <Variable.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


namespace fig
{

/**
 * @brief Variable implementation defined by a set of possible values:
 *        { v1, v2, ..., vN }
 *
 *        Useful to handle non-integer values, such as floats or strings,
 *        which the VariableInterval class can't cope with.
 *
 * @see   VariableInterval
 *
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

	/// Move content from any container with internal data type equal to T_*
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	VariableSet(const std::string &thename,
				Container<ValueType*, OtherContainerArgs...>&& values);

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
	 * @note Only applicable to fresh variables, see Variable::operator=()
	 */
	VariableSet<T_>& operator=(VariableSet<T_> that);

	virtual ~VariableSet() { values_.clear(); }

public:  // Accessors

	inline T_ val() const noexcept override final
		{ return values_[Variable<T_>::offset_]; }
	inline T_ val(const size_t& offset) const override final
		{ return values_[offset]; }

public:  // Modifiers

	/// @copydoc Variable::operator=()
	/// @note If 'value' is invalid then it won't be applied
	VariableSet& operator=(const T_& value) override final;

	/// @copydoc Variable::assign()
	void assign(const T_& value) override final;

public:  // Relational operators

	bool operator==(const VariableSet<T_>& that) const;
	bool operator<=(const VariableSet<T_>& that) const;

	inline bool operator==(const Variable<T_>& that) const override
	    {
		    auto pthat = dynamic_cast<const VariableSet<T_>*>(&that);
			return nullptr != pthat && (*this) == (*pthat);
	    }

	inline bool operator<=(const Variable<T_>& that) const override
	    {
		    auto pthat = dynamic_cast<const VariableSet<T_>*>(&that);
			return nullptr != pthat && (*this) <= (*pthat);
	    }

	/// @copydoc Variable::is_valid_value()
	bool is_valid_value(const T_& val) const override final;

public:  // Invariant

#ifndef NDEBUG
	void assert_invariant() const override;
#else
	inline void assert_invariant() const override {}
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
		Variable<T_>(thename,
					 static_cast<T_>(0),
					 static_cast<T_>(0),
					 static_cast<T_>(0))
{
	static_assert(std::is_same< T_, ValueType >::value,
				  "ERROR: type mismatch. Container internal data type "
				  "must match the type of this template class");

	Variable<T_>::min_ = std::numeric_limits<T_>::max();
	Variable<T_>::max_ = std::numeric_limits<T_>::min();

	for(const auto& e: values) {
		values_.emplace_back(e);
		Variable<T_>::min_ = e < Variable<T_>::min_ ? e : Variable<T_>::min_;
		Variable<T_>::max_ = e > Variable<T_>::max_ ? e : Variable<T_>::max_;
	}

	Variable<T_>::offset_ = 0;
	Variable<T_>::range_  = values_.size();
	Variable<T_>::ini_    = values_[Variable<T_>::offset_];

	assert_invariant();
}


// This is the move ctor from object references
template< typename T_ >
template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
VariableSet<T_>::VariableSet(
	const std::string& thename,
	Container<ValueType, OtherContainerArgs...>&& values) :
		Variable<T_>(thename,
					 static_cast<T_>(0),
					 static_cast<T_>(0),
					 static_cast<T_>(0))
{
	static_assert(std::is_same< T_, ValueType >::value,
				  "ERROR: type mismatch. Container internal data type "
				  "must match the type of this template class");

	Variable<T_>::min_ = std::numeric_limits<T_>::max();
	Variable<T_>::max_ = std::numeric_limits<T_>::min();

	for(auto& e: values) {
		values_.emplace_back(std::move(e));
		Variable<T_>::min_ = e < Variable<T_>::min_ ? e : Variable<T_>::min_;
		Variable<T_>::max_ = e > Variable<T_>::max_ ? e : Variable<T_>::max_;
	}
	values.clear();

	Variable<T_>::offset_ = 0;
	Variable<T_>::range_  = values_.size();
	Variable<T_>::ini_    = values_[Variable<T_>::offset_];

	assert_invariant();
}


// This is the move ctor from object pointers
template< typename T_ >
template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
VariableSet<T_>::VariableSet(
	const std::string& thename,
	Container<ValueType*, OtherContainerArgs...>&& values) :
		Variable<T_>(thename,
					 static_cast<T_>(0),
					 static_cast<T_>(0),
					 static_cast<T_>(0))
{
	static_assert(std::is_same< T_, ValueType >::value,
				  "ERROR: type mismatch. Container internal data type pointer "
				  "must match the type of this template class");

	Variable<T_>::min_ = std::numeric_limits<T_>::max();
	Variable<T_>::max_ = std::numeric_limits<T_>::min();

	for(const auto& e: values) {
		values_.emplace_back(std::move(*e));
		Variable<T_>::min_ = e < Variable<T_>::min_ ? e : Variable<T_>::min_;
		Variable<T_>::max_ = e > Variable<T_>::max_ ? e : Variable<T_>::max_;
		e = nullptr;
	}
	values.clear();

	Variable<T_>::offset_ = 0;
	Variable<T_>::range_  = values_.size();
	Variable<T_>::ini_    = values_[Variable<T_>::offset_];

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
		Variable<T_>(thename,
					 static_cast<T_>(0),
					 static_cast<T_>(0),
					 static_cast<T_>(0)),
		values_(std::distance(from,to))
{
	static_assert(std::is_same< T_, ValueType >::value,
				  "ERROR: type mismatch. Iterator pointed-to data type "
				  "must match the type of this template class");

	Variable<T_>::min_ = std::numeric_limits<T_>::max();
	Variable<T_>::max_ = std::numeric_limits<T_>::min();

	size_t i(0u);
	do {
		values_[i++] = *from;
		Variable<T_>::min_ = *from < Variable<T_>::min_ ? *from : Variable<T_>::min_;
		Variable<T_>::max_ = *from > Variable<T_>::max_ ? *from : Variable<T_>::max_;
	} while (++from != to);

	Variable<T_>::offset_ = 0;
	Variable<T_>::range_  = values_.size();
	Variable<T_>::ini_    = values_[Variable<T_>::offset_];

	assert_invariant();
}


} // namespace fig

#endif // VARIABLESET_H
