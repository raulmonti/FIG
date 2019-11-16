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
#include <string>
#include <vector>
#include <memory>     // std::shared_ptr<>
#include <algorithm>  // std::swap(), std::find()
#include <utility>    // std::move(), std::pair<>
#include <type_traits>  // std::is_unsigned
// FIG
#include <core_typedefs.h>
#include <FigLog.h>
#include <State.h>
#include <Clock.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


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
	friend class TraialPool;  // to instantiate (ctor)

	/// @todo TODO maybe remove following and use copy elision in TraialPool::ensure_resources()?
	friend class std::vector< Traial >;
	friend class __gnu_cxx::new_allocator<Traial>;

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
		/// Clock's position in Traial's global state
		unsigned gpos;
		/// Data ctor
		Timeout(std::shared_ptr<const ModuleInstance> themodule,
				const std::string& thename,
				const float& thevalue,
				const unsigned& theglobalpos) :
			module(themodule), name(thename), value(thevalue), gpos(theglobalpos) {}
	};

public:  // Attributes

	/// Importance/Threshold level where the Traial currently is
    ImportanceValue level;

	/// How far down the current level is w.r.t. the creation level
	int depth;

	/// Simulation's temporal field to keep track of thresholds crossing
	int numLevelsCrossed;

	/// At which level should the Traial split next time it crosses a threshold upwards?
	/// @note Relevant for RESTART with prolonged retrials
	int nextSplitLevel;

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

	/// Projection of \ref Clock "clocks" valuations into a continuous array
	/// @note Same order as clocks_ vector
	std::vector< Reference< const CLOCK_INTERNAL_TYPE > > clocksValuations_;

	/// Position of smallest non-negative Clock value in clocks_.
	/// Negative if all are null.
	int nextClock_;

private:  // Ctors: TraialPool should be the only one to create Traials

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
	 * @param whichClocks   Global positions of the clocks to initialize, if any
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

public:  // Copy/Assign/Dtor

	/// Copy ctor disabled to avoid accidental copies,
	/// only the TraialPool should explicitly create/destroy Traials
	Traial(const Traial&) = delete;  // Instantiate with 'Traial&', not 'auto'

	/// Move ctor
	Traial(Traial&& that) = default;

	/// Copy assignment
	/// @note Avoid copying the references in that.clocksValuations_
	inline Traial& operator=(const Traial& that)
	    {
		    level            = that.level;
			depth            = that.depth;
			numLevelsCrossed = that.numLevelsCrossed;
			lifeTime         = that.lifeTime;
			state            = that.state;
			clocks_          = that.clocks_;
			orderedIndex_    = that.orderedIndex_;
			nextClock_       = that.nextClock_;
			return *this;
	    }

	/// Move assignemnt
	Traial& operator=(Traial&&) = default;

	~Traial();

