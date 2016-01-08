//==============================================================================
//
//  VariableSet.cpp
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
#include <limits>       // std::numeric_limits<>
#include <utility>      // std::move()
#include <type_traits>  // std::is_same<>
// C
#include <cassert>
// FIG
#include <VariableSet.h>
#include <FigException.h>


namespace fig
{

template< typename T_ >
VariableSet<T_>::VariableSet(const std::string &thename, const T_ *array, size_t arraySize) :
	Variable<T_>(thename,
				 std::numeric_limits<T_>::max(),
				 std::numeric_limits<T_>::min(),
				 static_cast<T_>(0)),
	values_(arraySize)
{
	for (size_t i = 0u ; i < arraySize ; i++) {
		values_[i] = array[i];
		Variable<T_>::min_ = array[i] < Variable<T_>::min_ ? array[i] : Variable<T_>::min_;
		Variable<T_>::max_ = array[i] > Variable<T_>::max_ ? array[i] : Variable<T_>::max_;
	}
	Variable<T_>::offset_ = 0;
	Variable<T_>::range_  = values_.size();
	Variable<T_>::ini_    = values_[Variable<T_>::offset_];
	assert_invariant();
}


template< typename T_ >
VariableSet<T_>&
VariableSet<T_>::operator=(VariableSet<T_> that)
{
	Variable<T_>::operator=(std::move(that));  // checks freshness
	std::swap(values_, that.values_);
	assert_invariant();
	return *this;
}


template< typename T_ >
VariableSet<T_>&
VariableSet<T_>::operator=(const T_& value)
{
#ifndef NDEBUG
	if (Variable<T_>::name_.empty())
		throw_FigException(std::string("can't assign value to a fresh variable")
			.append(" (\"").append(Variable<T_>::name_).append("\")"));
#endif
	for (size_t i=0 ; i < values_.size() ; i++) {
		if (value == values_[i]) {
			Variable<T_>::offset_ = i;
			assert(i < Variable<T_>::range());
			return *this;
		}
	}
	return *this;  // ignore invalid values
}


template< typename T_ >
void
VariableSet<T_>::assign(const T_& value)
{
	if (Variable<T_>::name_.empty())
		throw_FigException(std::string("can't assign value to a fresh variable")
			.append(" (\"").append(Variable<T_>::name_).append("\")"));
	for (size_t i=0 ; i < values_.size() ; i++) {
		if (value == values_[i]) {
			Variable<T_>::offset_ = i;
			assert(i < Variable<T_>::range());
			return;
		}
	}
	throw_FigException(std::string("can't assign ")
		.append(std::to_string(value)).append(" to variable \"")
		.append(Variable<T_>::name_).append("\", invalid value"));
}


template< typename T_ >
bool
VariableSet<T_>::operator==(const Variable<T_>& that) const
{
	auto pthat = dynamic_cast<const VariableSet<T_>*>(&that);
	if (nullptr == pthat)
		return false;
	else
		return (*this) == (*pthat);
}


template< typename T_ >
bool
VariableSet<T_>::operator==(const VariableSet<T_>& that) const
{
	if (  Variable<T_>::name_   != that.name_
	   || Variable<T_>::min_    != that.min_
	   || Variable<T_>::max_    != that.max_
	   || Variable<T_>::ini_    != that.ini_
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
		assert(Variable<T_>::min_ <= e && e <= Variable<T_>::max_);
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
template class VariableSet< float              >;
template class VariableSet< double             >;
template class VariableSet< long double        >;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Devs corner *
 * * * * * * * *
 *
 * We could also instantiate the template copy/move ctors like:
 *
 * template VariableSet<short>::VariableSet(const std::string &thename,
 *                                          const std::set<short>&);
 *
 * However this would have to span over the cross product given by
 * valid_types x valid_containers, and that's just too much.
 *
 * Most sensible workaround seems to be defining these ctors in the header,
 * in spite of code bloating and what have you.
 *
 * References:
 *   http://stackoverflow.com/q/115703
 *   http://stackoverflow.com/a/495056
 *   https://isocpp.org/wiki/faq/templates#templates-defn-vs-decl
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

} // namespace fig

