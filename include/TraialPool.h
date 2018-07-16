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
 */
class TraialPool
{
	friend class ModuleNetwork;

	/// Single existent instance of the pool (singleton design pattern)
	static std::unique_ptr< TraialPool > instance_;

	/// Single instance thread safety
	static std::once_flag singleInstance_;

	/// Container with the actual resources (i.e. Traial instances)
	static std::vector< Traial > traials_;

	/// Resources not currently in use and thus available to users
	static std::forward_list< Reference< Traial > > available_traials_;

public:

	/// Size of available_traials_ on pool creation
	static constexpr size_t INITIAL_SIZE = ((1ul)<<(18ul));  // 256 K

	/// How many new resources to allocate when either get_traial_copies() or
	/// get_traial() is invoked and available_traials_ is empty.
	static constexpr size_t INCREMENT_SIZE = ((INITIAL_SIZE)>>(6ul));  // INITIAL_SIZE/64

private:

	/// Private ctors (singleton design pattern)
	TraialPool();
	TraialPool(TraialPool&& that)                 = delete;
	TraialPool& operator=(const TraialPool& that) = delete;

private:  // Global info handled by the ModuleNetwork

	/// Size of the (symbolic) system global state
	static size_t numVariables;

	/// Number of clocks in the whole system model
	static size_t numClocks;

public:  // Access to the TraialPool instance

	/// Global access point to the unique instance of this pool
	/// @warning The model must have been \ref ModelSuite::seal() "sealed" beforehand
	/// \ifnot NDEBUG
	///   @throw FigException if ModelSuite hasn't been sealed yet
	/// \endif
	static TraialPool& get_instance();

	/// Allow syntax "auto tpool = fig::TraialPool::get_instance();"
	inline TraialPool(const TraialPool&) {}
		// { instance_.swap(that.instance_); }

	~TraialPool();

public:  // Accessors

	/// @copydoc INITIAL_SIZE
	static inline size_t initial_size() { return INITIAL_SIZE; }

	/// @copydoc INCREMENT_SIZE
	static inline size_t increment_size() { return INCREMENT_SIZE; }

public:  // Access to resources (viz Traials)

	/**
	 * @brief Obtain single (dirty) Traial to simulate with
	 * @details Instantiate in the following way:
	 *          \code
	 *          Traial& varname = TraialPool::get_instance().get_traial();
	 *          \endcode
	 *          Don't use the 'auto' keyword to define the variable.
	 * @return Dirty Traial
	 * @note <b>Complexity:</b> <i>O(1)</i> if free resources are available,
	 *       <i>O(INCREMENT_SIZE)</i> if new resources need to be allocated.
	 */
	Traial& get_traial();

	/**
	 * @brief Give back single Traial to the pool
	 * @note <b>Complexity:</b> <i>O(1)</i>
	 * @warning Argument is invalidated after call
	 */
	void return_traial(Traial&& traial);

	/// @copydoc return_traial()
	void return_traial(Reference<Traial> traial);

    /**
     * Obtain specified amount of (dirty) Traial instances
     *
	 * @param cont       Container where traials will be stored (added/emplaced)
     * @param numTraials Number of \ref Traial "traials" requested
     *
     * @note <b>Complexity:</b> <i>O(numTraials)</i> if enough free resources
	 *       are available, <i>O(max(numTraials,INCREMENT_SIZE))</i>
     *       if new resources need to be allocated.
     */
    template< template< typename... > class Container,
              typename... OtherArgs >
    void get_traials(Container< Reference<Traial>, OtherArgs...>& cont,
                     unsigned numTraials);

	/**
	 * Obtain specified amount of copies of given Traial instance with 0 depth
     *
	 * @param cont      Container where Traial copies will be stored (added/emplaced)
	 * @param traial    Traial instance whose internals will be copied
     * @param numCopies Number of \ref Traial "traials" requested
     * @param depth     Depth assigned to the delivered \ref Traial "traials"
     *
     * @note <b>Complexity:</b> <i>O(numCopies)</i> if enough free resources
	 *       are available, <i>O(max(numCopies,INCREMENT_SIZE))</i>
     *       if new resources need to be allocated.
     */
	template< template< typename... > class Container,
			  typename... OtherArgs >
    void get_traial_copies(Container< Reference<Traial>, OtherArgs...>& cont,
                           const Traial& traial,
                           unsigned numCopies,
                           short depth = 0);

	/**
	 * @brief Give back a bunch of \ref Traial "traials" to the pool
	 * @param traials  Container with the references to the returned traials
	 * @note The container is devoided to avoid potential memory corruption issues
	 * @note <b>Complexity:</b> <i>O(size(traials))</i>
	 */
	template< template< typename... > class Container,
			  typename... OtherArgs >
	void return_traials(Container< Reference<Traial>, OtherArgs...>& traials);

public:  // Utils

	/// Make sure at least 'requiredResources' \ref Traial "traials" are
	/// available, without the need for in-between allocations when requested.
	/// @note <b>Complexity:</b> <i>O(max(requiredResources,num_resources()))</i>
	void ensure_resources(const size_t& requiredResources);

	/// How many \ref Traial "traials" are currently available?
	/// @note <b>Complexity:</b> <i>O(num_resources())</i>
	size_t num_resources() const noexcept;

	/// Allow our friend ModuleNetwork to get the time-state of a Traial
	/// @return The Timeouts vector of the Traial (aka its active clocks)
	/// @note Used by ModuleNetwork::peak_simulation()
	/// @note Using the friend of a friend to access private data is nasty,
	///       don't do this at home
	static std::vector<Traial::Timeout> get_timeouts(const Traial& t);

	/// Allow our friend ModuleNetwork to change the time-state of a Traial
	/// @param t      Traial whose Timeouts vector will be modified
	/// @param clocks Vector of Timeouts which will
	/// @note Used by ModuleNetwork::peak_simulation()
	/// @note Using the friend of a friend to access private data is nasty,
	///       don't do this at home
	static void set_timeouts(Traial& t, std::vector<Traial::Timeout> clocks);

	/// Delete all instances of Traial we hold,
	/// and erase infor like \p numVariables and \p numClocks
	static void clear();
};

} // namespace fig

#endif // TRAIALPOOL_H

