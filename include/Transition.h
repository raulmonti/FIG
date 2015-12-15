//==============================================================================
//
//  Transition.h
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


#ifndef TRANSITION_H
#define TRANSITION_H

// C++
#include <string>
#include <utility>    // std::move(), std::forward<>()
#include <iterator>   // std::begin(), std::end()
#include <algorithm>  // std::find_if()
#include <functional>
#include <unordered_map>
// C
#include <cassert>
// FIG
#include <core_typedefs.h>  // Bitflag
#include <FigException.h>
#include <Label.h>
#include <Clock.h>
#include <Precondition.h>
#include <Postcondition.h>
#include <Traial.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

// ADL
using std::begin;
using std::end;


namespace fig
{

/**
 * @brief IOSA module transition
 *
 *        A Transition consists of an input or output label,
 *        a precondition on variable values and maybe a clock enabling it, and
 *        a postcondition with variables updates and a set of clocks to reset
 *        when the transition is taken.
 *        For a formal definition of the IOSA formalism and these transitions
 *        please visit the <a href="http://dsg.famaf.unc.edu.ar">
 *        Dependable System Group's webpage</a>.
 *
 * @note  Offers generic construction from the following STL containers:
 *        vector, list, forward_list, set, unordered_set, deque.
 * @note  Will not build from the following STL containers:
 *        queue, stack, array.
 */
class Transition
{
	friend class ModuleInstance;

	/// Synchronization label, could also be tau (viz. empty)
	Label label_;

	/// Name of the clock regulating transition applicability
	/// (empty for input transitions)
	std::string triggeringClock_;

protected:
	/// Guard regulating transition applicability
	Precondition pre;

	/// Updates to perform when transition is taken
	Postcondition pos;

	/// Clocks to reset when transition is taken
	union {
		std::vector< std::string > resetClocksList_;  // carbon version
		Bitflag resetClocks_;  // crystal version
	} __attribute__((aligned(4)));
	enum { CARBON, CRYSTAL } resetClocksData_ __attribute__((aligned(4)));

public:  // Ctors/Dtor

	/**
	 * @brief Data ctor (copies/moves {pre,post}conditions)
	 *
	 * @param label            @copydoc label_
	 * @param triggeringClock  @copydoc triggeringClock_
	 * @param pre              @copydoc pre
	 * @param pos              @copydoc pos
	 * @param resetClocks      Names of the clocks to reset when transition is taken
	 *
	 * @note Resulting transition isn't fit for simulations.
	 *       Global information still has to be provided through callback()
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	Transition(const Label& label,
			   const std::string& triggeringClock,
			   const Precondition& pre,
			   const Postcondition& pos,
			   const Container<ValueType, OtherContainerArgs...>& resetClocks);

	/// @copydoc fig::Transition::Transition()
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	Transition(const Label& label,
			   const std::string& triggeringClock,
			   Precondition&& pre,
			   Postcondition&& pos,
			   const Container<ValueType, OtherContainerArgs...>& resetClocks);

	/// Copy ctor
	Transition(const Transition&);

	/// Move ctor
	Transition(Transition&&);

	/// Can't copy assign due to Label's const string
	Transition& operator=(const Transition&) = delete;
	/// Can't move assign due to Label's const string
	Transition& operator=(Transition&&)      = delete;

	/// Dtor
	~Transition();

public:  // Read access to some attributes

	/// @copydoc label_
	inline const Label& label() const noexcept { return label_; }

	/// @copydoc triggeringClock_
	inline const std::string& triggeringClock() const noexcept { return triggeringClock_; }

	/// Clocks to reset when transition is taken, as a list of clocks names
	inline const std::vector< std::string > resetClocksList() const noexcept
		{
			if (CARBON == resetClocksData_)
				return resetClocksList_;
			else
				return std::vector< std::string >();
		}

	/// Clocks to reset when transition is taken, encoded as Bitflag
	inline const Bitflag& resetClocks() const noexcept
		{
			if (CRYSTAL == resetClocksData_)
				return resetClocks_;
			else
				return std::move(static_cast<Bitflag>(0u));
		}

//protected:  // Utilities offered to ModuleInstance
public:  // Public only for testing

	/**
	 * @brief Provide the global info needed for simulations
	 *
	 *        Acts as an asynchronous callback: the transition remains
	 *        "decompressed" until all \ref ModuleInstance "modules" have
	 *        been added to the \ref ModuleNetwork "network". The network
	 *        is then \ref ModuleNetwork::seal() "sealed", eventually
	 *        triggering this member function.
	 *
	 * @param globalClocks  Map of clock    names to their global positions
	 * @param globalVars    Map of variable names to their global positions
	 *
	 * @warning This should be called <b>exactly once</b>
	 * \ifnot NDEBUG
	 *   @throw FigException if called more than once
	 * \endif
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if some invalid mapping was found
	 * \endif
	 */
	inline void callback(const PositionsMap& globalClocks,
						 const PositionsMap& globalVars)
		{
			crystallize(globalClocks);
			pre.pin_up_vars(globalVars);
			pos.pin_up_vars(globalVars);
		}

