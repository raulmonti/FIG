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
#include <stack>
#include <vector>
#include <forward_list>
#include <unordered_set>
#include <iterator>    // std::begin(), std::end()
#include <functional>  // std::ref()
#include <type_traits>
// FIG
#include <TraialPool.h>
#include <FigLog.h>

// ADL
using std::begin;
using std::end;


namespace   // // // // // // // // // // // // // // // // // // // // // // //
{

/// We use delicate move-semantics; keep track of the actual memory location
static fig::Traial* TRAIALS_MEM_ADDR(nullptr);

/// Check whether the TraialPool was requested a positive number of Traials
/// @return Whether \p numTraials > 0u
/// @note Print warning if DEBUG mode is on and numTraials == 0u
bool
positive_num_traials(const unsigned& numTraials)
{
	if (0u == numTraials) {
#ifndef NDEBUG
		fig::figTechLog << "[WARNING] TraialPool invoked to get 0 Traials\n";
#endif
		return false;
	}
	return true;
}

} // namespace   // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

using TraialRef = Reference< Traial >;

// Static variables initialization

std::unique_ptr< TraialPool > TraialPool::instance_ = nullptr;

std::once_flag TraialPool::singleInstance_;

std::vector< Traial > TraialPool::traials_;

std::forward_list< Reference< Traial > > TraialPool::available_traials_;

size_t TraialPool::numVariables = 0u;

size_t TraialPool::numClocks = 0u;


// TraialPool class member functions

TraialPool::TraialPool()
{
	ensure_resources(initial_size());
}


TraialPool&
TraialPool::get_instance()
{
#ifndef NDEBUG
	if (0u >= numVariables && 0u >= numClocks)
		throw_FigException("can't build the TraialPool since the "
						   "ModelSuite hasn't been sealed yet");
#endif
	std::call_once(singleInstance_,
	               [] () {instance_.reset(new TraialPool);});
	assert(nullptr != instance_);
	return *instance_;
}


TraialPool::~TraialPool()
{
	// clear();  // NO!
	// Deleting these containers has produced double free() errors.
	// Since the TraialPool should only be deleted after simulations conclusion,
	/// @warning we ingnore this potential memory leak due to its short life.
}


Traial&
TraialPool::get_traial()
{
	if (available_traials_.empty())
		ensure_resources(increment_size());  // Need to create more Traials
	Traial& traial(available_traials_.front());
	available_traials_.pop_front();
	return traial;
}


void
TraialPool::return_traial(Traial&& traial)
{
	available_traials_.emplace_front(traial);
}


void
TraialPool::return_traial(TraialRef traial)
{
	available_traials_.push_front(traial);
}


template< template< typename... > class Container,
		  typename... OtherArgs >
void
TraialPool::get_traials(Container<TraialRef, OtherArgs...>& cont,
						unsigned numTraials)
{
	if (!positive_num_traials(numTraials))
		return;
	numTraials++;  // loop guard condition requires this increment
retrieve_traials:
	while (!available_traials_.empty() && 0u < --numTraials) {
		cont.emplace(end(cont), available_traials_.front());
		available_traials_.pop_front();
	}
	if (0u < numTraials) {
		ensure_resources(std::max(numTraials+1ul, increment_size()));
		goto retrieve_traials;
	}
}
// TraialPool::get_traials() generic version can only be invoked with the following containers
template void TraialPool::get_traials(std::list< TraialRef >&, unsigned);
template void TraialPool::get_traials(std::deque< TraialRef >&, unsigned);
template void TraialPool::get_traials(std::vector< TraialRef >&, unsigned);

// TraialPool::get_traials() specialization for STL std::stack<>
template<> void
TraialPool::get_traials(std::stack< TraialRef >& stack,
						unsigned numTraials)
{
	if (!positive_num_traials(numTraials))
		return;
	numTraials++;  // loop guard condition requires this increment
	retrieve_traials:
	while (!available_traials_.empty() && 0u < --numTraials) {
		stack.emplace(available_traials_.front());
		available_traials_.pop_front();
	}
	if (0u < numTraials) {
		ensure_resources(std::max<size_t>(numTraials++, increment_size()));
		goto retrieve_traials;
	}
}

// TraialPool::get_traials() specialization for STL std::forward_list<>
template<> void
TraialPool::get_traials(std::forward_list< TraialRef >& flist,
						unsigned numTraials)
{
	if (!positive_num_traials(numTraials))
		return;
	numTraials++;  // loop guard condition requires this increment
	retrieve_traials:
	while (!available_traials_.empty() && 0u < --numTraials) {
		flist.push_front(available_traials_.front());
		available_traials_.pop_front();
	}
	if (0u < numTraials) {
		ensure_resources(std::max<size_t>(numTraials++, increment_size()));
		goto retrieve_traials;
	}
}


