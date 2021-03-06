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
#include <unordered_map>
// C
#include <cassert>
#include <cmath>
// FIG
#include <core_typedefs.h>
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

public:

	/// Name of the clock regulating transition applicability
	/// (empty for input transitions)
	const std::string triggeringClock;

private:

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

	/// Dummy resetClocks_ bitflag prior crystallisation
	static Bitflag emptyBitflag_;

public:  // Ctors/Dtor

	/**
	 * @brief Data ctor (copies/moves {pre,post}conditions)
	 *
	 * @param label            @copydoc label_
	 * @param triggeringClock  @copydoc triggeringClock
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

	/// @copydoc pre
	inline const Precondition& precondition() const noexcept { return pre; }

	/// @copydoc pos
	inline const Postcondition& postcondition() const noexcept { return pos; }

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
				return emptyBitflag_;
		}

protected:  // Utilities offered to ModuleInstance

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
#ifndef NRANGECHK
	inline void callback(const PositionsMap& globalClocks,
						 const PositionsMap& globalVars)
#else
	inline void callback(PositionsMap& globalClocks,
						 PositionsMap& globalVars)
#endif
		{
			crystallize(globalClocks);
            pre.prepare(globalVars);
            pos.prepare(globalVars);
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
	 * @param globalClocks  Map of clock names to their global positions
	 * @param globalState   State with the position of the variables of this
	 *                      transition within the global state of the system
	 *
	 * @warning This should be called <b>exactly once</b>
	 * \ifnot NDEBUG
	 *   @throw FigException if called more than once
	 * \endif
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if some invalid mapping was found
	 * \endif
	 */
#ifndef NRANGECHK
	inline void callback(const PositionsMap& globalClocks,
						 const State<STATE_INTERNAL_TYPE>& globalState)
#else
	inline void callback(PositionsMap& globalClocks,
						 const State<STATE_INTERNAL_TYPE>& globalState)
#endif
		{
			crystallize(globalClocks);
			pre.prepare(globalState);
			pos.prepare(globalState);
		}

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
#ifndef NRANGECHK
	void crystallize(const PositionsMap& globalClocks);
#else
	void crystallize(PositionsMap& globalClocks);
#endif

public: //Debug
        void print_info(std::ostream &out) const;

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
		triggeringClock(triggeringClock),
		pre(pre),
		pos(pos),
		resetClocksList_(),
		resetClocksData_(CARBON)
{
	static_assert(std::is_constructible< std::string, ValueType >::value,
				  "ERROR: type mismatch. Transition ctor needs a "
				  "container with the names of the resetting clocks");
	// Input enabledness: no triggering clock for input labels
	assert(!label_.is_input() || triggeringClock.empty());
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
		triggeringClock(triggeringClock),
		pre(std::forward<fig::Precondition&&>(pre)),
		pos(std::forward<fig::Postcondition&&>(pos)),
		resetClocksList_(),
		resetClocksData_(CARBON)
{
	static_assert(std::is_constructible< std::string, ValueType >::value,
				  "ERROR: type mismatch. Transition ctor needs a "
				  "container with the names of the resetting clocks");
	// Input enabledness: no triggering clock for input labels
	assert(!label_.is_input() || triggeringClock.empty());
	// Copy reset clocks names
	resetClocksList_.insert(begin(resetClocksList_),
							begin(resetClocks),
							end(resetClocks));
}


} // namespace fig

#endif // TRANSITION_H
