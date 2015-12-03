//==============================================================================
//
//  ModuleInstance.h
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


#ifndef MODULEINSTANCE_H
#define MODULEINSTANCE_H

// C++
#include <vector>
#include <iterator>     // std::begin(), std::end()
#include <algorithm>    // std::find_if(), std::copy() range
#include <functional>   // std::function
#include <unordered_map>
#include <type_traits>  // std::is_same<>, std::is_constructible<>
// FIG
#include <core_typedefs.h>
#include <Module.h>
#include <State.h>
#include <Transition.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

// ADL
using std::begin;
using std::end;
using std::copy;


namespace fig
{

/**
 * @brief Single system module, possibly open regarding synchronization
 *        \ref Label "labels"
 *
 *        A module consists of \ref Variable "variables" which determine its
 *        state, \ref Clock "clocks" which mark time passage and can be reset,
 *        and \ref Transition "transitions" which describe the change dynamics
 *        of those components.
 *
 * @note  Offers generic construction from the following STL containers:
 *        vector, list, forward_list, set, unordered_set, deque.
 * @note  Will not build from the following STL containers:
 *        queue, stack, array.
 */
class ModuleInstance : public Module
{
	friend class ModuleNetwork;

	/// Local \ref Variable "variables"
	State< STATE_INTERNAL_TYPE > lState_;

	/// Local \ref Clock "clocks"
	std::vector< Clock > lClocks_;

	/// Transitions semi-ordered by their triggering \ref Clock "clock"
	std::unordered_map< std::string,
						std::vector< std::shared_ptr< Transition> > >
		transitions_by_clock_;

	/// Transitions semi-ordered by their synchronization \ref Label "label"
	std::unordered_map< std::string,
						std::vector< std::shared_ptr< Transition> > >
		transitions_by_label_;

public:

	const std::string name;

	const size_t numClocks;

private:  // Global info to be defined by the ModuleNetwork

	/// Position of this module in the global ModuleNetwork
	int globalIndex_;

	/// Index of our first clock as it would appear in a global array,
	/// where the clocks from all the modules were placed side by side.
	/// @note This is needed by Traial for mantaining the clocks internal time.
	int firstClock_;

public:  // Ctors/Dtor and populating facilities

	/**
	 * @brief Basic ctor
	 *
	 *        Builds only the local arrays of variables and clocks,
	 *        without defining the transitions.
	 *        Those can be later filled up with add_transition()
	 *
	 * @param thename  Module name
	 * @param state    Variables defined in this module
	 * @param clocks   Clocks defined in this module
	 *
	 * @note All values are copied, no move ctors are offered
	 */
	template< template< typename, typename... > class Container1,
			  typename ValueType1,
			  typename... OtherContainerArgs1 >
	ModuleInstance(const std::string& thename,
				   const State& state,
				   const Container1< ValueType1, OtherContainerArgs1... >& clocks);

	/**
	 * @brief Copy ctor from lvalue container with \ref Transition "transitions"
	 *
	 *        Builds the local arrays of variables and clocks,
	 *        and also the transitions localized in this module.
	 *        Still more transitions can be later added with add_transition()
	 *
	 * @param thename     Module name
	 * @param state       Variables defined in this module
	 * @param clocks      Clocks defined in this module
	 * @param transitions Transitions defined in this module
	 *
	 * @note All arguments are copied
	 */
	template< template< typename, typename... > class Container1,
			  typename ValueType1,
			  typename... OtherContainerArgs1 >
	template< template< typename, typename... > class Container2,
			  typename ValueType2,
			  typename... OtherContainerArgs2 >
	ModuleInstance(const std::string& thename,
				   const State& state,
				   const Container1< ValueType1, OtherContainerArgs1... >& clocks,
				   const Container2< ValueType2, OtherContainerArgs2... >& transitions);

	/**
	 * @brief Move ctor from rvalue container with \ref Transition "transition"
	 *        objects
	 *
	 *        Builds the local arrays of variables and clocks,
	 *        and also the transitions localized in this module.
	 *        Still more transitions can be later added with add_transition()
	 *
	 * @param thename     Module name
	 * @param state       Variables defined in this module
	 * @param clocks      Clocks defined in this module
	 * @param transitions Transitions defined in this module
	 *
	 * @note Variables and clocks are copied, transitions are moved
	 */
	template< template< typename, typename... > class Container1,
			  typename ValueType1,
			  typename... OtherContainerArgs1 >
	template< template< typename, typename... > class Container2,
			  typename ValueType2,
			  typename... OtherContainerArgs2 >
	ModuleInstance(const std::string& thename,
				   const State& state,
				   const Container1< ValueType1, OtherContainerArgs1... >& clocks,
				   Container2< ValueType2, OtherContainerArgs2... >&& transitions);

