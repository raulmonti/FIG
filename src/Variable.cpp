//==============================================================================
//
//  Variable.cpp
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


// C++
#include <utility>   // std::move()
#include <limits>    // std::numeric_limits
#include <iterator>  // std::distance
// C
#include <cassert>
// Project code
#include <Variable.h>
#include <FigException.h>


namespace fig
{

//  Base Variable  /////////////////////////////////////////////////////////////

template< typename T_ >
Variable<T_>::Variable()
{}


template< typename T_ >
Variable<T_>::Variable(const std::string& thename) :
	name_(thename),
	range_(1),
	offset_(0)
{
	assert_invariant();
}


template< typename T_ >
Variable<T_>::Variable(std::string&& thename) :
	name_(std::move(thename)),
	range_(1),
	offset_(0)
{
	assert_invariant();
}


template< typename T_ >
Variable<T_>&
Variable<T_>::operator=(const std::string& thename)
{
	if (!name_.empty()) {
		std::string err("trying to assign to non-fresh variable \"");
		throw new FigException(err.append(name_).append("\""));
	}
	name_ = thename;
	range_ = 1;
	offset_ = 0;
	assert_invariant();
	return *this;
}


template< typename T_ >
Variable<T_>&
Variable<T_>::operator=(std::string&& thename)
{
	if (!name_.empty()) {
		std::string err("trying to assign to non-fresh variable \"");
		throw new FigException(err.append(name_).append("\""));
	}
	name_ = std::move(thename);
	range_ = 1;
	offset_ = 0;
	assert_invariant();
	return *this;
}


template< typename T_ >
Variable<T_>&
Variable<T_>::operator=(const Variable<T_>& that)
{
	if (!name_.empty()) {
		std::string err("trying to assign to non-fresh variable \"");
		throw new FigException(err.append(name_).append("\""));
	}
	name_ = that.name_;
	range_ = that.range_;
	offset_ = that.offset_;
	assert_invariant();
	return *this;
}


template< typename T_ >
Variable<T_>&
Variable<T_>::operator=(Variable<T_>&& that)
{
	if (!name_.empty()) {
		std::string err("trying to assign to non-fresh variable \"");
		throw new FigException(err.append(name_).append("\""));
	}
	name_ = std::move(that.name_);
	range_ = std::move(that.range_);
	offset_ = std::move(that.offset_);
	assert_invariant();
	return *this;
}


// Variable can only be instantiated with following numeric types
template class Variable< short              >;
//template class Variable< int                >;  // MuParser can't
template class Variable< long               >;
template class Variable< long long          >;
template class Variable< unsigned short     >;
template class Variable< unsigned int       >;
template class Variable< unsigned long      >;
template class Variable< unsigned long long >;



//  Interval Variable  /////////////////////////////////////////////////////////

template< typename T_ >
VariableInterval<T_>::VariableInterval(const std::string& thename,
									   const T_& min,
									   const T_& max) :
	Variable<T_>(thename),
	min_(min),
	max_(max)
{
	Variable<T_>::offset_ = 0u;
	Variable<T_>::range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(const std::string& thename,
									   const T_& min,
									   const T_& max,
									   const T_& val) :
	Variable<T_>(thename),
	min_(min),
	max_(max)
{
	assert(min_ <= val);
	assert(val <= max_);
	Variable<T_>::offset_ = val - min_;
	Variable<T_>::range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(std::string&& thename,
									   const T_& min,
									   const T_& max) :
	Variable<T_>(thename),
	min_(min),
	max_(max)
{
	Variable<T_>::offset_ = 0u;
	Variable<T_>::range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(std::string&& thename,
									   const T_& min,
									   const T_& max,
									   const T_& val) :
	Variable<T_>(thename),
	min_(min),
	max_(max)
{
	assert(min_ <= val);
	assert(val <= max_);
	Variable<T_>::offset_ = val - min_;
	Variable<T_>::range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(const VariableDeclaration< T_ >& dec) :
	Variable<T_>(std::get<0>(dec)),
	min_(std::get<1>(dec)),
	max_(std::get<2>(dec))
{
	Variable<T_>::offset_ = 0u;
	Variable<T_>::range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(const VariableDefinition< T_ >& def) :
	Variable<T_>(std::get<0>(def)),
	min_(std::get<1>(def)),
	max_(std::get<2>(def))
{
	T_ val(std::get<3>(def));
	assert(min_ <= val);
	assert(val <= max_);
	Variable<T_>::offset_ = val - min_;
	Variable<T_>::range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(VariableDeclaration< T_ >&& dec) :
	Variable<T_>(std::move(std::get<0>(dec))),
	min_(std::move(std::get<1>(dec))),
	max_(std::move(std::get<2>(dec)))
{
	Variable<T_>::offset_ = 0u;
	Variable<T_>::range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(VariableDefinition< T_ >&& def) :
	Variable<T_>(std::move(std::get<0>(def))),
	min_(std::move(std::get<1>(def))),
	max_(std::move(std::get<2>(def)))
{
	T_ val(std::move(std::get<3>(def)));
	assert(min_ <= val);
	assert(val <= max_);
	Variable<T_>::offset_ = val - min_;
	Variable<T_>::range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>&
VariableInterval<T_>::operator=(VariableInterval<T_> that)
{
	Variable<T_>::operator=(std::move(that));  // checks freshness
	std::swap(min_, that.min_);
	std::swap(max_, that.max_);
	std::swap(Variable<T_>::offset_, that.offset_);
	std::swap(Variable<T_>::range_, that.range_);
	assert_invariant();
	return *this;
}


template< typename T_ >
VariableInterval<T_>&
VariableInterval<T_>::operator=(VariableDeclaration<T_> dec)
{
	Variable<T_>::operator=(std::move(std::get<0>(dec)));  // checks freshness
	std::swap(min_, std::get<1>(dec));
	std::swap(max_, std::get<2>(dec));
	Variable<T_>::offset_ = 0u;
	Variable<T_>::range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	assert_invariant();
	return *this;
}


template< typename T_ >
VariableInterval<T_>&
VariableInterval<T_>::operator=(VariableDefinition<T_> def)
{
	Variable<T_>::operator=(std::move(std::get<0>(def)));  // checks freshness
	std::swap(min_, std::get<1>(def));
	std::swap(max_, std::get<2>(def));
	Variable<T_>::offset_ = std::get<3>(def) - min_;
	Variable<T_>::range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	assert_invariant();
	return *this;
}


template< typename T_ >
bool
VariableInterval<T_>::operator==(const VariableInterval<T_>& that) const
{
	return Variable<T_>::name_ == that.name_ &&
		   min_ == that.min_ &&
		   max_ == that.max_ &&
		   Variable<T_>::offset_ == that.offset_;
}


// VariableInterval can only be instantiated with following numeric types
template class VariableInterval< short              >;
//template class VariableInterval< int                >;   // MuParser can't
template class VariableInterval< long               >;
template class VariableInterval< long long          >;
template class VariableInterval< unsigned short     >;
template class VariableInterval< unsigned int       >;
template class VariableInterval< unsigned long      >;
template class VariableInterval< unsigned long long >;



//  Set Variable  //////////////////////////////////////////////////////////////

template< typename T_ >
template< class Set_ >
VariableSet<T_>::VariableSet(const std::string &thename, const Set_& setOfValues) :
	Variable<T_>(thename),
	values_(setOfValues.size()),
	min_(std::numeric_limits<T_>::max()),
	max_(std::numeric_limits<T_>::min())
{
	static_assert(std::is_same< T_, typename Set_::value_type >::value,
				  "ERROR: construction container internal data type "
				  "and this template type must be the same");
	size_t i(0u);
	for (const auto& e: setOfValues) {
		values_[i++] = e;
		min_ = e < min_ ? e : min_;
		max_ = e > max_ ? e : max_;
	}
	assert_invariant();
}


template< typename T_ >
template< class Set_ >
VariableSet<T_>::VariableSet(const std::string &thename, Set_&& setOfValues) :
	Variable<T_>(thename),
	values_(setOfValues.size()),
	min_(std::numeric_limits<T_>::max()),
	max_(std::numeric_limits<T_>::min())
{
	static_assert(std::is_same< T_, typename Set_::value_type >::value,
				  "ERROR: construction container internal data type "
				  "and this template type must be the same");
	size_t i(0u);
	for (const auto& e: setOfValues) {
		values_[i++] = std::move(e);
		min_ = e < min_ ? e : min_;
		max_ = e > max_ ? e : max_;
	}
	setOfValues.clear();
	assert_invariant();
}


template< typename T_ >
template< class Iter_ >
VariableSet<T_>::VariableSet(const std::string &thename, Iter_ from, Iter_ to) :
	Variable<T_>(thename),
	values_(std::distance(from,to)),
	min_(std::numeric_limits<T_>::max()),
	max_(std::numeric_limits<T_>::min())
{
	static_assert(std::is_same< T_, typename Iter_::value_type >::value,
				  "ERROR: construction container internal data type "
				  "and this template type must be the same");
	size_t i(0u);
	do {
		values_[i++] = *from;
		min_ = *from < min_ ? *from : min_;
		max_ = *from > max_ ? *from : max_;
	} while (++from != to);
	assert_invariant();
}


template< typename T_ >
VariableSet<T_>::VariableSet(const std::string &thename, const T_ *array, size_t arraySize) :
	Variable<T_>(thename),
	values_(arraySize),
	min_(std::numeric_limits<T_>::max()),
	max_(std::numeric_limits<T_>::min())
{
	for (size_t i = 0u ; i < arraySize ; i++) {
		values_[i] = array[i];
		min_ = array[i] < min_ ? array[i] : min_;
		max_ = array[i] > max_ ? array[i] : max_;
	}
	assert_invariant();
}


template< typename T_ >
VariableSet<T_>&
VariableSet<T_>::operator=(VariableSet<T_> that)
{
	Variable<T_>::operator=(std::move(that));
	std::swap(Variable<T_>::range_, that.range_);
	std::swap(Variable<T_>::offset_, that.offset_);
	std::swap(values_, that.values_);
	std::swap(min_, that.min_);
	std::swap(max_, that.max_);
	assert_invariant();
	return *this;
}


template< typename T_ >
bool
VariableSet<T_>::operator==(const VariableSet<T_>& that) const
{
	if (  Variable<T_>::name_   != that.name_
	   || Variable<T_>::range_  != that.range_
	   || Variable<T_>::offset_ != that.offset_)
		return false;
	for (size_t i = 0u ; i < values_.size() ; i++)
		if (values_[i] != that.values_[i])
			return false;
	return true;
}


template< typename T_ >
bool
VariableSet<T_>::is_valid_value(const T_& val) const
{
	for (const auto& e: values_)
		if (val==e)
			return true;
	return false;
}


template< typename T_ >
void
VariableSet<T_>::assert_invariant() const
{
	Variable<T_>::assert_invariant();
	for (const auto& e: values_)
		assert(min_ <= e && e <= max_);
}


// VariableSet can only be instantiated with following numeric types
template class VariableSet< short              >;
//template class VariableSet< int                >;   // MuParser can't
template class VariableSet< long               >;
template class VariableSet< long long          >;
template class VariableSet< unsigned short     >;
template class VariableSet< unsigned int       >;
template class VariableSet< unsigned long      >;
template class VariableSet< unsigned long long >;



} // namespace fig
