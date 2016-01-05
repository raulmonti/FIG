//==============================================================================
//
//  Traial.h
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


#ifndef TRAIAL_H
#define TRAIAL_H

// C++
#include <vector>
#include <memory>     // std::shared_ptr<>
#include <iterator>   // std::begin(), std::end()
#include <algorithm>  // std::swap(), std::find()
#include <utility>    // std::move()
#include <numeric>    // std::iota()
// FIG
#include <core_typedefs.h>
#include <State.h>
#include <Clock.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

// ADL
using std::begin;
using std::end;


namespace fig
{

class ModuleInstance;

/**
 * @brief Simulation kernel (or 'trial trail')
 *
 *        Simulations are embodied through instances of this class.
 *        A Traial holds the state of the variables and the clocks values,
 *        i.e. all that is needed to "run through" the user's system model.
 *        Traials should be handled with the TraialPool, to ensure fast
 *        acquisition/release (instead of creation/destruction) of the
 *        instances.
 */
class Traial
{
	friend class Transition;
	friend class ModuleNetwork;

protected:

	/// Paraphernalia needed on clock expiration
	struct Timeout
	{
		/// Module where the expired clock exists
		const std::shared_ptr<const ModuleInstance> module;
		/// Clock's name
		const std::string& name;
		/// Clock's time value
		float value;
		/// Data ctor
		Timeout(std::shared_ptr<ModuleInstance> themodule,
				const std::string& thename,
				const float& thevalue) :
			module(themodule), name(thename), value(thevalue) {}
		// Other ctors
		Timeout(const Timeout& that) = default;
		Timeout(Timeout&& that)      = default;
		Timeout& operator=(const Timeout& that) = delete;
		Timeout& operator=(Timeout&& that)      = delete;
	};

public:  // Attributes

	/// Importance/Threshold level where the Traial currently is
	ImportanceValue importance;

	/// Importance/Threshold level where the Traial was born
	ImportanceValue creationImportance;

	/// Time this Traial has been running around the system model
	CLOCK_INTERNAL_TYPE lifeTime;

	/// \ref Variable "Variables" values instantiation
	/// (same order as in the system global state)
	StateInstance state;

protected:

	/// \ref Clock "Clocks" values instantiation
	/// (order given by each \ref ModuleInstance "module" internals,
	/// and in which order these were added to the network)
	std::vector< Timeout > clocks_;

private:

	/// Time-increasing-ordered view of 'clocks_' vector.
	/// Access for friends is safely granted through next_timeout()
	std::vector< unsigned > orderedIndex_;

	/// Position of smallest not-null Clock value in clocks_.
	/// Negative if all are null.
	int firstNotNull_;

public:  // Ctors/Dtor

	/**
	 * @brief Void ctor for resources pool
	 * @param stateSize Symbolic size of the global state, i.e. # of variables
	 *        in the system model (counting from all modules)
	 * @param numClocks # of clocks in the system model (counting from all modules)
	 */
	Traial(const size_t& stateSize, const size_t& numClocks);

	/**
	 * @brief Data ctor
	 *
	 * @param stateSize     Symbolic size of the global State
	 * @param numClocks     Number of clocks in the whole system
	 * @param whichClocks   Which clocks to initialize, if any
	 * @param orderTimeouts Whether to order the timeouts after initializations
	 *
	 * @note By default, and regardless of clocks initialization,
	 *       the timeouts won't be ordered. To force ordering call
	 *       with last parameter set to <b>true</b>.
	 */
	Traial(const size_t& stateSize,
		   const size_t& numClocks,
		   Bitflag whichClocks,
		   bool orderTimeouts = false);

	/// @copydoc Traial(const size_t&, const size_t&, fig::Bitflag, bool)
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	Traial(const size_t& stateSize,
		   const size_t& numClocks,
		   const Container<ValueType, OtherContainerArgs...>& whichClocks,
		   bool orderTimeouts = false);

	/// Copy ctor
	inline Traial(const Traial& that) :
		state(that.state),
		clocks_(that.clocks_),
		orderedIndex_(that.orderedIndex_),
		firstNotNull_(that.firstNotNull_)
		{}

	/// Move ctor
	inline Traial(Traial&& that) :
		state(std::move(that.state)),
		clocks_(std::move(that.clocks_)),
		orderedIndex_(std::move(that.orderedIndex_)),
		firstNotNull_(std::move(that.firstNotNull_))
		{}

	/// Copy assignment with copy&swap idiom
	inline Traial& operator=(Traial that)
		{
			std::swap(state, that.state);
			std::swap(clocks_, that.clocks_);
			std::swap(orderedIndex_, that.orderedIndex_);
			std::swap(firstNotNull_, that.firstNotNull_);
			return *this;
		}

	~Traial();

public:  // Utils

	/**
	 * @brief Reset traial to the initial conditions of the system model
	 *
	 *        When a new simulation starts, the \ref Variable "system variables"
	 *        should begin at their initial values, and the \ref Clock "initial
	 *        clocks" should be reset with their corresponding distributions.
	 *        This member function resets the Traial instance to comply
	 *        with such initial conditions.
	 *
	 * @warning ModelSuite::seal() must have been called beforehand
	 * \ifnot NDEBUG
	 *   @throw FigException if the system model hasn't been sealed yet
	 * \endif
	 */
	void initialize();

protected:

	/**
	 * @brief Retrieve next not-null expiring clock
	 * @param reorder  Whether to reorder internal clocks prior the retrieval
	 * @note  <b>Complexity:</b> <i>O(m log(m))</i> if reorder, <i>O(1)</i>
	 *        otherwise, where 'm' is the number of clocks in the system.
	 * @throw FigException if all our clocks have null value
	 */
	inline const Timeout&
	next_timeout(bool reorder = true)
		{
			if (reorder)
				reorder_clocks();
			if (0 > firstNotNull_)
				throw FigException("all clocks are null!");
			return clocks_[firstNotNull_];
		}

private:

	/**
	 * @brief Sort our clocks in increasing-value order for next_timeout()
	 * @note <b>Complexity:</b> <i>O(m log(m))</i>, where
	 *       m is the total number of clocks in the system
	 */
	void
	reorder_clocks();
};

} // namespace fig

#endif // TRAIAL_H