	/**
	 * @brief Provide the global info needed for simulations
	 *
	 *        Acts as an asynchronous callback: the transition remains
	 *        "decompressed" until all \ref ModuleInstance "modules" have
	 *        been added to the \ref ModuleNetwork "network". The network
	 *        is then \ref ModuleNetwork::seal() "sealed", eventually
	 *        triggering this member function.
	 *
	 * @param globalClocks Map of clock names to their global positions
	 * @param posOfVar     Member function of State which given a variable name
	 *                     returns the position where this resides internally
	 * @param globalState  Global state instance, owner of the member function
	 *
	 * @warning This should be called <b>exactly once</b>
	 * \ifnot NDEBUG
	 *   @throw FigException if called more than once
	 * \endif
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if some invalid mapping was found
	 * \endif
	 */
	inline void callback(const PositionsMap& globalClocks,
						 std::function< size_t(const fig::State<STATE_INTERNAL_TYPE>&,
											   const std::string&)
									  > posOfVar,
						 const fig::State<STATE_INTERNAL_TYPE>& globalState)
		{
			crystallize(globalClocks);
			pre.pin_up_vars(posOfVar, globalState);
			pos.pin_up_vars(posOfVar, globalState);
		}

	/**
	 * @brief Reset and/or make time elapse in specified range of clocks
	 *
	 *        During a simulation run, and within the specified range,
	 *        the \ref resetClocks() "reset clocks" of this transition
	 *        will have their time value resampled from the corresponding
	 *        stochastic distributions. All other clocks in the specified
	 *        range will undergo an advance in their internal time of
	 *        'timeLapse' units.
	 *
	 * @param traial     Traial whose clock values will be affected
	 * @param fromClock  Iterator pointing  to  the first affected clock
	 * @param toClock    Iterator pointing past the last  affected clock
	 * @param firstClock Index of the first affected clock in the global
	 *                   vector of clocks
	 * @param timeLapse  Amount of time elapsed for the non-reseting clocks
	 *
	 * @warning callback() must have been called beforehand
	 * \ifnot NTIMECHK
	 *   @throw FigException if some clock was assigned a negative value
	 * \endif
	 *
	 * @note <b>Complexity:</b> <i>O(std::distance(from,to))</i>
	 */
	template< template< typename, typename... > class Iterator,
			  typename ValueType = Clock,
			  typename... OtherIteratorArgs >
	void handle_clocks(Traial& traial,
					   Iterator<ValueType, OtherIteratorArgs...> fromClock,
					   Iterator<ValueType, OtherIteratorArgs...> toClock,
					   const unsigned& firstClock,
					   const CLOCK_INTERNAL_TYPE& timeLapse) const;

private:  // Utils