template< typename Container >
void
TraialPool::get_traial_copies(Container& cont,
                              const Traial& traial,
                              unsigned numCopies,
                              short depth)
{
	static_assert(std::is_same<typename Container::value_type,Reference<Traial>>::value,
	              "ERROR: type mismatch, expected Reference<Traial> value_type in container");
	assert(0 >= depth);  // we're typically called on a threshold-level-up
	if (!positive_num_traials(numCopies))
		return;
	numCopies++;  // loop guard condition requires this increment
    retrieve_traials:
	while (!available_traials_.empty() && 0u < --numCopies) {
		Traial& t = available_traials_.front();
		available_traials_.pop_front();
		t.copyResampling(traial);  // copy, but resample clock times
		t.depth = depth;
		t.nextSplitLevel = static_cast<decltype(t.nextSplitLevel)>(traial.level) + 1;
		cont.emplace(end(cont), std::ref(t));  // copy elision
	}
	if (0u < numCopies) {
		ensure_resources(std::max<size_t>(numCopies++, increment_size()));
		goto retrieve_traials;
	}
}
// TraialPool::get_traial_copies() generic version can only be invoked with the following containers
template void TraialPool::get_traial_copies(std::list< TraialRef >&, const Traial&, unsigned, short);
template void TraialPool::get_traial_copies(std::deque< TraialRef >&, const Traial&, unsigned, short);
template void TraialPool::get_traial_copies(std::vector< TraialRef >&, const Traial&, unsigned, short);
//template void TraialPool::get_traial_copies(std::forward_list< TraialRef >&, const Traial&, unsigned, short);
//
// TraialPool::get_traial_copies() specialization for STL std::stack<>
template<> void
TraialPool::get_traial_copies(std::stack< TraialRef >& stack,
                              const Traial& traial,
                              unsigned numCopies,
                              short depth)
{
	if (!positive_num_traials(numCopies))
		return;
	assert(0 >= depth);  // we're typically called on a threshold-level-up
	numCopies++;  // loop guard condition requires this increment
    retrieve_traials:
	while (!available_traials_.empty() && 0u < --numCopies) {
		Traial& t = available_traials_.front();
		available_traials_.pop_front();
		t.copyResampling(traial);  // copy, but resample clock times
		t.depth = depth;
		t.nextSplitLevel = 1 + static_cast<decltype(t.nextSplitLevel)>(traial.level);
		stack.push(std::ref(t));  // copy elision
	}
	if (0u < numCopies) {
		ensure_resources(std::max<size_t>(numCopies++, increment_size()));
		goto retrieve_traials;
	}
}
//
// TraialPool::get_traial_copies() specialization for STL std::stack<vector>
template<> void
TraialPool::get_traial_copies(std::stack< TraialRef,std::vector<TraialRef> >& stack,
                              const Traial& traial,
                              unsigned numCopies,
                              short depth)
{
	if (!positive_num_traials(numCopies))
		return;
	assert(0 >= depth);  // we're typically called on a threshold-level-up
	numCopies++;  // loop guard condition requires this increment
    retrieve_traials:
	while (!available_traials_.empty() && 0u < --numCopies) {
		Traial& t = available_traials_.front();
		available_traials_.pop_front();
		t.copyResampling(traial);  // copy, but resample clock times
		t.depth = depth;
		t.nextSplitLevel = 1 + static_cast<decltype(t.nextSplitLevel)>(traial.level);
		stack.push(std::ref(t));  // copy elision
	}
	if (0u < numCopies) {
		ensure_resources(std::max<size_t>(numCopies++, increment_size()));
		goto retrieve_traials;
	}
}


