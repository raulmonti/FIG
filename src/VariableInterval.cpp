//==============================================================================
//
//  VariableInterval.cpp
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
#include <tuple>    // std::get<>
#include <utility>  // std::move()
// C
#include <cassert>
// Project code
#include <VariableInterval.h>
#include <FigException.h>


namespace fig
{

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
void
VariableInterval<T_>::assign(const T_& value)
{
	if (Variable<T_>::name_.empty())
		throw FigException(std::string("can't assign value to a fresh variable")
			.append(" (\"").append(Variable<T_>::name_).append("\")"));
	else if (!is_valid_value(value))
		throw FigException(std::string("can't assign ")
			.append(std::to_string(value)).append(" to variable \"")
			.append(Variable<T_>::name_).append("\", invalid value"));
	else
		Variable<T_>::offset_ = value - min_;
}


template< typename T_ >
bool
VariableInterval<T_>::operator==(const Variable<T_>& that) const
{
	auto pthat = dynamic_cast<const VariableInterval<T_>*>(&that);
	if (nullptr == pthat)
		return false;
	else
		return (*this) == (*pthat);
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


} // namespace fig
