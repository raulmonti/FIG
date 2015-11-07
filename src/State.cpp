//==============================================================================
//
//  State.cpp
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
#include <utility>    // std::move() elements
#include <iterator>   // std::inserter()
#include <algorithm>  // std::move() ranges
// Project code
#include <State.h>
#include <VariableInterval.h>
#include <FigException.h>


namespace fig
{

template< typename T_ >
void
GlobalState<T_>::build_concrete_bound()
{
	maxConcreteState_ = 1u;
	for(const auto pvar: pvars_)
		maxConcreteState_ *= pvar->range_;  // ignore overflow :D
}


template< typename T_ >
GlobalState<T_>::GlobalState(GlobalState<T_>&& that) :
	pvars_(that.size()),
	maxConcreteState_(std::move(that.maxConcreteState_))
{
	std::move(that.pvars_.begin(), that.pvars_.end(), pvars_.begin());
	positionOfVar_.reserve(that.size());
	std::move(that.positionOfVar_.begin(), that.positionOfVar_.end(),
			  std::inserter(positionOfVar_, positionOfVar_.begin()));
	// Clean that up
	that.pvars_.clear();
	that.positionOfVar_.clear();
	that.maxConcreteState_ = 0;
}


template< typename T_ >
std::shared_ptr< const Variable< T_ > >
GlobalState<T_>::operator[](const std::string& varname) const
{
	for (auto pvar: pvars_)
		if (varname == pvar->name_)
			return pvar;
	return nullptr;
}


template< typename T_ >
std::shared_ptr< Variable< T_ > >
GlobalState<T_>::operator[](const std::string& varname)
{
	for (auto pvar: pvars_)
		if (varname == pvar->name_)
			return pvar;
	return nullptr;
}


template< typename T_ >
void
GlobalState<T_>::print_out(std::ostream& out, bool withNewline) const
{
	for (const auto& pvar: pvars_)
		out << pvar->name_ << "=" << pvar->val() << ", ";
	if (!pvars_.empty() && withNewline)
		out << "\b\b  \b\b\n";
	else if (!pvars_.empty())
		out << "\b\b  \b\b";
}


// FIXME: Not needed if there's a unique GlobalState instance
template< typename T_ >
bool
GlobalState<T_>::operator==(const GlobalState< T_ >& that) const
{
	if (this->size() != that.size())
		return false;
	for (size_t i=0 ; i < pvars_.size() ; i++)
		if (*that[i] != *pvars_[i])
			return false;
	return true;
}


template< typename T_ >
bool
GlobalState<T_>::is_valid_state_instance(State s) const
{
	if (s.size() != size())
		return false;
	for (size_t i = 0u ; i < size() ; i++)
		if (!pvars_[i]->is_valid_value(s[i]))
			return false;
	return true;
}


template< typename T_ >
void
GlobalState<T_>::copy_from_state_instance(const State &s, bool checkValidity)
{
	if (s.size() != size())
		throw FigException("attempted to copy values from an invalid state");
	if (checkValidity) {
		for (size_t i = 0u ; i < size() ; i++)
			pvars_[i]->assign(s[i]);
	} else {
		for (size_t i = 0u ; i < size() ; i++)
			(*pvars_[i]) = s[i];
	}
}


template< typename T_ >
void
GlobalState<T_>::copy_to_state_instance(State s) const
{
	if (s.size() != size())
		throw FigException("attempted to copy values to an invalid state");
	for (size_t i = 0u ; i < size() ; i++)
		s[i] = pvars_[i]->val();
}


template< typename T_ >
std::unique_ptr<State>
GlobalState<T_>::to_state_instance() const
{
	std::unique_ptr<State> s(new State(size()));
	for (size_t i = 0u ; i < size() ; i++)
		(*s)[i] = pvars_[i]->val();
	return s;
}


template< typename T_ >
size_t
GlobalState<T_>::encode_state() const
{
	size_t n(0), numVars(size());
	#pragma omp parallel for reduction(+:n) shared(numVars)
	for (size_t i=0 ; i < numVars ; i++) {
		size_t stride(1);
		for (size_t j = i+1 ; j < numVars; j++)
			stride *= pvars_[j]->range_;
		n += pvars_[i]->offset_ * stride;
	}
	return n;
}


template< typename T_ >
void
GlobalState<T_>::decode_state(const size_t& n)
{
	const size_t numVars(size());
	assert(n < maxConcreteState_);
	#pragma omp parallel for shared(n, numVars)
	for (size_t i=0 ; i < numVars ; i++) {
		size_t stride(1u);
		for (size_t j = i+1 ; j < numVars ; j++)
			stride *= pvars_[j]->range_;
		pvars_[i]->offset_ = (n / stride) % pvars_[i]->range_;
	}
}


template< typename T_ >
T_
GlobalState<T_>::decode_state(const size_t& n, const size_t& i) const
{
	size_t numVars(size()), stride(1u);
	assert(i < numVars);
	assert(n < maxConcreteState_);
	#pragma omp parallel for reduction (*:stride) shared(i,numVars)
	for (size_t j = i+1 ; j < numVars ; j++)
		stride *= pvars_[j]->range_;
	return pvars_[i]->val((n / stride) % pvars_[i]->range_);
}


template< typename T_ >
T_
GlobalState<T_>::decode_state(const size_t& n, const std::string& varname) const
{
	size_t varpos(0);
	assert(n < maxConcreteState_);
	for (; varpos < size() ; varpos++)
		if (varname == pvars_[varpos]->name_)
			break;
	assert(varpos < size());
	return decode_state(n, varpos);
}

// GlobalState can only be instantiated with following integral types
template class GlobalState< short              >;
//template class GlobalState< int                >;   // MuParser can't
template class GlobalState< long               >;
template class GlobalState< long long          >;
template class GlobalState< unsigned short     >;
template class GlobalState< unsigned int       >;
template class GlobalState< unsigned long      >;
template class GlobalState< unsigned long long >;

} // namespace fig
