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
#include <utility>  // std::move()
#include <limits>   // std::numeric_limits
// C
#include <cassert>
// Project code
#include <Variable.h>


namespace fig
{

//  Base Variable  /////////////////////////////////////////////////////////////

template< typename T_ >
Variable<T_>::Variable(const std::string& thename) :
	name(thename),
	range_(1),
	offset_(0)
{
	assert_invariant();
}


template< typename T_ >
Variable<T_>::Variable(std::string&& thename) :
	name(std::move(thename)),
	range_(1),
	offset_(0)
{
	assert_invariant();
}



//  Interval Variable  /////////////////////////////////////////////////////////

template< typename T_ >
VariableInterval<T_>::VariableInterval(const std::string& thename,
									   const T_& min,
									   const T_& max) :
	Variable(thename),
	min_(min),
	max_(max)
{
	range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	offset_ = 0u;
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(const std::string& thename,
									   const T_& min,
									   const T_& max,
									   const T_& val) :
	Variable(thename),
	min_(min),
	max_(max)
{
	assert(min_ <= val);
	assert(val <= max_);
	range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	offset_ = val - min_;
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(std::string&& thename,
									   const T_& min,
									   const T_& max) :
	Variable(thename),
	min_(min),
	max_(max)
{
	range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	offset_ = 0u;
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(std::string&& thename,
									   const T_& min,
									   const T_& max,
									   const T_& val) :
	Variable(thename),
	min_(min),
	max_(max)
{
	assert(min_ <= val);
	assert(val <= max_);
	range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	offset_ = val - min_;
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(const VariableDeclaration< T_ >& dec) :
	Variable(std::get<0>(dec)),
	min_(std::get<1>(dec)),
	max_(std::get<2>(dec))
{
	range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	offset_ = 0u;
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(const VariableDefinition< T_ >& def) :
	Variable(std::get<0>(def)),
	min_(std::get<1>(def)),
	max_(std::get<2>(def))
{
	T_ val(std::get<3>(def));
	assert(min_ <= val);
	assert(val <= max_);
	range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	offset_ = val - min_;
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(VariableDeclaration< T_ >&& dec) :
	Variable(std::move(std::get<0>(dec))),
	min_(std::move(std::get<1>(dec))),
	max_(std::move(std::get<2>(dec)))
{
	range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	offset_ = 0u;
	assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(VariableDefinition< T_ >&& def) :
	Variable(std::move(std::get<0>(def))),
	min_(std::move(std::get<1>(def))),
	max_(std::move(std::get<2>(def)))
{
	T_ val(std::move(std::get<3>(def)));
	assert(min_ <= val);
	assert(val <= max_);
	range_ = static_cast<size_t>(max_ - min_) + 1;  // closed interval [min,max]
	offset_ = val - min_;
	assert_invariant();
}


template< typename T_ >
virtual bool
VariableInterval<T_>::operator==(const VariableInterval<T_>& that) const
{
	return name == that.name &&
		   min_ == that.min_ &&
		   max_ == that.max_ &&
		   offset_ == that.offset_;
}



//  Set Variable  //////////////////////////////////////////////////////////////

template< typename T_ >
template< class Set >
VariableSet<T_>::VariableSet(const Set& setOfValues) :
	values(std::vector<T_>(setOfValues.size())),
	min_(std::numeric_limits<T_>::max()),
	max_(std::numeric_limits<T_>::min())
{
	static_assert(std::is_same< T_, typename Set::value_type >::value,
				  "ERROR: construction container internal data type "
				  "and this template type (T_) must be the same");
	size_t i(0u);
	for (const auto& e: setOfValues) {
		values[i++] = e;
		min_ = e < min_ ? e : min_;
		max_ = e > max_ ? e : max_;
	}
}

/// TODO
//  Implement ctors


template< typename T_ >
virtual bool
VariableSet<T_>::operator==(const VariableSet<T_>& that) const
{
	if (name != that.name || range_ != that.range_ || offset_ != that.offset_)
		return false;
	for (size_t i = 0u ; i < values.size() ; i++)
		if (values[i] != that.values[i])
			return false;
	return true;
}


template< typename T_ >
virtual bool
VariableSet<T_>::is_valid_value(const T_& val) const
{
	for (const auto& e: *values)
		if (val==e) return true;
	return false;
}


template< typename T_ >
virtual bool
VariableSet<T_>::assert_invariant() const
{
	Variable::assert_invariant();
	for (const auto& e: values)
		assert(min_ <= e && e <= max_);
}


} // namespace fig
