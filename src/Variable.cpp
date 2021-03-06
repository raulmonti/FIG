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
//	along with FIG; if not, write to the Free Software Foundation,
//	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//==============================================================================


// C++
#include <utility>   // std::move()
#include <limits>    // std::numeric_limits
#include <iterator>  // std::distance
// C
#include <cassert>
// FIG
#include <Variable.h>
#include <FigException.h>


namespace fig
{

template< typename T_ >
Variable<T_>::Variable(const std::string& thename, T_ themin, T_ themax, T_ theini) :
	name_(thename),
	min_(themin),
	max_(themax),
	ini_(theini),
	range_(1),
	offset_(0)
{
	assert_invariant();
}


template< typename T_ >
Variable<T_>::Variable(std::string&& thename, T_ themin, T_ themax, T_ theini) :
	name_(std::move(thename)),
	min_(themin),
	max_(themax),
	ini_(theini),
	range_(1),
	offset_(0)
{
	assert_invariant();
}


template< typename T_ >
Variable<T_>::Variable(const Variable<T_>& that) :
	min_(that.min_),
	max_(that.max_),
	ini_(that.ini_),
	range_(that.range_),
	offset_(that.offset_)
{
	if (!name_.empty()) {
		std::string err("trying to copy-construct the non-fresh variable \"");
		throw_FigException(err.append(name_).append("\""));
	}
	name_.assign(that.name_);
}


template< typename T_ >
Variable<T_>::Variable(Variable<T_>&& that) :
	min_(that.min_),
	max_(that.max_),
	ini_(that.ini_),
	range_(std::move(that.range_)),
	offset_(std::move(that.offset_))
{
	if (!name_.empty()) {
		std::string err("trying to move-construct the non-fresh variable \"");
		throw_FigException(err.append(name_).append("\""));
	}
	name_.assign(std::move(that.name_));
}


template< typename T_ >
Variable<T_>&
Variable<T_>::operator=(const Variable<T_>& that)
{
	if (!name_.empty()) {
		std::string err("trying to copy-assign the non-fresh variable \"");
		throw_FigException(err.append(name_).append("\""));
	}
	name_   = that.name_;
	min_    = that.min_;
	max_    = that.max_;
	ini_    = that.ini_;
	range_  = that.range_;
	offset_ = that.offset_;
	assert_invariant();
	return *this;
}


template< typename T_ >
Variable<T_>&
Variable<T_>::operator=(Variable<T_>&& that)
{
	if (!name_.empty()) {
		std::string err("trying to copy-assign the non-fresh variable \"");
		throw_FigException(err.append(name_).append("\""));
	}
	name_   = std::move(that.name_);
	min_    = std::move(that.min_);
	max_    = std::move(that.max_);
	ini_    = std::move(that.ini_);
	range_  = std::move(that.range_);
	offset_ = std::move(that.offset_);
	assert_invariant();
	return *this;
}


template< typename T_ >
T_
Variable<T_>::inc()
{
	T_ oldVal = val();
	if (offset_+1ul < range_)
		offset_++;
	return oldVal;
}


template< typename T_ >
T_
Variable<T_>::dec()
{
	T_ oldVal = val();
	if (0ul < offset_)
		offset_--;
	return oldVal;
}


// Variable can only be instantiated with following numeric types
template class Variable< short              >;
template class Variable< int                >;  // MuParser can't
template class Variable< long               >;
template class Variable< long long          >;
template class Variable< unsigned short     >;
template class Variable< unsigned int       >;
template class Variable< unsigned long      >;
template class Variable< unsigned long long >;
template class Variable< float              >;
template class Variable< double             >;
template class Variable< long double        >;


} // namespace fig
