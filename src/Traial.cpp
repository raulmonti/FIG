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
#include <sstream>
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
    level(static_cast<ImportanceValue>(0u)),
    depth(0),
	numLevelsCrossed(0),
	lifeTime(0.0),
	state(stateSize),
	orderedIndex_(numClocks),
	nextClock_(-1)
{
	std::iota(begin(orderedIndex_), end(orderedIndex_), 0u);
	clocks_.reserve(numClocks);
	for (const auto& module_ptr: ModelSuite::get_instance().model->modules) {
		int clkPos = module_ptr->first_clock_gpos();
		assert(0 <= clkPos);
		for (const auto& clock: module_ptr->clocks())
			// following assumes we're iterating a vector
			// and thus the access to the clocks is sequentially ordered
			clocks_.emplace_back(module_ptr, clock.name(), 0.0f, clkPos++);
	}
}


Traial::Traial(const size_t& stateSize,
			   const size_t& numClocks,
			   Bitflag whichClocks,
			   bool orderTimeouts) :
    level(static_cast<ImportanceValue>(0u)),
    depth(0),
	numLevelsCrossed(0),
	lifeTime(0.0),
	state(stateSize),
	orderedIndex_(numClocks),
	nextClock_(-1)
{
	std::iota(begin(orderedIndex_), end(orderedIndex_), 0u);
	clocks_.reserve(numClocks);
	for (const auto& module_ptr: ModelSuite::get_instance().model->modules) {
		int clkPos = module_ptr->first_clock_gpos();
		assert(0 <= clkPos);
		for (const auto& clock: module_ptr->clocks()) {
			// following assumes we're iterating a vector
			// and thus the access to the clocks is sequentially ordered
			clocks_.emplace_back(module_ptr,
								 clock.name(),
								 whichClocks[clkPos] ? clock.sample() : 0.0f,
								 clkPos);
			clkPos++;
		}
	}
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
    level(static_cast<ImportanceValue>(0u)),
	depth(0),
	numLevelsCrossed(0),
	lifeTime(static_cast<CLOCK_INTERNAL_TYPE>(0.0)),
	state(stateSize),
	orderedIndex_(numClocks),
	nextClock_(-1)
{
	auto must_reset =
		[&] (const std::string& name) -> bool
		{ return std::find(begin(whichClocks),end(whichClocks),name) != end(whichClocks); };
	static_assert(std::is_convertible< std::string, ValueType >::value,
				  "ERROR: type mismatch. Traial data ctor needs a container "
				  "with clock names");
	std::iota(begin(orderedIndex_), end(orderedIndex_), 0u);
	clocks_.reserve(numClocks);
	for (const auto& module_ptr: ModelSuite::get_instance().model->modules) {
		int firstClock = module_ptr->first_clock_gpos();
		assert(0 <= firstClock);
		for (const auto& clock: module_ptr->clocks())
			// following assumes we're iterating a vector
			// and thus the access to the clocks is sequentially ordered
			clocks_.emplace_back(module_ptr,
								 clock.name(),
								 must_reset(clock.name()) ? clock.sample() : 0.0f,
								 firstClock++);
	}
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
Traial::clocks_values(bool ordered) const
{
	std::vector< std::pair< std::string, CLOCK_INTERNAL_TYPE > >values(clocks_.size());
	if (ordered)
		for (size_t i = 0ul ; i < values.size() ; i++)
			values[i] = std::make_pair(clocks_[orderedIndex_[i]].name,
									   clocks_[orderedIndex_[i]].value);
	else
		for (size_t i = 0ul ; i < values.size() ; i++)
			values[i] = std::make_pair(clocks_[i].name,
									   clocks_[i].value);
	return values;
}


void
Traial::initialize(const ModuleNetwork& network,
				   const ImportanceFunction& impFun)
{
#ifndef NDEBUG
	if (!network.sealed())
		throw_FigException("ModuleNetwork hasn't been sealed yet");
	else if (!impFun.has_importance_info())
		throw_FigException(std::string("importance function \"")
						  .append(impFun.name()).append("\" doesn't have ")
						  .append("importance info; can't initialize Traial"));
#else
	if (! (network.sealed() && impFun.has_importance_info()) )
		return;  // we can't do anything without that data
#endif
	// Initialize variables value
	network.initial_state().copy_to_state_instance(state);
	// Initialize clocks (reset all and then resample initials)
	for (auto& timeout : clocks_)
		timeout.value = 0.0f;
	for (const auto& posCLK: network.initialClocks)
		clocks_[posCLK.first].value = posCLK.second.sample();  // should be non-negative
	// Initialize importance and simulation time
	level = impFun.ready() ? impFun.level_of(state)
						   : impFun.importance_of(state);
	depth = -static_cast<short>(level);
	numLevelsCrossed = 0;
	lifeTime = static_cast<CLOCK_INTERNAL_TYPE>(0.0);
}


void
Traial::reorder_clocks()
{
	// Sort orderedIndex_ vector according to our current clock values
	std::sort(begin(orderedIndex_), end(orderedIndex_),
		[&](const unsigned& left, const unsigned& right)
		{ return clocks_[left].value < clocks_[right].value; }
	);
	// Find next clock to check, or record '-1' if all are negative
	for (unsigned i=0u ; i < clocks_.size() || ((nextClock_ = -1) && false) ; i++) {
		if (std::isfinite(clocks_[orderedIndex_[i]].value) &&
				0.0f <= clocks_[orderedIndex_[i]].value) {
			nextClock_ = orderedIndex_[i];
			break;
		}
	}
}


void
Traial::report_deadlock()
{
	std::stringstream errMsg;
	errMsg << "all clocks are null, deadlock? State is (";
	for (const auto& v: state)
		errMsg << v << ",";
	errMsg << "\b)[";
	for (const auto& t: clocks_)
		if (std::isfinite(t.value))
			errMsg << t.name << ":" << t.value << "|";
	errMsg << "\b] -- Omitted clocks have nan/inf value.";
	throw_FigException(errMsg.str());
}

} // namespace fig
