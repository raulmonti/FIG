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
//	along with PRISM; if not, write to the Free Software Foundation,
//	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//==============================================================================


// C++
#include <type_traits>  // std::is_constructible<>, std::is_convertible<>
#include <iterator>     // std::distance()
#include <utility>      // std::move()
// Project code
#include <State.h>

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
template< class Container_ >
GlobalState<T_>::GlobalState(const Container_& container) :
	pvars_(container.size(), nullptr),
	maxConcreteState_(1u)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_constructible<
					  typename VariableInterval<T_>,
					  typename Container_::value_type
				  >::value,
				  "ERROR: GlobalState can only be constructed from Variables, "
				  "VariableDefinitions or VariableDeclarations");
	size_t i(0u);
	for (const auto& e: container)
		pvars_[i++] = std::make_shared< typename VariableInterval< T_ > >(e);
	build_concrete_bound();
}


template< typename T_ >
template< class Container_ >
GlobalState<T_>::GlobalState(Container_&& container) :
	pvars_(container.size(), nullptr),
	maxConcreteState_(1u)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_convertible<
					  typename Container_::value_type,
					  typename std::shared_ptr< VariableInterval<T_> >
				  >::value,
				  "ERROR: GlobalState can only be move-constructed from "
				  "another GlobalState or a container with Variable pointers");
	size_t i(0u);
	for (auto& e: container) {
		pvars_[i++] = std::move(
			dynamic_cast< std::shared_ptr< VariableInterval< T_ > > >(e));
		e = nullptr;
	}
	build_concrete_bound();
	container.clear();
}


template< typename T_ >
template< class Iter_ >
GlobalState<T_>::GlobalState(Iter_ from, Iter_ to) :
	pvars_(std::distance(from,to)),
	maxConcreteState_(1u)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_constructible<
					  typename VariableInterval<T_>,
					  typename Iter_::value_type
				  >::value,
				  "ERROR: GlobalState can only be constructed from Variables, "
				  "VariableDefinitions or VariableDeclarations");
	size_t i(0);
	do {
		pvars_[i++] = std::make_shared< typename VariableInterval<T_> >(*from);
	} while (++from != to);
	build_concrete_bound();
}


template< typename T_ >
GlobalState<T_>::GlobalState(GlobalState<T_>&& that) :
	pvars_(that.size()),
	maxConcreteState_(std::move(that.maxConcreteState_))
{
	size_t i(0u);
	for (auto& ptr: that.pvars_) {
		pvars_[i++] = ptr;
		ptr = nullptr;
	}
	that.pvars_.clear();
	that.maxConcreteState_ = 0;
}


template< typename T_ >
std::shared_ptr< const Variable< T_ > >
GlobalState<T_>::operator[](const std::string& varname) const
{
	for (auto pvar: pvars_)
		if (varname == pvar->name())
			return pvar;
	return nullptr;
}


template< typename T_ >
std::shared_ptr< Variable< T_ > >
GlobalState<T_>::operator[](const std::string& varname)
{
	for (auto pvar: pvars_)
		if (varname == pvar->name())
			return pvar;
	return nullptr;
}


template< typename T_ >
void
GlobalState<T_>::print_out(std::ostream& out, bool withNewline) const
{
	for (const auto& pvar: pvars_)
		out << pvar->name << "=" << pvar->value() << ", ";
	if (!pvars_.empty() && withNewline)
		out << "\b\b  \b\b\n";
	else if (!pvars_.empty())
		out << "\b\b  \b\b";
}


template< typename T_ >
bool
GlobalState<T_>::operator==(const State< T_ >& that) const
{
	if (this->size() != that.size())
		return false;
	for (size_t i=0 ; i < pvars_.size() ; i++)
		if (that[i] != pvars_[i])
			return false;
	return true;
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

} // namespace fig