template< typename Container >
void
TraialPool::get_traial_clones(Container& cont,
                              const Traial& traial,
                              unsigned numClones,
                              short depth)
{
	static_assert(std::is_same<typename Container::value_type,Reference<Traial>>::value,
	              "ERROR: type mismatch, expected Reference<Traial> value_type in container");
	if (!positive_num_traials(numClones))
		return;
	assert(0 >= depth);  // we're typically called on a threshold-level-up
	numClones++;  // loop guard condition requires this increment
	retrieve_traials:
	while (!available_traials_.empty() && 0u < --numClones) {
		Traial& t = available_traials_.front();
		available_traials_.pop_front();
		t = traial;  // copy without resampling times
		t.depth = depth;
		t.nextSplitLevel = static_cast<decltype(t.nextSplitLevel)>(traial.level) + 1;
		cont.emplace(end(cont), std::ref(t));  // copy elision
	}
	if (0u < numClones) {
		ensure_resources(std::max<size_t>(numClones++, increment_size()));
		goto retrieve_traials;
	}
}
// TraialPool::get_traial_clones() generic version can only be invoked with the following containers
template void TraialPool::get_traial_clones(std::list< TraialRef >&, const Traial&, unsigned, short);
template void TraialPool::get_traial_clones(std::deque< TraialRef >&, const Traial&, unsigned, short);
template void TraialPool::get_traial_clones(std::vector< TraialRef >&, const Traial&, unsigned, short);
//
// TraialPool::get_traial_clones() specialization for STL std::stack<>
template<> void
TraialPool::get_traial_clones(std::stack< TraialRef >& stack,
                              const Traial& traial,
                              unsigned numClones,
                              short depth)
{
	if (!positive_num_traials(numClones))
		return;
	assert(0 >= depth);  // we're typically called on a threshold-level-up
	numClones++;  // loop guard condition requires this increment
	retrieve_traials:
	while (!available_traials_.empty() && 0u < --numClones) {
		Traial& t = available_traials_.front();
		available_traials_.pop_front();
		t = traial;  // copy without resampling times
		t.depth = depth;
		t.nextSplitLevel = 1 + static_cast<decltype(t.nextSplitLevel)>(traial.level);
		stack.push(std::ref(t));  // copy elision
	}
	if (0u < numClones) {
		ensure_resources(std::max<size_t>(numClones++, increment_size()));
		goto retrieve_traials;
	}
}
//
// TraialPool::get_traial_clones() specialization for STL std::stack<>
template<> void
TraialPool::get_traial_clones(std::stack< TraialRef,std::vector<TraialRef> >& stack,
                              const Traial& traial,
                              unsigned numClones,
                              short depth)
{
	if (!positive_num_traials(numClones))
		return;
	assert(0 >= depth);  // we're typically called on a threshold-level-up
	numClones++;  // loop guard condition requires this increment
	retrieve_traials:
	while (!available_traials_.empty() && 0u < --numClones) {
		Traial& t = available_traials_.front();
		available_traials_.pop_front();
		t = traial;  // copy without resampling times
		t.depth = depth;
		t.nextSplitLevel = 1 + static_cast<decltype(t.nextSplitLevel)>(traial.level);
		stack.push(std::ref(t));  // copy elision
	}
	if (0u < numClones) {
		ensure_resources(std::max<size_t>(numClones++, increment_size()));
		goto retrieve_traials;
	}
}
//
// TraialPool::get_traial_clones() specialization for STL std::forward_list<>
template<> void
TraialPool::get_traial_clones(std::forward_list< TraialRef >& flist,
                              const Traial& traial,
                              unsigned numClones,
                              short depth)
{
	if (!positive_num_traials(numClones))
		return;
	assert(0 >= depth);  // we're typically called on a threshold-level-up
	numClones++;  // loop guard condition requires this increment
	retrieve_traials:
	while (!available_traials_.empty() && 0u < --numClones) {
		Traial& t = available_traials_.front();
		available_traials_.pop_front();
		t = traial;
		t.depth = depth;
		t.nextSplitLevel = 1 + static_cast<decltype(t.nextSplitLevel)>(traial.level);
		flist.push_front(std::ref(t));  // copy elision
    }
	if (0u < numClones) {
		ensure_resources(std::max<size_t>(numClones++, increment_size()));
		goto retrieve_traials;
	}
}


template< template< typename... > class Container,
		  typename... OtherArgs >
void
TraialPool::return_traials(Container< TraialRef,OtherArgs... >& traials,
						   const size_t num)
{
	const auto numTraials = (num == 0 || num >= traials.size()) ? traials.size() : num;
	auto numReturned = 0ul;
	auto it = begin(traials);
	for ( ; numReturned < numTraials ; it++, numReturned++ )
		available_traials_.push_front(it->get());
	// remove from traials all those extracted references
	traials.erase(begin(traials), it);
}

// TraialPool::return_traials() generic version can only be invoked with the following containers
template void TraialPool::return_traials(std::set< TraialRef >&, const size_t);
template void TraialPool::return_traials(std::list< TraialRef >&, const size_t);
template void TraialPool::return_traials(std::deque< TraialRef >&, const size_t);
template void TraialPool::return_traials(std::vector< TraialRef >&, const size_t);