	/**
	 * @brief Move ctor from rvalue container with raw pointers to
	 *        \ref Transition "transitions"
	 *
	 *        Builds the local arrays of variables and clocks,
	 *        and also the transitions localized in this module.
	 *        Still more transitions can be later added with add_transition()
	 *
	 * @param thename     Module name
	 * @param state       Variables defined in this module
	 * @param clocks      Clocks defined in this module
	 * @param transitions Transitions defined in this module
	 *
	 * @note Variables and clocks are copied, transitions are moved
	 */
	template< template< typename, typename... > class Container1,
			  typename ValueType1,
			  typename... OtherContainerArgs1 >
	template< template< typename, typename... > class Container2,
			  typename ValueType2,
			  typename... OtherContainerArgs2 >
	ModuleInstance(const std::string& thename,
				   const State& state,
				   const Container1< ValueType1, OtherContainerArgs1... >& clocks,
				   Container2< ValueType2*, OtherContainerArgs2... >&& transitions);

	/**
	 * @brief Copy ctor from \ref Transition "transitions" iterator range
	 *
	 *        Builds the local arrays of variables and clocks,
	 *        and also the transitions localized in this module.
	 *        Still more transitions can be later added with add_transition()
	 *
	 * @param thename  Module name
	 * @param state    Variables defined in this module
	 * @param clocks   Clocks defined in this module
	 * @param from     Iterator  to  first transition defined in this module
	 * @param to       Iterator past last  transition defined in this module
	 *
	 * @note All arguments are copied
	 */
	template< template< typename, typename... > class Container,
			  typename ValueTypeContainer,
			  typename... OtherContainerArgs >
	template< template< typename, typename... > class Iterator,
			  typename ValueTypeIterator,
			  typename... OtherIteratorArgs >
	ModuleInstance(const std::string& thename,
				   const State& state,
				   const Container< ValueTypeContainer, OtherContainerArgs... >& clocks,
				   Iterator< ValueTypeIterator, OtherIteratorArgs... > from,
				   Iterator< ValueTypeIterator, OtherIteratorArgs... > to);

	/**
	 * @brief Add a new transition to this module
	 * @param transition Transition to copy/move
	 * @warning Do not invoke after mark_added()
	 * \ifnot NDEBUG
	 *   @throw FigException if this module has already been added to the network
	 *   @throw FigException if there's a variable or clock in 'transition'
	 *                       which doesn't belong to this module
	 * \endif
	 */
	void add_transition(const Transition& transition);

	/// @copydoc add_transition()
	void add_transition(Transition&& transition);

public:  // Utils

	virtual inline void accept(ImportanceFunction& ifun)
		{ ifun.assess_importance(this); }

	/**
	 * @brief Active module jump caused by expiration of our clock "clockName"
	 *
	 * @param clockName    Name of the clock (from this model!) which expires
	 * @param elapsedTime  Time lapse for the clock to expire
	 * @param traial       Instance of Traial to update
	 *
	 * @return Pointer to output label fired by the transition taken.
	 *         If none was enabled then 'tau' is returned.
	 *
	 * @note <b>Complexity:</b> <i>O(t+c+v)</i>, where
	 *       <ul>
	 *       <li> <i>t</i> is the number of transitions of this module,</li>
	 *       <li> <i>c</i> is the number of   clocks    of this module and</li>
	 *       <li> <i>v</i> is the number of  variables  of this module.</li>
	 *       </ul>
	 *
	 * @note Modifies sections both in StateInstance and clock-vector within "traial"
	 *       which correspond to variables and clocks from this module.
	 */
	const Label& jump(const std::string& clockName,
					  const CLOCK_INTERNAL_TYPE& elapsedTime,
					  Traial& traial);

	/**
	 * @brief Passive module jump following "label" label
	 *
	 * @param label        Output label triggered by current active jump
	 * @param elapsedTime  Time lapse for the clock to expire
	 * @param traial       Instance of Traial to update
	 *
	 * @note <b>Complexity:</b> <i>O(t+c+v)</i>, where
	 *       <ul>
	 *       <li> <i>t</i> is the number of transitions of this module,</li>
	 *       <li> <i>c</i> is the number of   clocks    of this module and</li>
	 *       <li> <i>v</i> is the number of  variables  of this module.</li>
	 *       </ul>
	 *
	 * @note Modifies sections both in StateInstance and clock-vector within "traial"
	 *       which correspond to variables and clocks from this module.
	 */
	void jump(const Label& label,
			  const CLOCK_INTERNAL_TYPE& elapsedTime,
			  Traial& traial);
private:

