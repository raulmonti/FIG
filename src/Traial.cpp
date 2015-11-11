//==============================================================================
//
//  Traial.cpp
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
#include <algorithm>  // std::sort()
// FIG
#include <Traial.h>


namespace fig
{

void
Traial::reorder_clocks()
{
	// Sort timeouts_ vector according to registered clock values
	std::sort(timeouts_.begin(), timeouts_.end(),
		[](const Timeout* first,
		   const Timeout* second)
		{
			assert(nullptr != first && nullptr != second);
			return first->value < second->value;
		});

	// Find first not-null clock, or record '-1' if all are null
	for (int i=0 ; i < clocks_.size() || ((firstNotNull_ = -1) && false) ; i++) {
		assert(nullptr != timeouts_[i]);
		if (timeouts_[i]->value > 0.0f) {
			firstNotNull_ = i;
			break;
		}
	}
}

} // namespace fig
