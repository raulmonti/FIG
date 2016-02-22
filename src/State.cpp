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
#include <sstream>
// FIG
#include <State.h>
#include <FigException.h>

// ADL
using std::swap;
using std::move;
using std::copy;
using std::begin;
using std::end;


namespace fig
{

template< typename T_ >
State<T_>::State(const State<T_>& that) :
	pvars_(that.pvars_.size()),
	maxConcreteState_(that.maxConcreteState_)
{
	// Here lies the depth in this copy
	for (size_t i = 0u ; i < that.pvars_.size() ; i++) {
		// We chose VariableInterval<> as implementation for our Variables
		auto var_ptr(std::dynamic_pointer_cast< VariableInterval<T_> >(that.pvars_[i]));
		pvars_[i] = std::make_shared< VariableInterval< T_ > >(*var_ptr);
	}
	positionOfVar_.reserve(that.size());
	copy(::begin(that.positionOfVar_), ::end(that.positionOfVar_),
		 std::inserter(positionOfVar_, ::begin(positionOfVar_)));
}


template< typename T_ >
State<T_>::State(State<T_>&& that) :
	pvars_(move(that.pvars_)),
	maxConcreteState_(move(that.maxConcreteState_))
{
	positionOfVar_.reserve(that.size());
	std::move(::begin(that.positionOfVar_), ::end(that.positionOfVar_),
			  std::inserter(positionOfVar_, ::begin(positionOfVar_)));
	// Clean that up
	that.pvars_.clear();
	that.positionOfVar_.clear();
	that.maxConcreteState_ = 0;
}


template< typename T_ >
State<T_>& State<T_>::operator=(State<T_> that)
{
	swap(pvars_, that.pvars_);
	swap(maxConcreteState_, that.maxConcreteState_);
	swap(positionOfVar_, that.positionOfVar_);
//  TODO erase below or erase above?
//	positionOfVar_.clear();
//	positionOfVar_.reserve(pvars_.size());
//	std::move(::begin(that.positionOfVar_), ::end(that.positionOfVar_),
//			  std::inserter(positionOfVar_, ::begin(positionOfVar_)));
	return *this;
}


template< typename T_ >
void
State<T_>::append(const State& tail)
{
#ifndef NDEBUG
	for (const auto var_ptr: tail)
		if (is_our_var(var_ptr->name()))
			throw_FigException(std::string("variable \"")
							   .append(var_ptr->name())
							   .append("\" already exists in this state"));
#endif
	size_t oldSize(pvars_.size());
	pvars_.insert(::end(pvars_), ::begin(tail), ::end(tail));
	positionOfVar_.insert(::begin(tail.positionOfVar_), ::end(tail.positionOfVar_));
	// Compute new variables global positions,
	// taking into account the offset from the ones we already have
	for (const auto& pair: tail.positionOfVar_)
		positionOfVar_[pair.first] += oldSize;
	build_concrete_bound();
}


template< typename T_ >
void
State<T_>::shallow_copy(State<T_>& that)
{
	pvars_ = that.pvars_;  // here lies the shallowness
	maxConcreteState_ = that.maxConcreteState_;
	positionOfVar_.reserve(that.size());
	copy(::begin(that.positionOfVar_), ::end(that.positionOfVar_),
		 std::inserter(positionOfVar_, ::begin(positionOfVar_)));
}


template< typename T_ >
std::shared_ptr< const Variable< T_ > >
State<T_>::operator[](const std::string& varname) const
{
	for (auto pvar: pvars_)
		if (varname == pvar->name_)
			return pvar;
	return nullptr;
}


template< typename T_ >
std::shared_ptr< Variable< T_ > >
State<T_>::operator[](const std::string& varname)
{
	for (auto pvar: pvars_)
		if (varname == pvar->name_)
			return pvar;
	return nullptr;
}


template< typename T_ >
size_t
State<T_>::position_of_var(const std::string& varname) const
{
#ifndef NRANGECHK
    try {
        return positionOfVar_.at(varname);
    } catch (std::out_of_range&) {
        std::stringstream ss;
        ss << "fig::State::position_of_var(): variable not found in state (\"";
        ss << varname << "\")";
        throw std::out_of_range(ss.str());
    }
#else
    return positionOfVar_[varname];  // creates location if inexistent!
#endif
}


template< typename T_ >
void
State<T_>::print_out(std::ostream& out, bool condensed) const
{
	if (condensed) {
		for (const auto& pvar: pvars_)
			out << pvar->name_ << "=" << pvar->val() << ", ";
		if (!pvars_.empty())
			out << "\b\b  \b\b" << std::endl;
	} else {
		for (const auto& pvar: pvars_)
			out << pvar->name_ << " = "
				<< pvar->val() << " : ("
				<< pvar->min() << ", "
				<< pvar->max() << ", "
				<< pvar->ini() << ")"
				<< std::endl;
	}
}


template< typename T_ >
bool
State<T_>::operator==(const State< T_ >& that) const
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
State<T_>::is_valid_state_instance(StateInstance s) const
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
State<T_>::copy_from_state_instance(const StateInstance &s, bool checkValidity)
{
	if (s.size() != size()) {
#ifndef NDEBUG
		std::cerr << "State of size " << size() << " attempted to copy from "
				  << "StateInstance with " << s.size() << " variables.\n";
#endif
		throw_FigException("attempted to copy values from an invalid state");
	}
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
State<T_>::copy_to_state_instance(StateInstance& s) const
{
	if (s.size() != size())
		throw_FigException("attempted to copy values to an incompatible state");
	for (size_t i = 0u ; i < size() ; i++)
		s[i] = pvars_[i]->val();
}


template< typename T_ >
StateInstance State<T_>::to_state_instance() const
{
	StateInstance s(size());
	for (size_t i = 0u ; i < size() ; i++)
		s[i] = pvars_[i]->val();
	return s;
}


template< typename T_ >
size_t
State<T_>::encode() const
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
const State<T_>&
State<T_>::decode(const size_t& n)
{
	const size_t numVars(size());
	assert(n < maxConcreteState_);
	#pragma omp parallel for default(shared)
	for (size_t i=0 ; i < numVars ; i++) {
		size_t stride(1u);
		for (size_t j = i+1 ; j < numVars ; j++)
			stride *= pvars_[j]->range_;
		pvars_[i]->offset_ = (n / stride) % pvars_[i]->range_;
	}
	return *this;
}


template< typename T_ >
T_
State<T_>::decode(const size_t& n, const size_t& i) const
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
State<T_>::decode(const size_t& n, const std::string& varname) const
{
	size_t varpos(0);
	assert(n < maxConcreteState_);
	for (; varpos < size() ; varpos++)
		if (varname == pvars_[varpos]->name_)
			break;
	assert(varpos < size());
	return decode(n, varpos);
}

// State can only be instantiated with following integral types
template class State< short              >;
//template class State< int                >;   // MuParser can't
template class State< long               >;
template class State< long long          >;
template class State< unsigned short     >;
template class State< unsigned int       >;
template class State< unsigned long      >;
template class State< unsigned long long >;


template< typename T_ >
void
State<T_>::build_concrete_bound()
{
	maxConcreteState_ = 1u;
	for(const auto pvar: pvars_)
		maxConcreteState_ *= pvar->range_;  // ignore overflow :D
}


template< typename T_ >
bool
State<T_>::is_our_var(const std::string& varName)
{
	auto varFound = std::find_if(
						::begin(pvars_), ::end(pvars_),
						[&] (const std::shared_ptr<Variable<T_>>& var_ptr)
						{ return varName == var_ptr->name(); });
	return ::end(pvars_) != varFound;
}

} // namespace fig