	/// Does the clock reside in this ModuleInstance?
	bool is_our_clock(const std::string& clockName);

	/// Build mapping of our clock names to their global positions
	/// @note Requires mark_added() to have been called beforehand
	PositionsMap map_our_clocks();

protected:  // Callback utilities offered to the ModuleNetwork

	/**
	 * @brief Report this module has been added to the network
	 *
	 *        Used by the ModuleNetwork to fill up the global-aware
	 *        information needed later during simulations.
	 *
	 * @param globalIndex  @copydoc globalIndex_
	 * @param firstClock   @copydoc firstClock_
	 *
	 * @return Const reference to our local state, to append to the global one
	 *
	 * @warning No more transitions can be added with add_transition()
	 *          after this invocation
	 * @warning Intended as callback to be called <b>exactly once</b>
	 * \ifnot NDEBUG
	 *   @throw FigException if called more than once
	 * \endif
	 */
	const State< STATE_INTERNAL_TYPE >&
	mark_added(const int& globalIndex, const int& firstClock);

	/**
	 * @brief Fill up the global-aware information needed by simulations
	 * @param globalVars Map of variable names to their global positions
	 * @warning Intended as callback to be called <b>exactly once</b>
	 * @warning mark_added() must have been called beforehand
	 */
	void seal(const PositionsMap& globalVars);

