//==============================================================================
//
//  ModuleNetwork.cpp
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
#include <iterator>   // std::begin(), std::end()
#include <algorithm>  // std::find_if()
#include <set>
#include <list>
#include <deque>
#include <vector>
#include <forward_list>
#include <unordered_set>
// FIG
#include <ModuleNetwork.h>
#include <TraialPool.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

// ADL
using std::find_if;
using std::begin;
using std::end;


namespace fig
{

// Static variables initialization

std::unique_ptr< ModuleNetwork > ModuleNetwork::instance_ = nullptr;

std::once_flag ModuleNetwork::singleInstance_;


// ModuleNetwork class member functions

ModuleNetwork::~ModuleNetwork()
{
//	modules.clear();

//	Deleting this vector would be linear in its size.
//	Since the ModuleNetwork should only be deleted after simulations conclusion,
///	@warning we ignore this (potential?) memory leak due to its short life.
}


void
ModuleNetwork::add_module(ModuleInstance** module)
{
	modules.emplace_back(*module);
	auto state = (*module)->mark_added(modules.size()-1, lastClockIndex_);
	gState.append(state);
	lastClockIndex_ += (*module)->clocks().size();
	*module = nullptr;
}


void
ModuleNetwork::add_module(std::shared_ptr< ModuleInstance >& module)
{
	modules.push_back(module);
	auto state = module->mark_added(modules.size()-1, lastClockIndex_);
	gState.append(state);
	lastClockIndex_ += module->clocks().size();
	module = nullptr;
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
void
ModuleNetwork::seal(const Container<ValueType, OtherContainerArgs...>& initialClocksNames)
{
	size_t numClocksReviewed(0u);
	static_assert(std::is_convertible< std::string, ValueType >::value,
				  "ERROR: type missmatch. ModuleNetwork::seal() needs "
				  "a container with the initial clock names");
	if (sealed_)
#ifndef NDEBUG
		throw FigException("the ModuleNetwork has already been sealed");
#else
		return;
#endif
	sealed_ = true;
	initialClocks.reserve(std::distance(begin(initialClocksNames),
										end(initialClocksNames)));
	// For each module in the network...
	for (auto& module_ptr: modules) {
		// ... seal it ...
		module_ptr->seal(&State<STATE_INTERNAL_TYPE>::position_of_var, gState);
		// ... search for initial clocks within ...
		auto module_clocks = module_ptr->clocks();
		for (const auto& initClkName: initialClocksNames) {
			auto clkIter = find_if(begin(module_clocks),
								   end(module_clocks),
								   [&] (const Clock& clk) -> bool
								   { return clk.name() == initClkName; });
			// ... and if found, register position and distribution
			if (clkIter != end(module_clocks)) {
				size_t clockLocalPos = std::distance(begin(module_clocks), clkIter);
				initialClocks.emplace(numClocksReviewed + clockLocalPos, *clkIter);
			}
		}
		numClocksReviewed += module_ptr->num_clocks();
	}
	// Fill other global info
	TraialPool::numVariables = gState.size();
	TraialPool::numClocks = numClocksReviewed;
}

// ModuleNetwork::seal() can only be be invoked with the following containers
template void ModuleNetwork::seal(const std::set<std::string>&);
template void ModuleNetwork::seal(const std::list<std::string>&);
template void ModuleNetwork::seal(const std::deque<std::string>&);
template void ModuleNetwork::seal(const std::vector<std::string>&);
template void ModuleNetwork::seal(const std::forward_list<std::string>&);
template void ModuleNetwork::seal(const std::unordered_set<std::string>&);

} // namespace fig
