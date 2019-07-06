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

	/// Probabilistic weights of the postcondition branches
	/// that this transition could take
	std::vector< float > probabilities;

	/// Updates to perform when transition is taken,
	/// divided in branches each with its own probabilistic weight
	std::vector< Postcondition > posts;

	/// Number of clocks to reset, considering all probabilistic branches
	size_t numClocks;

	/// Clocks to reset when transition is taken
	/// divided in branches each with its own probabilistic weight
	union {
		std::vector< std::set< std::string > > resetClocksList_;  // carbon version
		std::vector< Bitflag > resetClocks_;                         // crystal version
	} __attribute__((aligned(4)));
	enum { CARBON, CRYSTAL } resetClocksData_ __attribute__((aligned(4)));

	/// Auxiliary for reset clocks over all branches
	mutable std::vector< Bitflag > allResetClocks;

	/// Auxiliary for reset clocks over all branches
	mutable std::vector< string > allResetClocksNames;

public:  // Ctors/Dtor

	/**
	 * @brief Data ctor (copies/moves {pre,post}conditions)
	 *
	 * @param label            @copydoc label_
	 * @param triggeringClock  @copydoc triggeringClock
	 * @param pre              @copydoc pre
	 * @param probabilities    @copydoc probabilities
	 * @param posts            @copydoc posts
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
	           const std::vector< float >& probabilities,
	           const std::vector< Postcondition >& posts,
			   const Container<ValueType, OtherContainerArgs...>& resetClocks);

	/// @copydoc fig::Transition::Transition()
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	Transition(const Label& label,
			   const std::string& triggeringClock,
			   Precondition&& pre,
	           std::vector< float >&& probabilities,
	           std::vector< Postcondition >&& posts,
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

public:  // Accessors

	/// @copydoc label_
	inline const Label& label() const noexcept { return label_; }

	/// Number of (probabilistic) branches leading out of this transition
	inline size_t num_branches() const noexcept { return probabilities.size(); }

	/// @copydoc numClocks
	inline const decltype(numClocks)& num_clocks() const noexcept { return numClocks; }

	/// @copydoc pre
	inline const decltype(pre)& precondition() const noexcept { return pre; }

	/// @copydoc posts
	inline const decltype(posts)& postconditions() const noexcept { return posts; }

	/// Clocks to reset when transition is taken, as a list of clocks names
	inline const std::vector< std::set< std::string > > reset_clocks_names() const noexcept
	    {
		    if (CARBON == resetClocksData_)
				return resetClocksList_;
			else
				return std::vector< std::set< std::string >>();
	    }

	/// Clocks to reset when transition is taken, encoded as Bitflag
	inline const std::vector< Bitflag >& reset_clocks() const noexcept
	    {
		    if (CRYSTAL == resetClocksData_)
				return resetClocks_;
			else
				return allResetClocks;
	    }

	/// Flattened version of resetClocksNames ("ignoring" probabilistic branching)
	std::vector< std::string > reset_clocks_names_list() const noexcept;

	/// Flattened version of resetClocks ("ignoring" probabilistic branching)
	const Bitflag& reset_clocks_list() const noexcept;

public:  // Utils

	/**
	 * Choose (probabilisitcally) a branch and apply its postcondition
	 * @param t      Traial to update its state (variables) and clocks values
	 * @param clocks Clocks of the ModuleInstance to which this Transition belongs
	 * @param fClk   Global position of the first clock of the ModuleInstance
	 * @note The state (variables) and clocks of the Traial are modified
	 */
	template< typename Integral >
	void apply_postcondition(Traial& t,
	                         const std::vector< Clock >& clocks,
	                         Integral fClk) const;

	/**
	 * Like apply_postcondition() but ignoring clocks (only update state)
	 * @param state   State to update with a possible postcondition
	 * @param branch  If set and valid, apply that branch postcondition;
	 *                else choose probabilistically which branch to follow
	 * @return The state resulting from applying the chosen postcondition to \p state
	 * @note Intended for state-space exploration purposes
	 */
	State< STATE_INTERNAL_TYPE >
	apply_postcondition(const State<STATE_INTERNAL_TYPE> &state, int branch = -1) const;

	/// States resulting from applying any possible postcondition to \p state,
	/// i.e. considering all probabilistic branching
	std::set< State< STATE_INTERNAL_TYPE > >
	apply_postconditions(const State<STATE_INTERNAL_TYPE> &state) const;

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
			for (auto& pbranch: posts)
				pbranch.prepare(globalVars);
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
			for (auto& pbranch: posts)
				pbranch.prepare(globalState);
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

