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



#include <State.h>

namespace fig
{

template< typename T_ >
void
GlobalState<T_>::build_concrete_bound()
{
	maxConcreteState_ = 1;
	for (size_t i=0 ; i < size() ; i++)
		maxConcreteState_ *= vars_[i].range_;
}

template< typename T_ >
std::shared_ptr< Variable< T_ > >
GlobalState<T_>::operator[](const string& varname)
{
	for (auto& e: *vars_p)
		if (varname == e.name())
			return std::make_shared< Variable< T_ > >(e);
	return nullptr;
}


template< typename T_ >
void
GlobalState<T_>::print_out(std::ostream& out, bool withNewline) const
{
	for (const auto& var: vars_)
		out << var.name << "=" << var.value() << ", ";
	if (!vars_.empty() && withNewline)
		out << "\b\b  \b\b\n";
	else if (!vars_.empty())
		out << "\b\b  \b\b";
}


template< typename T_ >
bool
GlobalState<T_>::operator==(const State< T_ >& that) const
{
	if (this->size() != that.size())
		return false;
	for (size_t i=0 ; i < size() ; i++)
		if (that[i] != vars_[i])
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
			stride *= vars_[j].range_;
		n += vars_[i].offset_ * stride;
	}
	return n;
}

} // namespace fig
