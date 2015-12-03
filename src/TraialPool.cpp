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


#include <TraialPool.h>


namespace fig
{

// Static variables initialization

std::unique_ptr< TraialPool > TraialPool::instance_ = nullptr;

size_t TraialPool::initialSize_ = (1u) << 12;  // 4K

size_t TraialPool::sizeChunkIncrement_ = TraialPool::initialSize_ >> 3;  // 1/8

std::forward_list< std::unique_ptr< Traial > > TraialPool::available_traials_;

size_t TraialPool::numVariables = 0u;

size_t TraialPool::numClocks = 0u;


// TraialPool class member functions

TraialPool::TraialPool()
{
	for(unsigned i = 0 ; i < initialSize_ ; i++)
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
		for(unsigned i = 0 ; i < sizeChunkIncrement_; i++)
			available_traials_.emplace_front(new Traial);
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
	assert(sizeChunkIncrement_ > numCopies);  // wouldn't make sense otherwise
	// Transfer available resources
	for(; !available_traials_.empty() && 0u < numCopies ; numCopies--) {
		result.emplace_front();
		available_traials_.front().swap(result.front());
		available_traials_.pop_front();
		result.front()->operator=(traial);  // copy 'traial' values
	}
	// Run out of traials but more needed?
	if (available_traials_.empty() && 0u < numCopies) {
		for(unsigned i = 0 ; i < sizeChunkIncrement_ - numCopies; i++)
			available_traials_.emplace_front(new Traial);
		for(; 0u < numCopies ; numCopies--)
			result.emplace_front(new Traial(traial));
	}
	return result;
	/* TODO: as an alternative implementation consider using std::list,
	 *       instead of std::forward_list, and its splice() method
	 *       in combination with size() and std::advance()
	 */
}

} // namespace fig
