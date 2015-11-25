//==============================================================================
//
//  Transition.cpp
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
#include <string>
#include <vector>
// FIG
#include <Clock.h>
#include <FigException.h>
#include <Transition.h>


namespace fig
{

template< template< typename, typename... > class Iterator,
		  typename ValueType,
		  typename... OtherIteratorArgs >
void
Transition::handle_clocks(Traial& traial,
						  Iterator<ValueType, OtherIteratorArgs...> fromClock,
						  Iterator<ValueType, OtherIteratorArgs...> toClock,
						  const unsigned& firstClock,
						  const float& timeLapse) const
{
	static_assert( std::is_same< Clock, ValueType >::value,
				   "ERROR: type missmatch. handle_clocks() takes iterators "
				   "pointing to instances of (or pointers to) Clock objects.");
	while (*fromClock != toClock) {

	}
	for (unsigned i = firstClock ; i < firstClock + numClocks ; i++) {
		if (must_reset(i))
			traial.clocks_[i].value = gClocks[i].sample();
		else
			traial.clocks_[i].value -= timeLapse;
#ifndef NTIMECHK
		if (0.0f > traial.clocks_[i].value)
			throw FigException(std::string("negative value for clock \"")
							   .append(gClocks[i].name).append("\""));
#endif
	}
}

} // namespace fig