public:  // Debug

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
    const std::vector< float >& probs,
    const std::vector<Postcondition>& posts,
	const Container<ValueType, OtherContainerArgs...>& resetClocks) :
		label_(label),
		triggeringClock(triggeringClock),
		pre(pre),
        probabilities(probs.size()),
        posts(posts),
		resetClocksList_(),
        resetClocksData_(CARBON),
        allResetClocks(1ul)
{
	static_assert(std::is_constructible< std::set< std::string >, ValueType >::value,
	              "ERROR: type mismatch. Transition ctor needs a "
	              "container with the names of the resetting clocks");
	// Input enabledness: no triggering clock for input labels
	assert(!label_.is_input() || triggeringClock.empty());
	// Copy reset clocks names
	if (std::distance(begin(resetClocks), end(resetClocks)) > 0) {
		resetClocksList_.insert(begin(resetClocksList_),
		                        begin(resetClocks),
		                        end(resetClocks));
		for (const auto& clockSet: resetClocksList_)
			numClocks += clockSet.size();
	} else {
		resetClocksList_.emplace_back();  // no clocks reset in this transition
	}
	// Prepare probabilities vector for efficient branch selection
	for (auto i = 0ul ; i < probs.size() ; i++)
		probabilities[i] = probabilities[(i-1)%probs.size()] + probs[i];
	// The probabilistic weigths of the branches must add up to 1.0
	if (probabilities.back() != 1.0f)
		throw_FigException("branch probabilities don't add up to 1.0");
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
Transition::Transition(
	const Label& label,
	const std::string& triggeringClock,
	Precondition&& pre,
    std::vector< float >&& probs,
    std::vector< Postcondition >&& posts,
	const Container<ValueType, OtherContainerArgs...>& resetClocks) :
		label_(label),
		triggeringClock(triggeringClock),
		pre(std::forward<fig::Precondition&&>(pre)),
        probabilities(probs.size()),
        posts(std::forward<std::vector<fig::Postcondition>&&>(posts)),
        numClocks(0ul),
		resetClocksList_(),
        resetClocksData_(CARBON),
        allResetClocks(1ul)
{
	static_assert(std::is_constructible< std::set< std::string >, ValueType >::value,
				  "ERROR: type mismatch. Transition ctor needs a "
				  "container with the names of the resetting clocks");
	// Input enabledness: no triggering clock for input labels
	assert(label_.is_input() == triggeringClock.empty());
	// Copy reset clocks names
	if (std::distance(begin(resetClocks), end(resetClocks)) > 0) {
		resetClocksList_.insert(begin(resetClocksList_),
		                        begin(resetClocks),
		                        end(resetClocks));
		for (const auto& clockSet: resetClocksList_)
			numClocks += clockSet.size();
	} else {
		resetClocksList_.emplace_back();  // no clocks reset in this transition
	}
	// Prepare probabilities vector for efficient branch selection
	for (auto i = 0ul ; i < probs.size() ; i++)
		probabilities[i] = probabilities[(i-1)%probs.size()] + probs[i];
	// The probabilistic weigths of the branches must add up to 1.0
	if (probabilities.back() != 1.0f)
		throw_FigException("branch probabilities don't add up to 1.0");
}


} // namespace fig

#endif // TRANSITION_H
