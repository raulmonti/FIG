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
//	along with FIG; if not, write to the Free Software Foundation,
//	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//==============================================================================


// C++
#include <tuple>    // std::get<>
#include <utility>  // std::move()
// C
#include <cassert>
// FIG
#include <VariableInterval.h>
#include <FigException.h>


namespace fig
{

template< typename T_ >
VariableInterval<T_>::VariableInterval(const std::string& thename,
									   const T_& min,
									   const T_& max) :
	Variable<T_>(thename, min, max, min)
{
	Variable<T_>::offset_ = 0u;
	Variable<T_>::range_ =  // closed interval [min,max]
		static_cast<size_t>(Variable<T_>::max_ - Variable<T_>::min_) + 1;
	Variable<T_>::assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(const std::string& thename,
									   const T_& min,
									   const T_& max,
									   const T_& val) :
	Variable<T_>(thename, min, max, val)
{
	Variable<T_>::offset_ = val - Variable<T_>::min_;
	Variable<T_>::range_ =  // closed interval [min,max]
		static_cast<size_t>(Variable<T_>::max_ - Variable<T_>::min_) + 1;
	Variable<T_>::assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(std::string&& thename,
									   const T_& min,
									   const T_& max) :
	Variable<T_>(thename, min, max, min)
{
	Variable<T_>::offset_ = 0u;
	Variable<T_>::range_ =  // closed interval [min,max]
		static_cast<size_t>(Variable<T_>::max_ - Variable<T_>::min_) + 1;
	Variable<T_>::assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(std::string&& thename,
									   const T_& min,
									   const T_& max,
									   const T_& val) :
	Variable<T_>(thename, min, max, val)
{
	Variable<T_>::offset_ = val - Variable<T_>::min_;
	Variable<T_>::range_ =  // closed interval [min,max]
		static_cast<size_t>(Variable<T_>::max_ - Variable<T_>::min_) + 1;
	Variable<T_>::assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(const VariableDeclaration< T_ >& dec) :
	Variable<T_>(std::get<0>(dec),
				 std::get<1>(dec),
				 std::get<2>(dec),
				 std::get<1>(dec))
{
	Variable<T_>::offset_ = 0u;
	Variable<T_>::range_ =  // closed interval [min,max]
		static_cast<size_t>(Variable<T_>::max_ - Variable<T_>::min_) + 1;
	Variable<T_>::assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(const VariableDefinition< T_ >& def) :
	Variable<T_>(std::get<0>(def),
				 std::get<1>(def),
				 std::get<2>(def),
				 std::get<3>(def))
{
	Variable<T_>::offset_ = Variable<T_>::ini_ - Variable<T_>::min_;
	Variable<T_>::range_ =  // closed interval [min,max]
		static_cast<size_t>(Variable<T_>::max_ - Variable<T_>::min_) + 1;
	Variable<T_>::assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(VariableDeclaration< T_ >&& dec) :
	Variable<T_>(std::move(std::get<0>(dec)),
						  (std::get<1>(dec)),  // don't move, for initial value
				 std::move(std::get<2>(dec)),
				 std::move(std::get<1>(dec)))
{
	Variable<T_>::offset_ = 0u;
	Variable<T_>::range_ =  // closed interval [min,max]
		static_cast<size_t>(Variable<T_>::max_ - Variable<T_>::min_) + 1;
	Variable<T_>::assert_invariant();
}


template< typename T_ >
VariableInterval<T_>::VariableInterval(VariableDefinition< T_ >&& def) :
	Variable<T_>(std::move(std::get<0>(def)),
				 std::move(std::get<1>(def)),
				 std::move(std::get<2>(def)),
				 std::move(std::get<3>(def)))
{
	Variable<T_>::offset_ = Variable<T_>::ini_ - Variable<T_>::min_;
	Variable<T_>::range_ =  // closed interval [min,max]
		static_cast<size_t>(Variable<T_>::max_ - Variable<T_>::min_) + 1;
	Variable<T_>::assert_invariant();
}


template< typename T_ >
VariableInterval<T_>&
VariableInterval<T_>::operator=(VariableInterval<T_> that)
{
	Variable<T_>::operator=(std::move(that));  // checks freshness
	std::swap(Variable<T_>::offset_, that.offset_);
	std::swap(Variable<T_>::range_, that.range_);
	Variable<T_>::assert_invariant();
	return *this;
}


template< typename T_ >
VariableInterval<T_>&
VariableInterval<T_>::operator=(VariableDeclaration<T_> dec)
{
	VariableInterval<T_> tmp(std::move(dec));  // checks freshness
	Variable<T_>::operator=(std::move(tmp));
	Variable<T_>::offset_ = 0u;
	Variable<T_>::range_ =  // closed interval [min,max]
		static_cast<size_t>(Variable<T_>::max_ - Variable<T_>::min_) + 1;
	Variable<T_>::assert_invariant();
	return *this;
}


template< typename T_ >
VariableInterval<T_>&
VariableInterval<T_>::operator=(VariableDefinition<T_> def)
{
	VariableInterval<T_> tmp(std::move(def));  // checks freshness
	Variable<T_>::operator=(std::move(tmp));
	Variable<T_>::offset_ = std::get<3>(def) - Variable<T_>::min_;
	Variable<T_>::range_ =  // closed interval [min,max]
		static_cast<size_t>(Variable<T_>::max_ - Variable<T_>::min_) + 1;
	Variable<T_>::assert_invariant();
	return *this;
}


template< typename T_ >
void
VariableInterval<T_>::assign(const T_& value)
{
	if (Variable<T_>::name_.empty())
		throw_FigException("can't assign value to fresh variable (\""
						   + Variable<T_>::name_ + "\")");
	else if (!is_valid_value(value))
		throw_FigException("can't assign " + std::to_string(value) + " to "
						   "variable \"" + Variable<T_>::name_ + "\", invalid value");
	else
		Variable<T_>::offset_ = static_cast<decltype(Variable<T_>::offset_)>(
		            value - Variable<T_>::min_);
}



// VariableInterval can only be instantiated with following numeric types
template class VariableInterval< short              >;
template class VariableInterval< int                >;   // MuParser can't
template class VariableInterval< long               >;
template class VariableInterval< long long          >;
template class VariableInterval< unsigned short     >;
template class VariableInterval< unsigned int       >;
template class VariableInterval< unsigned long      >;
template class VariableInterval< unsigned long long >;


} // namespace fig
