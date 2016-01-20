//==============================================================================
//
//  TraialPool.cpp
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
#include <set>
#include <list>
#include <deque>
#include <vector>
#include <forward_list>
#include <unordered_set>
#include <iterator>    // std::begin(), std::end()
#include <functional>  // std::ref()
// FIG
#include <TraialPool.h>

// ADL
using std::begin;
using std::end;


namespace fig
{

// Static variables initialization

std::unique_ptr< TraialPool > TraialPool::instance_ = nullptr;

std::once_flag TraialPool::singleInstance_;

std::vector< Traial > TraialPool::traials_;

std::forward_list< Reference< Traial > > TraialPool::available_traials_;

const size_t TraialPool::initialSize = (1u) << 12;  // 4K

const size_t TraialPool::sizeChunkIncrement = TraialPool::initialSize >> 3;  // initialSize/8

size_t TraialPool::numVariables = 0u;

size_t TraialPool::numClocks = 0u;


// TraialPool class member functions

TraialPool::TraialPool()
{
	ensure_resources(initialSize);
}


TraialPool&
TraialPool::get_instance()
{
	if (0u >= numVariables && 0u >= numClocks)
#ifndef NDEBUG
		throw_FigException("can't build the TraialPool since the "
						   "ModelSuite hasn't been sealed yet");
#endif
	std::call_once(singleInstance_,
				   [] () {instance_.reset(new TraialPool);});
	return *instance_;
}


TraialPool::~TraialPool()
{
//	available_traials_.clear();
//	traials_.clear();

//	Deleting these containers would be linear in their sizes.
//	Since the TraialPool should only be deleted after simulations conclusion,
///	@warning we ingnore this (potential?) memory leak due to its short life.
}


Traial&
TraialPool::get_traial()
{
	if (available_traials_.empty())
		ensure_resources(sizeChunkIncrement);  // Need to create more Traials
	Traial& traial(available_traials_.front());
	available_traials_.pop_front();
	return traial;
}


void
TraialPool::return_traial(Traial&& traial)
{
	available_traials_.push_front(traial);
}


std::forward_list< Reference< Traial > >
TraialPool::get_traial_copies(const Traial& traial, unsigned numCopies)
{
	std::forward_list< Reference< Traial > > result;
	assert(sizeChunkIncrement > numCopies);  // wouldn't make sense otherwise
	ensure_resources(numCopies);
	for (unsigned i = 0u ; i < numCopies ; i++) {
		result.push_front(available_traials_.front());
		available_traials_.pop_front();
		static_cast<Traial&>(result.front()) = traial;  // copy 'traial' values
	}
	return result;
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
void
TraialPool::return_traials(Container<ValueType, OtherContainerArgs...>& traials)
{
	static_assert(std::is_same< Reference<Traial>, ValueType >::value,
				  "ERROR: type mismatch. Only Traial references can be "
				  "returned to the TraialPool.");
	for (Traial& t: traials)
		available_traials_.push_front(t);
	traials.clear();  // keep user from tampering with those references
}

// TraialPool::return_traials() can only be invoked with the following containers
template void TraialPool::return_traials(std::set< Reference<Traial> >&);
template void TraialPool::return_traials(std::list< Reference<Traial> >&);
template void TraialPool::return_traials(std::deque< Reference<Traial> >&);
template void TraialPool::return_traials(std::vector< Reference<Traial> >&);

// Specialization for forward_list, up to 2x faster
template <>
void
TraialPool::return_traials(std::forward_list< Reference< Traial > >& list)
{
	for (auto it = list.begin() ; it != list.end() ; it = list.begin()) {
		available_traials_.push_front(std::move(*it));
		list.pop_front();  // 'it' got invalidated
	}
}


void
TraialPool::ensure_resources(const size_t& requiredResources)
{
	const size_t oldSize = traials_.size();
	const size_t newSize = oldSize + std::max<long>(0, requiredResources - num_resources());
	traials_.reserve(newSize);
	for(size_t i = oldSize ; i < newSize ; i++) {
		traials_.emplace_back(numVariables, numClocks);
		available_traials_.emplace_front(std::ref(traials_[i]));
	}
}


size_t
TraialPool::num_resources() const noexcept
{
	return std::distance(begin(available_traials_), end(available_traials_));
}

} // namespace fig
