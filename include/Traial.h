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
class ModuleNetwork;
class ImportanceFunction;

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
    friend class Transition;  // allow them to handle our clocks

public:

	/// Paraphernalia needed on clock expiration
	struct Timeout
	{
		/// Module where the expired clock exists
		std::shared_ptr<const ModuleInstance> module;
		/// Clock's name
		std::string name;
		/// Clock's time value
		float value;
		/// Data ctor
		Timeout(std::shared_ptr<const ModuleInstance> themodule,
				const std::string& thename,
				const float& thevalue) :
			module(themodule), name(thename), value(thevalue) {}
	};

public:  // Attributes

	/// Importance/Threshold level where the Traial currently is
	ImportanceValue importance;

	/// Importance/Threshold level where the Traial was born
	ImportanceValue creationImportance;

	/// Time span this Traial has been running around the system model
	CLOCK_INTERNAL_TYPE lifeTime;

	/// \ref Variable "Variables" values instantiation
	/// (same order as in the system global state)
	StateInstance state;

private:

	/// \ref Clock "Clocks" values instantiation
	/// (order given by each \ref ModuleInstance "module" internals,
	/// and in which order these were added to the network)
	std::vector< Timeout > clocks_;

	/// Time-increasing-ordered view of 'clocks_' vector.
	/// Access for friends is safely granted through next_timeout()
	std::vector< unsigned > orderedIndex_;

	/// Position of smallest not-null Clock value in clocks_.
	/// Negative if all are null.
	int firstNotNull_;

public:  // Ctors/Dtor: TraialPool should be the only to create Traials

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

	/// Copy ctor disabled to void accidental copies,
	/// only the TraialPool should explicitly create/destroy Traials
	Traial(const Traial&) = delete;  // Instantiate with 'Traial&', not 'auto'

	/// Move ctor
	Traial(Traial&& that) = default;

	/// Copy assignment
	Traial& operator=(const Traial&) = default;

	/// Move assignemnt
	Traial& operator=(Traial&&) = default;

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
	 * @param network ModuleNetwork already sealed
	 * @param impFun  ImportanceFunction currently on use for simulations
	 *
	 * @warning ModelSuite::seal() must have been called beforehand
	 * \ifnot NDEBUG
	 *   @throw FigException if the system model hasn't been sealed yet
	 * \endif
	 */
	void initialize(const ModuleNetwork& network,
					const ImportanceFunction& impFun);

	/**
	 * @brief Retrieve next not-null expiring clock
	 * @param reorder  Whether to reorder internal clocks prior the retrieval
	 * @note  <b>Complexity:</b> <i>O(m log(m))</i> if reorder, <i>O(1)</i>
	 *        otherwise, where 'm' is the number of clocks in the system.
	 * @note  Attempted inlined for efficiency, sorry
	 * @throw FigException if all our clocks have null value
	 */
	inline const Timeout&
	next_timeout(bool reorder = true)
		{
			if (reorder)
				reorder_clocks();
			if (0 > firstNotNull_)
                throw_FigException("all clocks are null! Deadlock?");
			return clocks_[firstNotNull_];
		}

    /**
     * @brief Make time elapse in specified range of clocks
     *
     *        The range [firstClock, firstClock+numClocks) should specify
     *        the global indices of all the clocks in a ModuleInstance,
     *        whose internal times need to be advanced in this Traial.
     *
     * @param firstClock First clock's index in the affected ModuleInstance
     * @param numClocks  Number of clocks of the affected ModuleInstance
     * @param timeLapse  Amount of time to kill
	 *
	 * @note  Attempted inlined for efficiency, sorry
	 */
    inline void
    kill_time(const size_t& firstClock,
              const size_t& numClocks,
              const CLOCK_INTERNAL_TYPE& timeLapse)
        {
			for (size_t i = firstClock ; i < firstClock + numClocks ; i++)
                clocks_[i].value -= timeLapse;
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