public:  // Accessors

	/// Get the current time value of the clock at position \p clkPos
	inline CLOCK_INTERNAL_TYPE& clock_value(const size_t& clkPos)
		{ assert(clkPos < clocks_.size()); return clocks_[clkPos].value; }

	/// @overload
	/// @note const version
	inline const CLOCK_INTERNAL_TYPE& clock_value(const size_t& clkPos) const
		{ assert(clkPos < clocks_.size()); return clocks_[clkPos].value; }

	/// Get the current time value of all clocks
	inline const decltype(clocksValuations_)& clocks_values() const
		{ return clocksValuations_; }

	/// Get the names and current time values of all the clocks in the system
	/// @param ordered Whether to return the increasing-order view of the clocks
	/// @return Fresh vector with names and values of clocks in this Traial
	std::vector< std::pair< std::string, CLOCK_INTERNAL_TYPE > >
	clocks_values(bool ordered = false) const;

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
	 * @return Reference to self
	 *
	 * \ifnot NDEBUG
	 *   @throw FigException if the system model hasn't been sealed yet
	 *                       or the ImportanceFunction has no importance info
	 * \endif
	 */
	Traial&
	initialise(const ModuleNetwork& network,
	           const ImportanceFunction& impFun);

	/**
	 * @brief Retrieve next expiring clock
	 * @param reorder  Whether to reorder internal clocks prior the retrieval
	 * @param quiet    Do not print state when a timelock is found
	 * @note  <b>Complexity:</b> <i>O(m log(m))</i> if reorder, <i>O(1)</i>
	 *        otherwise, where 'm' is the number of clocks in the system.
	 * @note  Attempted inlined for efficiency, canadian sorry
	 * @throw FigException if all clocks are expired
	 */
	inline const Timeout&
	next_timeout(bool reorder = true, bool quiet = false)
		{
			if (reorder)
				reorder_clocks();
			if (0 > nextClock_)
				report_timelock(quiet);
			return clocks_[nextClock_];
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
	 * @note  Attempted inlined for efficiency, canadian sorry
	 */
	template< typename size_type_1_, typename size_type_2_ >
	inline void
	advance_time(const size_type_1_& firstClock,
	          const size_type_2_& numClocks,
			  const CLOCK_INTERNAL_TYPE& timeLapse)
	    {
		    // checks
		    static_assert(std::is_integral<size_type_1_>::value,
			              "ERROR: type mismatch, size types must be "
			              "(non-negative) integrals");
			static_assert(std::is_integral<size_type_2_>::value,
			              "ERROR: type mismatch, size types must be "
			              "(non-negative) integrals");
			assert(static_cast<size_type_1_>(0) <= firstClock);
			assert(static_cast<size_type_2_>(0) <= numClocks);
			// code
			for (auto i = firstClock ; i < static_cast<size_type_1_>(firstClock+numClocks) ; i++)
				clocks_[i].value -= timeLapse;
		}

	/// @overload
	/// Single-clock version of the above to avoid loop (is this useful?)
	template< typename size_type_ >
	inline void
	advance_time(const size_type_& clkPos,
	          const CLOCK_INTERNAL_TYPE& timeLapse)
	    {
		    // checks
		    static_assert (std::is_integral<size_type_>::value,
			               "ERROR: type mismatch, size types must be "
			               "(non-negative) integrals");
			assert(static_cast<size_type_>(0) <= clkPos);
			//code
			clocks_[clkPos].value -= timeLapse;
	    }

	/**
	 * @brief Update the value of all clocks in specified range
	 *
	 *        The range [firstClock, firstClock+numClocks) should specify
	 *        the global indices of all the clocks in a ModuleInstance,
	 *        whose internal times will be set to the values contained
	 *        in the third argument
	 *
	 * @param firstClock  First clock's index in the affected ModuleInstance
	 * @param numClocks   Number of clocks of the affected ModuleInstance
	 * @param clockValues Container with the values to use to update the clocks,
	 *                    with at least \p numClocks elements
	 *
	 * @note  Attempted inlined for efficiency, canadian sorry
	 */
	template< typename size_type_1_, typename  size_type_2_,
	          template< typename, typename... > class Container,
			  typename ValueArg,
			  typename... OtherArgs >
	inline void
	update_clocks(const size_type_1_& firstClock,
	              const size_type_2_& numClocks,
				  const Container<ValueArg, OtherArgs...> & clockValues)
	    {
		    // checks
		    static_assert(std::is_integral<size_type_1_>::value,
			              "ERROR: type mismatch, size types must be "
			              "(non-negative) integrals");
			static_assert(std::is_integral<size_type_2_>::value,
			              "ERROR: type mismatch, size types must be "
			              "(non-negative) integrals");
			static_assert(std::is_floating_point<ValueArg>::value,
					"ERROR: type mismatch. Traial::update_clocks() takes "
					"a container with (floating point) clock values");
			assert(static_cast<size_type_1_>(0) <= firstClock);
			assert(static_cast<size_type_2_>(0) <= numClocks);
			// code
			auto clkValIter = begin(clockValues);
			for (auto i = firstClock ; i < static_cast<size_type_1_>(firstClock+numClocks) ; i++) {
				assert(clkValIter != end(clockValues));
				clocks_[i].value = *clkValIter;
				clkValIter++;
			}
		}

	/// Show Traial contents on stream
	/// @param flush Extra-verbose and flush before and after use
	void print_out(std::ostream& ostr = figTechLog, bool flush = true) const;

private:  // Class utils

	/**
	 * @brief Sort our clocks in increasing-value order for next_timeout()
	 * @note <b>Complexity:</b> <i>O(m log(m))</i>, where
	 *       m is the total number of clocks in the system
	 */
	void
	reorder_clocks();

	/// Throw an exception showing current timelock state
	/// @param  quiet  Do not show current state before throwing
	/// @throw FigException Always throws
	[[noreturn]] void
	report_timelock(bool quiet = false);
};

} // namespace fig

#endif // TRAIAL_H
