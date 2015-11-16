//==============================================================================
//
//  TraialPool.h
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


#ifndef TRAIALPOOL_H
#define TRAIALPOOL_H

// C++
#include <forward_list>
#include <type_traits>  // std::is_same<>
#include <memory>       // std::unique_ptr<>
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
 *
 * @todo  If possible, keep Traials references instead of pointers internally,
 *        and implement all functionality using C++ move semantics.
 */
class TraialPool
{
	/// Single existent instance of the pool (singleton design pattern)
	static std::unique_ptr< TraialPool > instance_;

	/// Resources available for users
	static std::forward_list< std::unique_ptr< Traial > > available_traials_;

	/// Size of available_traials_ on pool creation
	static size_t initialSize_;

	/// How many new resources to allocate when either get_traial_copies() or
	/// get_traial() is invoked and available_traials_ is empty.
	static size_t sizeChunkIncrement_;

	/// Private ctor (singleton design pattern)
	TraialPool();

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

	~TraialPool();

public:  // Access to resources (viz Traials)

	/**
	 * @brief Obtain single Traial to simulate with
	 * @return Traial pointer, possibly dirty with old internal data
	 * @note <b>Complexity:</b> <i>O(1)</i> if free resources are available,
	 *       <i>O(sizeChunkIncrement_)</i> if new resources need to be allocated.
	 */
	std::unique_ptr< Traial > get_traial();

	/**
	 * @brief Give back single Traial to the pool
	 * @note <b>Complexity:</b> <i>O(1)</i>
	 * @warning Argument becomes nullptr after call
	 */
	void return_traial(std::unique_ptr<Traial>& traial_p);

	/**
	 * @brief  Obtain specified amount of copies of given Traial instance
	 *
	 * @param  traial    Traial instance whose internals will be copied
	 * @param  numCopies Number of \ref Traial "traials" requested
	 *
	 * @return <a href="http://www.cplusplus.com/reference/forward_list/forward_list/">
	 *         C++ STL forward list</a> with requested copies of traial
	 *
	 * @note   <b>Complexity:</b> <i>O(numCopies)</i> if free resources are
	 *         available, <i>O(max(numCopies,sizeIncrement_))</i>
	 *         if new resources need to be allocated.
	 */
	std::forward_list< std::unique_ptr< Traial > >
	get_traial_copies(const Traial& traial, unsigned numCopies);

	/**
	 * @brief Give back a bunch of \ref Traial "traials" to the pool
	 *
	 * @param traials  Container with the traials to return
	 *
	 * @note  Container argument 'traials' can contain either raw or unique
	 *        pointers to Traial objects:
	 *        <ul>
	 *        <li> In the first case 'pass by rvalue' is assumed and the
	 *             container is devoided;</li>
	 *        <li> In the second case 'pass by lvalue' is assumed and all the
	 *             pointers in 'traials' are set to 'nullptr'.</li>
	 *        </ul>
	 *        These measures avoid potential memory corruption issues.
	 * @note  <b>Complexity:</b> <i>O(size(traials))</i>
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	void return_traials(Container<ValueType, OtherContainerArgs...>& traials);

	/// @copydoc return_traials()
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	void return_traials(Container<ValueType*, OtherContainerArgs...>&& traials);

public:  // Utils

	/**
	 * @brief Make sure at least numResources are available for users
	 *        acquisition without then need for in-between allocations.
	 */
	void ensure_resources(const size_t numResources);
};


// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
void
TraialPool::return_traials(Container<ValueType, OtherContainerArgs...>& traials)
{
	static_assert(std::is_same< std::unique_ptr< Traial >, ValueType >::value,
				  "ERROR: type missmatch. Only Traial pointers, either raw or "
				  "unique, can be returned to the TraialPool.");
	for (auto& uptr: traials) {
		available_traials_.emplace_front();
		uptr.swap(available_traials_.front());
		assert(nullptr == uptr);
	}
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
void
TraialPool::return_traials(Container<ValueType*, OtherContainerArgs...>&& traials)
{
	static_assert(std::is_same< Traial, ValueType >::value,
				  "ERROR: type missmatch. Only Traial pointers, either raw or "
				  "unique, can be returned to the TraialPool.");
	for (auto ptr: traials)
		available_traials_.emplace_front(ptr);
	traials.clear();  // keep user from tampering with those dangerous pointers
}

} // namespace fig

#endif // TRAIALPOOL_H

