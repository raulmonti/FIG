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
#include <set>
#include <list>
#include <deque>
#include <vector>
#include <forward_list>
#include <unordered_set>
// FIG
#include <Traial.h>
#include <ModelSuite.h>
#include <ModuleNetwork.h>
#include <ImportanceFunction.h>

// ADL
using std::begin;
using std::end;


namespace fig
{

Traial::Traial(const size_t& stateSize, const size_t& numClocks) :
	importance(0),
	depth(0),
	lifeTime(0.0),
	state(stateSize),
	orderedIndex_(numClocks)
{
	std::iota(begin(orderedIndex_), end(orderedIndex_), 0u);
	clocks_.reserve(numClocks);
	for (const auto& module_ptr: ModelSuite::get_instance().model->modules)
		for (const auto& clock: module_ptr->clocks())
			clocks_.emplace_back(module_ptr, clock.name(), 0.0f);
}


Traial::Traial(const size_t& stateSize,
			   const size_t& numClocks,
			   Bitflag whichClocks,
			   bool orderTimeouts) :
	importance(0),
	depth(0),
	lifeTime(0.0),
	state(stateSize),
	orderedIndex_(numClocks)
{
	size_t i(0u);
	auto must_reset =
		[&] (const size_t& i) -> bool
		{ return whichClocks & (static_cast<Bitflag>(1u) << i); };
	std::iota(begin(orderedIndex_), end(orderedIndex_), 0u);
	clocks_.reserve(numClocks);
	i = 0;
	for (const auto& module_ptr: ModelSuite::get_instance().model->modules)
		for (const auto& clock: module_ptr->clocks())
			clocks_.emplace_back(module_ptr,
								 clock.name(),
								 must_reset(i++) ? clock.sample() : 0.0f);
	if (orderTimeouts)
		reorder_clocks();
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
Traial::Traial(const size_t& stateSize,
			   const size_t& numClocks,
			   const Container <ValueType, OtherContainerArgs...>& whichClocks,
			   bool orderTimeouts) :
	importance(static_cast<ImportanceValue>(0u)),
	depth(0),
	lifeTime(static_cast<CLOCK_INTERNAL_TYPE>(0.0)),
	state(stateSize),
	orderedIndex_(numClocks)
{
	auto must_reset =
		[&] (const std::string& name) -> bool
		{ return std::find(begin(whichClocks),end(whichClocks),name) != end(whichClocks); };
	static_assert(std::is_convertible< std::string, ValueType >::value,
				  "ERROR: type mismatch. Traial data ctor needs a container "
				  "with clock names");
	std::iota(begin(orderedIndex_), end(orderedIndex_), 0u);
	clocks_.reserve(numClocks);
	for (const auto& module_ptr: ModelSuite::get_instance().model->modules)
		for (const auto& clock: module_ptr->clocks())
			clocks_.emplace_back(module_ptr,
								 clock.name(),
								 must_reset(clock.name()) ? clock.sample() : 0.0f);
	if (orderTimeouts)
		reorder_clocks();
}
// Traial() template ctor can only be invoked with the following containers
template Traial::Traial(const size_t&, const size_t&, const std::set<std::string>&, bool);
template Traial::Traial(const size_t&, const size_t&, const std::list<std::string>&, bool);
template Traial::Traial(const size_t&, const size_t&, const std::deque<std::string>&, bool);
template Traial::Traial(const size_t&, const size_t&, const std::vector<std::string>&, bool);
template Traial::Traial(const size_t&, const size_t&, const std::forward_list<std::string>&, bool);
template Traial::Traial(const size_t&, const size_t&, const std::unordered_set<std::string>&, bool);


Traial::~Traial()
{
//	timeouts_.clear();
//	clocks_.clear();

//	Deleting the vectors would be linear in their size.
//	Since traials should only be deleted after simulations conclusion,
///	@warning we ingnore this (potential?) memory leak due to its short life.
}


std::vector< std::pair< std::string, CLOCK_INTERNAL_TYPE > >
Traial::clocks_values() const
{
	std::vector< std::pair< std::string, CLOCK_INTERNAL_TYPE > >values(clocks_.size());
	for (size_t i = 0ul ; i < values.size() ; i++)
		values[i] = std::make_pair(clocks_[i].name, clocks_[i].value);
	return values;
}


void
Traial::initialize(const ModuleNetwork& network,
				   const ImportanceFunction& impFun)
{
	if (!network.sealed())
#ifndef NDEBUG
		throw_FigException("ModuleNetwork hasn't been sealed yet");
#else
		return;  // we can't do anything without the global data
#endif
	// Initialize variables value
	network.global_state().copy_to_state_instance(state);
    // Initialize clocks (reset all and then resample any initial clock)
    for (auto& timeout : clocks_)
        timeout.value = 0.0f;
	for (const auto& pos_clk_pair: network.initialClocks)
		clocks_[pos_clk_pair.first].value = pos_clk_pair.second.sample();
	// Initialize importance and simulation time
    importance = UNMASK(impFun.importance_of(state));
	depth = 0;
	lifeTime = static_cast<CLOCK_INTERNAL_TYPE>(0.0);
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
