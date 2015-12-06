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
#include <iterator>    // std::begin(), std::end()
#include <numeric>     // std::iota()
// FIG
#include <Traial.h>
#include <ModuleNetwork.h>

// ADL
using std::begin;
using std::end;


namespace fig
{

Traial::Traial(const size_t& stateSize, const size_t& numClocks) :
	state(stateSize),
	orderedIndex_(numClocks)
{
	std::iota(begin(orderedIndex_), end(orderedIndex_), 0u);
	clocks_.reserve(numClocks);
	for (const auto& module_ptr: ModuleNetwork::get_instance().modules)
		for (const auto& clock: module_ptr->clocks())
			clocks_.emplace_back(module_ptr, clock.name, 0.0f);
}


Traial::Traial(const size_t& stateSize,
			   const size_t& numClocks,
			   bool initClocks,
			   Bitflag whichClocks,
			   bool orderTimeouts) :
	state(stateSize),
	orderedIndex_(numClocks)
{
	size_t i(0u);
	std::function<bool(const size_t&)> must_reset =
		[&](const size_t& i)
		{ return initClocks && (whichClocks & (static_cast<Bitflag>(1u) << i)); };
	std::iota(begin(orderedIndex_), end(orderedIndex_), 0u);
	clocks_.reserve(numClocks);
	i = 0;
	for (const auto& module_ptr: ModuleNetwork::get_instance().modules)
		for (const auto& clock: module_ptr->clocks())
			clocks_.emplace_back(module_ptr,
								 clock.name,
								 must_reset(i++) ? clock.sample() : 0.0f);
	if (orderTimeouts)
		reorder_clocks();
}


Traial::~Traial()
{
//	timeouts_.clear();
//	clocks_.clear();

//	Deleting the vectors would be linear in their size.
//	Since traials should only be deleted after simulations conclusion,
///	@warning we ingnore this (potential?) memory leak due to its short life.
}


void
Traial::reorder_clocks()
{
	// Sort orderedIndex_ vector according to our current clock values
	std::sort(begin(orderedIndex_), end(orderedIndex_),
		[&](const unsigned& left,
			const unsigned& right)
		{
			return clocks_[left].value < clocks_[right].value;
		}
	);
	// Find first not-null clock, or record '-1' if all are null
	for (unsigned i=0 ; i < clocks_.size() || ((firstNotNull_ = -1) && false) ; i++) {
		if (0.0f < clocks_[orderedIndex_[i]].value) {
			firstNotNull_ = orderedIndex_[i];
			break;
		}
	}
}

} // namespace fig