	/**
	 * @brief Compress reset clocks "carbon version" as a Bitflag
	 *
	 * @param globalClocks Mapping of the reset clock names to their
	 *                     respective positions in a global array
	 *
	 * @warning Intended as callback to be called <b>exactly once</b>
	 * \ifnot NDEBUG
	 *   @throw FigException if called more than once
	 * \endif
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if some invalid clock index was given
	 *                       or some reset clock wasn't mapped,
	 *                       including the triggering clock
	 * \endif
	 */
	void crystallize(const PositionsMap& globalClocks);
};

// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
Transition::Transition(
	const Label& label,
	const std::string& triggeringClock,
	const Precondition& pre,
	const Postcondition& pos,
	const Container<ValueType, OtherContainerArgs...>& resetClocks) :
		label_(label),
		triggeringClock_(triggeringClock),
		pre(pre),
		pos(pos),
		resetClocksList_(),
		resetClocksData_(CARBON)
{
	static_assert(std::is_constructible< std::string, ValueType >::value,
				  "ERROR: type missmatch. Transition ctor needs a "
				  "container with the names of the resetting clocks");
	// Input enabledness: no triggering clock for input labels
	assert(label_.is_input() == triggeringClock.empty());
	// Copy reset clocks names
	resetClocksList_.insert(begin(resetClocksList_),
							begin(resetClocks),
							end(resetClocks));
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
Transition::Transition(
	const Label& label,
	const std::string& triggeringClock,
	Precondition&& pre,
	Postcondition&& pos,
	const Container<ValueType, OtherContainerArgs...>& resetClocks) :
		label_(label),
		triggeringClock_(triggeringClock),
		pre(std::forward<fig::Precondition&&>(pre)),
		pos(std::forward<fig::Postcondition&&>(pos)),
		resetClocksList_(),
		resetClocksData_(CARBON)
{
	static_assert(std::is_constructible< std::string, ValueType >::value,
				  "ERROR: type missmatch. Transition ctor needs a "
				  "container with the names of the resetting clocks");
	// Input enabledness: no triggering clock for input labels
	assert(label_.is_input() == triggeringClock.empty());
	// Copy reset clocks names
	resetClocksList_.insert(begin(resetClocksList_),
							begin(resetClocks),
							end(resetClocks));
}


template< template< typename, typename... > class Iterator,
		  typename ValueType,
		  typename... OtherIteratorArgs >
void
Transition::handle_clocks(Traial& traial,
						  Iterator<ValueType, OtherIteratorArgs...> fromClock,
						  Iterator<ValueType, OtherIteratorArgs...> toClock,
						  const unsigned& firstClock,
						  const CLOCK_INTERNAL_TYPE& timeLapse) const
{
	// Is the clock at position 'pos' marked for reset?
//	std::function<bool(const unsigned&)> must_reset =
	auto must_reset =
		[&] (const unsigned& pos) -> bool
		{
			assert(pos < 8*sizeof(Bitflag));  // check for overflow
			return resetClocks_ & ((static_cast<Bitflag>(1)) << pos);
		};
	static_assert(std::is_same< Clock*, ValueType >::value,
				  "ERROR: type missmatch. handle_clocks() takes iterators "
				  "pointing to Clock objects");
	// Make sure callback() has been called already
	assert(CRYSTAL == resetClocksData_);
	assert(pre.pinned());
	assert(pos.pinned());
	// Handle clocks
	unsigned thisClock(firstClock);
	while (fromClock != toClock) {
		if (must_reset(thisClock))
			traial.clocks_[thisClock].value = fromClock->sample();
		else
			traial.clocks_[thisClock].value -= timeLapse;
#ifndef NTIMECHK
		if (0.0f > traial.clocks_[thisClock].value)
			throw FigException(std::string("negative value for clock \"")
							   .append(fromClock->name()).append("\""));
#endif
		thisClock++;
		fromClock++;
	}
}


} // namespace fig

#endif // TRANSITION_H
