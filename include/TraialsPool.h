//==============================================================================
//
//  TraialsPool.h
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


#ifndef TRAIALSPOOL_H
#define TRAIALSPOOL_H

// C++
#include <vector>
#include <memory>  // std::unique_ptr<>
// FIG
#include <Traial.h>


namespace fig
{

/**
 * @brief Resource pool for Traial instances.
 *
 *        To avoid countless creations and destructions of such ephemeral
 *        objects as only \ref Traial "traial instances" can be, we manage
 *        this core resource for rare event simulation by means of a
 *        <a href="https://sourcemaking.com/design_patterns/object_pool">
 *        resource pool</a>.
 *
 * @note  The pool itself follows the singleton design pattern,
 *        thus unifying the access policy to these resources.
 */
class TraialPool
{
	/// Single existent instance of the pool
	static std::unique_ptr< TraialPool > instance_;

	/// Resources available for users
	/// @todo TODO decide which TAD is best. Maybe forward_list?
	///       We need to ensure O(1) get/return for single traials
	static std::list< Traial > available_traials_;

	/// Size of available_traials_ on pool creation
	static size_t initialSize_;

	/// How many new resources to allocate when either get_traial_copies() or
	/// get_traial() is invoked and available_traials_ is empty.
	static size_t sizeIncrement_;

	/// Private ctor
	TraialPool() : available_traials_(initialSize_) {}

	/// Proclaim to the four winds the uniqueness of the single instance
	TraialPool(const TraialPool& that)            = delete;
	TraialPool(TraialPool&& that)                 = delete;
	TraialPool& operator=(const TraialPool& that) = delete;

public:  // Access to TraialPool

	/// Global access point to the unique instance of this pool
	static TraialPool& get_instance() {
		if (nullptr == instance_)
			instance_ = std::unique_ptr< TraialPool >(new TraialPool);
		return *instance_;
	}

	~TraialPool() { available_traials_.clear(); }

public:  // Access to resources (viz Traials)

	/**
	 * @brief  Obtain single Traial to simulate with
	 * @return Traial instance, possibly dirty with old internal data
	 * @note   <b>Complexity:</b> <i>O(1)</i> if free resources are available,
	 *         <i>O(sizeIncrement_)</i> if new resources need to be allocated.
	 */
	Traial& get_traial();

	/**
	 * @brief  Obtain specified amount of copies of given Traial instance
	 *
	 * @param  traial    Traial instance whose internals will be copied
	 * @param  numCopies Number of \ref Traial "traials" requested
	 *
	 * @return @todo Decide on the proper STL container to return
	 * @note   <b>Complexity:</b> <i>O(numCopies)</i> if free resources are
	 *         available, <i>O(max(numCopies,sizeIncrement_))</i>
	 *         if new resources need to be allocated.
	 */
	std::list< Traial > get_traial_copies(const Traial& traial,
										  const unsigned& numCopies);

	/**
	 * @brief Return single Traial to the pool
	 * @note <b>Complexity:</b> <i>O(1)</i>
	 */
	void return_traial(Traial&& traial);
	/// @copydoc return_traial()
	void return_traial(Traial* traial);

	/**
	 * @brief Return bunch of \ref Traial "traials" to the pool,
	 *        passing them as references.
	 * @param traials  Container with the Traial references
	 * @note  Container is devoided
	 * @note  <b>Complexity:</b> <i>O(size(traials))</i>
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	return_traials(Container<ValueType, OtherContainerArgs...>&& traials);

	/**
	 * @brief Return bunch of \ref Traial "traials" to the pool,
	 *        passing them as raw pointers.
	 * @param traials  Container with the Traial pointers
	 * @note  Container is devoided
	 * @note  <b>Complexity:</b> <i>O(size(traials))</i>
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	return_traials(Container<ValueType*, OtherContainerArgs...>&& traials);

public:  // Utils

	/**
	 * @brief Make sure at least numResources are available for users
	 *        acquisition without then need for in-between allocations.
	 */
	void ensure_resources(const size_t numResources);
};

} // namespace fig

#endif // TRAIALSPOOL_H

