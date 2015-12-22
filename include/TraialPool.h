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
#include <mutex>        // std::call_once(), std::once_flag
// FIG
#include <Traial.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


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
 * @note  The pool itself follows the
 *        <a href="https://sourcemaking.com/design_patterns/singleton">
 *        singleton design pattern</a>, thus unifying the access policy to
 *        these resources. It was implemented using C++11 facilities to make it
 *        <a href="http://silviuardelean.ro/2012/06/05/few-singleton-approaches/">
 *        thread safe</a>.
 *
 * @todo  If possible, keep Traials references instead of pointers internally,
 *        and implement all functionality using C++ move semantics.
 */
class TraialPool
{
	friend class ModuleNetwork;

	/// Single existent instance of the pool (singleton design pattern)
	static std::unique_ptr< TraialPool > instance_;

	/// Single instance thread safety
	static std::once_flag singleInstance_;

	/// Resources available for users
	static std::forward_list< std::unique_ptr< Traial > > available_traials_;

public:

	/// Size of available_traials_ on pool creation
	static const size_t initialSize = (1u) << 12;  // 4K

	/// How many new resources to allocate when either get_traial_copies() or
	/// get_traial() is invoked and available_traials_ is empty.
	static const size_t sizeChunkIncrement = initialSize >> 3;  // 1/8

private:

	/// Private ctors (singleton design pattern)
	TraialPool();
	TraialPool(TraialPool&& that)                 = delete;
	TraialPool& operator=(const TraialPool& that) = delete;

//protected:  // Global info handled by the ModuleNetwork
public:  // Public only for testing

	/// Size of the (symbolic) system global state
	static size_t numVariables;

	/// Number of clocks in the whole system model
	static size_t numClocks;

public:  // Access to the TraialPool instance

	/// Global access point to the unique instance of this pool
	/// @warning The system model must have been \ref ModuleNetowrk::seal()
	///          "sealed" beforehand
	static inline TraialPool& get_instance()
		{
			assert(0u < numVariables && 0u < numClocks);
			std::call_once(singleInstance_,
						   [] () {instance_.reset(new TraialPool);});
			return *instance_;
		}

	/// Allow syntax "auto tpool = fig::TraialPool::get_instance();"
	inline TraialPool(const TraialPool& that) {}
		// { instance_.swap(that.instance_); }

	~TraialPool();

public:  // Access to resources (viz Traials)

	/**
	 * @brief Obtain single Traial to simulate with
	 * @return Traial pointer, possibly dirty with old internal data
	 * @note <b>Complexity:</b> <i>O(1)</i> if free resources are available,
	 *       <i>O(sizeChunkIncrement)</i> if new resources need to be allocated.
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

	/// @copydoc get_traial_copies()
	/// @note Version offered for easy combination with get_traial()
	inline std::forward_list< std::unique_ptr< Traial > >
	get_traial_copies(const std::unique_ptr< Traial >& traial_p, unsigned numCopies)
		{ return get_traial_copies(*traial_p, numCopies); }

	/**
	 * @brief Give back a bunch of \ref Traial "traials" to the pool
	 *
	 * @param traials  Container with the traials to return
	 *
	 * @note  Container argument 'traials' can contain either raw or unique
	 *        pointers to Traial objects:
	 *        <ul>
	 *        <li> In the first case 'pass by rvalue' is assumed;</li>
	 *        <li> In the second case 'pass by lvalue' is assumed and all the
	 *             pointers in 'traials' are set to 'nullptr'.</li>
	 *        </ul>
	 *        In both cases the container is devoided. These measures aim at
	 *        avoiding potential memory corruption issues.
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

	/// Make sure at least 'numResources' \ref Traial "traials" are available,
	/// without the need for in-between allocations when requested.
	/// @note <b>Complexity:</b> <i>O(numResources)</i>
	void ensure_resources(const size_t& numResources);

	/// How many \ref Traial "traials" are currently available?
	/// @note <b>Complexity:</b> <i>O(num_resources())</i>
	size_t num_resources() const noexcept;
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
	traials.clear();
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

