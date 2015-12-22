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
#include <iterator>
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

std::forward_list< std::unique_ptr< Traial > > TraialPool::available_traials_;

size_t TraialPool::numVariables = 0u;

size_t TraialPool::numClocks = 0u;


// TraialPool class member functions

TraialPool::TraialPool()
{
	for(unsigned i = 0 ; i < initialSize ; i++)
		available_traials_.emplace_front(new Traial(numVariables, numClocks));
}


TraialPool::~TraialPool()
{
//	available_traials_.clear();

//	Deleting this vector would be linear in its size.
//	Since the TraialPool should only be deleted after simulations conclusion,
///	@warning we ingnore this (potential?) memory leak due to its short life.
}


std::unique_ptr<Traial> TraialPool::get_traial()
{
	if (available_traials_.empty())
		for(unsigned i = 0 ; i < sizeChunkIncrement; i++)
			available_traials_.emplace_front(new Traial(numVariables, numClocks));
	std::unique_ptr< Traial > traial_p(nullptr);
	available_traials_.front().swap(traial_p);
	available_traials_.pop_front();
	return traial_p;
}


void TraialPool::return_traial(std::unique_ptr<Traial>& traial_p)
{
	assert(nullptr != traial_p);
	available_traials_.emplace_front();
	available_traials_.front().swap(traial_p);
	assert(nullptr == traial_p);
}


std::forward_list< std::unique_ptr< Traial > >
TraialPool::get_traial_copies(const Traial& traial, unsigned numCopies)
{
	std::forward_list< std::unique_ptr< Traial > > result;
	assert(sizeChunkIncrement > numCopies);  // wouldn't make sense otherwise
	// Transfer available resources
	for(; !available_traials_.empty() && 0u < numCopies ; numCopies--) {
		result.emplace_front();
		available_traials_.front().swap(result.front());
		available_traials_.pop_front();
		result.front()->operator=(traial);  // copy 'traial' values
	}
	// Run out of traials but more needed?
	if (available_traials_.empty() && 0u < numCopies) {
		for(unsigned i = 0 ; i < sizeChunkIncrement - numCopies; i++)
			available_traials_.emplace_front(new Traial(numVariables, numClocks));
		for(; 0u < numCopies ; numCopies--)
			result.emplace_front(new Traial(traial));
	}
	return result;
	/* TODO: as an alternative implementation consider using std::list,
	 *       instead of std::forward_list, and its splice() method
	 *       in combination with size() and std::advance()
	 */
}


void
TraialPool::ensure_resources(const size_t& numResources)
{
	for (size_t available = std::distance(begin(available_traials_),
										  end(available_traials_))
		; available < numResources
		; available++ )
		available_traials_.emplace_front(new Traial(numVariables, numClocks));
}


size_t
TraialPool::num_resources() const noexcept
{
	return std::distance(begin(available_traials_), end(available_traials_));
}

} // namespace fig
