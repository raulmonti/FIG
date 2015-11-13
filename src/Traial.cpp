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
#include <algorithm>   // std::sort()
#include <functional>  // std::function
// FIG
#include <Traial.h>


namespace fig
{

Traial::Traial() :
	state(gState.size()),
	timeouts_(gClocks.size(), nullptr)
{
	clocks_.reserve(gClocks.size());
	size_t i(0u);
	for (const auto& clk: gClocks) {
		clocks_.emplace_back(clk.module, clk.name, 0.0f);
		timeouts_[i] = &clocks_[i];
		i++;
	}
}


Traial::Traial(bool initState, bool initClocks, Bitflag whichClocks) :
	state(gState.size()),
	timeouts_(gClocks.size(), nullptr)
{
	size_t i(0u);
	std::function<bool(const size_t&)>
	must_reset = [&initClocks, &whichClocks](const size_t& i) {
		return initClocks &&
				(whichClocks & (static_cast<Bitflag>(1u) << i));
	};
	if (initState) {
		i = 0;
		for (auto& varp: gState)
			state[i++] = varp->ini();
	}
	i = 0;
	clocks_.reserve(gClocks.size());
	for (const auto& clk: gClocks) {
		clocks_.emplace_back(clk.module,
							 clk.name,
							 must_reset(i) ? clk.sample() : 0.0f);
		timeouts_[i] = &clocks_[i];
		i++;
	}
}


Traial::~Traial()
{
//	timeouts_.clear();
//	clocks_.clear();

//	Deleting the vectors would be linear in their size.
//	Since traials should only be deleted after simulations conclusion,
///	@warning we ingnore this memory leak due to its short life.
}


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
	for (unsigned i=0 ; i < clocks_.size() || ((firstNotNull_ = -1) && false) ; i++) {
		assert(nullptr != timeouts_[i]);
		if (0.0f < timeouts_[i]->value) {
			firstNotNull_ = i;
			break;
		}
	}
}

} // namespace fig
