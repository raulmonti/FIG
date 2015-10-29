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


} // namespace fig