// TraialPool::return_traials() specialization for STL std::stack<>, up to 2x faster
template <>
void
TraialPool::return_traials(std::stack< TraialRef >& stack,
						   const size_t num)
{
	const auto numTraials = num > 0ul ? num : stack.size();
	for (size_t i = 0ul ; !stack.empty() && i < numTraials ; i++) {
		available_traials_.push_front(stack.top());
		stack.pop();
	}
}

// TraialPool::return_traials() specialization for STL std::stack<>, up to 2x faster
template <>
void
TraialPool::return_traials(std::stack< TraialRef, std::vector<TraialRef> >& stack,
						   const size_t num)
{
	const auto numTraials = num > 0ul ? num : stack.size();
	for (size_t i = 0ul ; !stack.empty() && i < numTraials ; i++) {
		available_traials_.push_front(stack.top());
		stack.pop();
	}
}

// TraialPool::return_traials() specialization for STL std::forward_list<>, up to 2x faster
template <>
void
TraialPool::return_traials(std::forward_list< TraialRef >& list,
						   const size_t num)
{
	auto numReturned = 0ul;
	for (auto it = list.begin()
		; it != list.end()  &&  (num > 0ul && numReturned++ < num)
		; it = list.begin())
	{
		available_traials_.push_front(*it);
		list.pop_front();  // 'it' got invalidated
	}
}

template< typename Integral >
void
TraialPool::ensure_resources(const Integral& requiredResources)
{
	static_assert(std::is_integral< Integral >::value,
	              "ERROR: type mismatch, was expecting an integral");
	const size_t oldSize = traials_.size();
	const size_t newSize = (oldSize == 0ul) ? initial_size()
	                                        : oldSize + static_cast<size_t>(std::max(0l, static_cast<long>(requiredResources) - static_cast<long>(num_resources())));

	if (newSize <= oldSize)
		return;  // nothing to do!

	traials_.reserve(newSize);
	if (0ul == oldSize)  // called by ctor? register mem address
		::TRAIALS_MEM_ADDR = traials_.data();
	else if (traials_.data() != ::TRAIALS_MEM_ADDR)  // we're fucked (*)
		throw_FigException("memory corrupted after reallocation of the "
		                   "Traials vector; ABORTING");

	// First reproduce old references
	available_traials_.clear();
	for (size_t i = 0ul ; i < oldSize ; i++)
		available_traials_.emplace_front(std::ref(traials_[i]));
	// Then create and reference the new Traial instances
	for (size_t i = oldSize ; i < newSize ; i++) {
		traials_.emplace_back(numVariables, numClocks);  // a Traial is born
		available_traials_.emplace_front(std::ref(traials_[i]));
	}

	assert(traials_.data() == ::TRAIALS_MEM_ADDR);

	/* (*) IMPORTANT NOTE FOR DEVELOPERS
	 *
	 * Using move semantics to offer the Traial instances may cause system
	 * malfunctions derived from memory remapings.
	 *
	 * When new resources are needed and we invoke traials_.reserve(),
	 * the OS may reallocate the whole vector in a different memory segment,
	 * invalidating all the references held by the users of the TraialPool.
	 *
	 * THIS ERROR IS UNRECOVERABLE AND FORCES US TO ABORT ALL COMPUTATIONS.
	 *
	 * This problem is postponed by reserving a memory space for 'traials_'
	 * greater than the creation 'INITIAL_SIZE'. But the problem will still
	 * arise after sufficient requests for new resources.
	 */
}
// TraialPool::ensure_resources() instantiations
template void TraialPool::ensure_resources(const short& requiredResources);
template void TraialPool::ensure_resources(const int& requiredResources);
template void TraialPool::ensure_resources(const long& requiredResources);
template void TraialPool::ensure_resources(const unsigned& requiredResources);
template void TraialPool::ensure_resources(const unsigned long& requiredResources);


size_t
TraialPool::num_resources() const noexcept
{
	return std::distance(begin(available_traials_), end(available_traials_));
}


std::vector<Traial::Timeout>
TraialPool::get_timeouts(const Traial& t)
{
	return t.clocks_;
}


void
TraialPool::set_timeouts(Traial& t, std::vector<Traial::Timeout> clocks)
{
	assert(t.clocks_.size() == clocks.size());
	t.clocks_.swap(clocks);
	t.clocksValuations_.clear();
	for (auto &to: t.clocks_)
		t.clocksValuations_.emplace_back(to.sampled,to.value);
#ifndef NDEBUG
	t.check_internal_consistency();
#endif
}


void
TraialPool::clear()
{
	available_traials_.clear();
//	available_traials_.resize(0ul,traials_.front());
	traials_.clear();
	numVariables = 0ul;
	numClocks = 0ul;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