	/**
	 * @brief Fill up the global-aware information needed by simulations
	 * @param posOfVar Member function of State which given a variable name
	 *                 returns the position where this resides internally
	 * @param globalState Global state instance, owner of the member function
	 * @warning Intended as callback to be called <b>exactly once</b>
	 * @warning mark_added() must have been called beforehand
	 */
	void seal(std::function< size_t(const fig::State<STATE_INTERNAL_TYPE>&,
									const std::string&)
						   > posOfVar,
			  const fig::State<STATE_INTERNAL_TYPE>& globalState);
};


// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< template< typename, typename... > class Container1,
		  typename ValueType1,
		  typename... OtherContainerArgs1 >
ModuleInstance::ModuleInstance(
	const std::string& thename,
	const State& state,
	const Container1< ValueType1, OtherContainerArgs1... >& clocks) :
		name(thename),
		numClocks(std::distance(begin(clocks), end(clocks))),
		lState_(state),
		globalIndex_(-1),
		firstClock_(-1)
{
	// Copy clocks
	static_assert(std::is_constructible< Clock, ValueType1 >::value,
				  "ERROR: type missmatch. ModuleInstance ctors require a "
				  "container with the clocks defined in this module");
	copy(begin(clocks), end(clocks), begin(lClocks_));
}


template< template< typename, typename... > class Container1,
		  typename ValueType1,
		  typename... OtherContainerArgs1 >
template< template< typename, typename... > class Container2,
		  typename ValueType2,
		  typename... OtherContainerArgs2 >
ModuleInstance::ModuleInstance(
	const std::string& thename,
	const State& state,
	const Container1< ValueType1, OtherContainerArgs1... >& clocks,
	const Container2< ValueType2, OtherContainerArgs2... >& transitions) :
		name(thename),
		numClocks(std::distance(begin(clocks), end(clocks))),
		lState_(state),
		globalIndex_(-1),
		firstClock_(-1)
{
	// Copy clocks
	static_assert(std::is_constructible< Clock, ValueType1 >::value,
				  "ERROR: type missmatch. ModuleInstance ctors require a "
				  "container with the clocks defined in this module");
	copy(begin(clocks), end(clocks), begin(lClocks_));
	// Copy transitions
	static_assert(std::is_same< Transition, ValueType2 >::value,
				  "ERROR: type missmatch. ModuleInstance can only be copy-"
				  "constructed from a container with Transition objects");
	for(const auto& tr: transitions) {
		auto ptr = std::make_shared<Transition>(tr);
		transitions_by_label_[tr.label().str].emplace_back(ptr);
		transitions_by_clock_[tr.triggeringClock()].emplace_back(ptr);
	}
}


template< template< typename, typename... > class Container1,
		  typename ValueType1,
		  typename... OtherContainerArgs1 >
template< template< typename, typename... > class Container2,
		  typename ValueType2,
		  typename... OtherContainerArgs2 >
ModuleInstance::ModuleInstance(
	const std::string& thename,
	const State& state,
	const Container1< ValueType1, OtherContainerArgs1... >& clocks,
	Container2< ValueType2, OtherContainerArgs2... >&& transitions) :
		name(thename),
		numClocks(std::distance(begin(clocks), end(clocks))),
		lState_(state),
		globalIndex_(-1),
		firstClock_(-1)
{
	// Copy clocks
	static_assert(std::is_constructible< Clock, ValueType1 >::value,
				  "ERROR: type missmatch. ModuleInstance ctors require a "
				  "container with the clocks defined in this module");
	copy(begin(clocks), end(clocks), begin(lClocks_));
	// Move transitions
	static_assert(std::is_same< Transition, ValueType2 >::value,
				  "ERROR: type missmatch. ModuleInstance can only be move-"
				  "constructed from a container with instances or raw pointers "
				  "to Transition objects");
	for(auto&& tr: transitions) {
		auto ptr = std::make_shared<Transition>(std::forward<Transition>(tr));
		transitions_by_label_[tr.label().str].emplace_back(ptr);
		transitions_by_clock_[tr.triggeringClock()].emplace_back(ptr);
	}
	transitions.clear();
}


template< template< typename, typename... > class Container1,
		  typename ValueType1,
		  typename... OtherContainerArgs1 >
template< template< typename, typename... > class Container2,
		  typename ValueType2,
		  typename... OtherContainerArgs2 >
ModuleInstance::ModuleInstance(
	const std::string& thename,
	const State& state,
	const Container1< ValueType1, OtherContainerArgs1... >& clocks,
	Container2< ValueType2*, OtherContainerArgs2... >&& transitions) :
		name(thename),
		numClocks(std::distance(begin(clocks), end(clocks))),
		lState_(state),
		globalIndex_(-1),
		firstClock_(-1)
{
	// Copy clocks
	static_assert(std::is_constructible< Clock, ValueType1 >::value,
				  "ERROR: type missmatch. ModuleInstance ctors require a "
				  "container with the clocks defined in this module");
	copy(begin(clocks), end(clocks), begin(lClocks_));
	// Move transitions
	static_assert(std::is_same< Transition, ValueType2 >::value,
				  "ERROR: type missmatch. ModuleInstance can only be move-"
				  "constructed from a container with instances or raw pointers "
				  "to Transition objects");
	for(auto tr_ptr: transitions) {
		auto ptr = std::shared_ptr<Transition>(tr_ptr);
		assert(nullptr != ptr);
		transitions_by_label_[tr_ptr->label().str].emplace_back(ptr);
		transitions_by_clock_[tr_ptr->triggeringClock()].emplace_back(ptr);
		tr_ptr = nullptr;
	}
	transitions.clear();
}


template< template< typename, typename... > class Container,
		  typename ValueTypeContainer,
		  typename... OtherContainerArgs >
template< template< typename, typename... > class Iterator,
		  typename ValueTypeIterator,
		  typename... OtherIteratorArgs >
ModuleInstance::ModuleInstance(
	const std::string& thename,
	const State& state,
	const Container< ValueTypeContainer, OtherContainerArgs... >& clocks,
	Iterator< ValueTypeIterator, OtherIteratorArgs... > from,
	Iterator< ValueTypeIterator, OtherIteratorArgs... > to) :
		name(thename),
		numClocks(std::distance(begin(clocks), end(clocks))),
		lState_(state),
		globalIndex_(-1),
		firstClock_(-1)
{
	// Copy clocks
	static_assert(std::is_constructible< Clock, ValueTypeContainer >::value,
				  "ERROR: type missmatch. ModuleInstance ctors require a "
				  "container with the clocks defined in this module");
	copy(begin(clocks), end(clocks), begin(lClocks_));
	// Move transitions
	static_assert(std::is_same< Transition, ValueTypeIterator >::value,
				  "ERROR: type missmatch. ModuleInstance ctor needs iterators "
				  "poiting to Transition objects");
	do {
		const Transition& tr = *from;
		auto ptr = std::make_shared<Transition>(tr);
		transitions_by_label_[tr.label().str].emplace_back(ptr);
		transitions_by_clock_[tr.triggeringClock()].emplace_back(ptr);
	} while (++from != to);
}

} // namespace fig

#endif // MODULEINSTANCE_H

