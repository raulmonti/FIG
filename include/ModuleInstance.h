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
#include <State.h>
#include <Module.h>
#include <Clock.h>
#include <Transition.h>
#include <Traial.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

// ADL
using std::begin;
using std::end;
using std::copy;


namespace fig
{

class Traial;
class Property;

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
        friend class Traial;

	/// Local \ref Variable "variables"
	State< STATE_INTERNAL_TYPE > lState_;

	/// Local \ref Clock "clocks"
        std::vector< Clock > lClocks_;

        using transition_vector_t = std::vector<Reference<const Transition>>;

	/// Transitions semi-ordered by their triggering \ref Clock "clock"
        std::unordered_map<std::string, transition_vector_t>
        transitions_by_clock_;

	/// Transitions semi-ordered by their synchronization \ref Label "label"
        std::unordered_map<std::string, transition_vector_t>
        transitions_by_label_;

public:

	const std::string name;

private:  // Global info to be defined by the ModuleNetwork

	/// Position of this module in the global ModuleNetwork
	int globalIndex_;

    /// Index of our first Variable as it would appear in a global State,
    /// where the variables from all the modules were placed contiguously.
    /// @note Needed by ImportanceFunctionConcreteSplit
    int firstVar_;

	/// Index of our first clock as it would appear in a global array,
    /// where the clocks from all the modules were placed contiguously.
    /// @note Needed by Traial for mantaining the clocks internal time.
	int firstClock_;

	/// Is the module ready for simulations?
	bool sealed_;

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
				   const State< STATE_INTERNAL_TYPE >& state,
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
	 * @note This is a template of variadic templates <3
	 */
	template<
		template< typename, typename... > class Container1,
			typename ValueType1,
			typename... OtherContainerArgs1,
		template< typename, typename... > class Container2,
			typename ValueType2,
			typename... OtherContainerArgs2
	>
	ModuleInstance(const std::string& thename,
				   const State< STATE_INTERNAL_TYPE >& state,
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
	 * @note This is a template of variadic templates <3
	 */
	template<
		template< typename, typename... > class Container1,
			typename ValueType1,
			typename... OtherContainerArgs1,
		template< typename, typename... > class Container2,
			typename ValueType2,
			typename... OtherContainerArgs2
	>
	ModuleInstance(const std::string& thename,
				   const State< STATE_INTERNAL_TYPE >& state,
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
	 * @note This is a template of variadic templates <3
	 */
	template<
		template< typename, typename... > class Container1,
			typename ValueType1,
			typename... OtherContainerArgs1,
		template< typename, typename... > class Container2,
			typename ValueType2,
			typename... OtherContainerArgs2
	>
	ModuleInstance(const std::string& thename,
				   const State< STATE_INTERNAL_TYPE >& state,
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
	 * @note This is a template of variadic templates <3
	 */
	template<
		template< typename, typename... > class Container,
			typename ValueTypeContainer,
			typename... OtherContainerArgs,
		template< typename, typename... > class Iterator,
			typename ValueTypeIterator,
			typename... OtherIteratorArgs
	>
	ModuleInstance(const std::string& thename,
				   const State< STATE_INTERNAL_TYPE >& state,
				   const Container< ValueTypeContainer, OtherContainerArgs... >& clocks,
				   Iterator< ValueTypeIterator, OtherIteratorArgs... > from,
				   Iterator< ValueTypeIterator, OtherIteratorArgs... > to);

	/// Default copy ctor
	/// @note This shares the transitions with 'that', which should be safe
	///       since Transition doesn't have an internal state
	ModuleInstance(const ModuleInstance& that) = default;

	/// Default move ctor
	ModuleInstance(ModuleInstance&& that) = default;

	/// Can't copy assign since Transitions can't
	ModuleInstance& operator=(const ModuleInstance&) = delete;

	/// Can't move assign since Transitions can't
	ModuleInstance& operator=(ModuleInstance&&) = delete;

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

	/**
	 * Add a new transition to this model, built from the forwarded arguments
	 *
	 * @param label           @copydoc Transition::label_
	 * @param triggeringClock @copydoc Transition::triggeringClock
	 * @param pre             @copydoc Transition::pre
	 * @param pos             @copydoc Transition::pos
	 * @param resetClocks     Names of the clocks to reset when transition is taken
	 *
	 * @warning Do not invoke after mark_added()
	 * \ifnot NDEBUG
	 *   @throw FigException if this module has already been added to the network
	 *   @throw FigException if there's a variable or clock which doesn't belong
	 *                       to this module
	 * \endif
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	void add_transition(const Label& label,
						const std::string& triggeringClock,
						const Precondition& pre,
						const Postcondition& pos,
						const Container<ValueType, OtherContainerArgs...>& resetClocks);

public:  // Accessors

	inline std::string id() const noexcept override { return name; }

	/// Number of variables defined in this module
    /// @note Same as state_size()
	inline size_t num_vars() const noexcept { return state_size(); }

	inline size_t num_clocks() const noexcept override { return lClocks_.size(); }

	inline size_t state_size() const noexcept override { return lState_.size(); }

	inline uint128_t concrete_state_size() const noexcept override { return lState_.concrete_size(); }

	inline bool sealed() const noexcept override { return sealed_; }

	/// @copydoc lState_
	inline const State<STATE_INTERNAL_TYPE>& local_state() const noexcept { return lState_; }

	/// Get all clocks residing in this module as a const vector
	inline const std::vector< Clock >& clocks() const { return lClocks_; }

	/// @copydoc globalIndex_
    /// @note Negative value until the module is added to the network.
	inline int global_index() const noexcept { return globalIndex_; }

    /// @copydoc firstVar_
    /// @note Negative value until the module is added to the network.
    inline const int& first_var_gpos() const noexcept { return firstVar_; }

    /// @copydoc firstClock_
    /// @note Negative value until the module is added to the network.
    inline const int& first_clock_gpos() const noexcept { return firstClock_; }

public:  // Utils

	State<STATE_INTERNAL_TYPE> initial_state() const override;

	size_t initial_concrete_state() const override;

	void instantiate_initial_state(StateInstance& s) const override;

	/**
	 * @copydoc Module::adjacent_states()
	 * @param s Concrete state from the state space of this module (not global!)
	 * @note <b>Complexity:</b> <i>O(t*v)</i>, where
	 *       <ul>
	 *       <li> <i>t</i> is the number of transitions of this module,</li>
	 *       <li> <i>v</i> is the number of  variables  of this module.</li>
	 *       </ul>
	 */
	std::forward_list<size_t> adjacent_states(const size_t& s) const override;

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
	 * @note <b>Complexity:</b> <i>O(t*v+c)</i>, where
	 *       <ul>
	 *       <li> <i>t</i> is the number of transitions of this module,</li>
	 *       <li> <i>v</i> is the number of  variables  of this module.</li>
	 *       <li> <i>c</i> is the number of   clocks    of this module and</li>
	 *       </ul>
	 * @note Modifies sections both in StateInstance and clock-vector within "traial"
	 *       which correspond to variables and clocks from this module.
	 *
	 * @warning seal() must have been called beforehand
	 * \ifnot NDEBUG
	 *   @throw FigException if the module hasn't been sealed yet
	 * \endif
	 */
	const Label& jump(const Traial::Timeout& to,
					  Traial& traial) const;

	/**
	 * @brief Passive module jump following "label" label
	 *
	 * @param label        Output label triggered by current active jump
	 * @param elapsedTime  Time lapse for the clock to expire
	 * @param traial       Instance of Traial to update
	 *
	 * @note <b>Complexity:</b> <i>O(t*v+c)</i>, where
	 *       <ul>
	 *       <li> <i>t</i> is the number of transitions of this module,</li>
	 *       <li> <i>v</i> is the number of  variables  of this module.</li>
	 *       <li> <i>c</i> is the number of   clocks    of this module and</li>
	 *       </ul>
	 * @note Modifies sections both in StateInstance and clock-vector within "traial"
	 *       which correspond to variables and clocks from this module.
	 *
	 * @warning seal() must have been called beforehand
	 * \ifnot NDEBUG
	 *   @throw FigException if the module hasn't been sealed yet
	 * \endif
	 */
	void jump(const Label& label,
			  const CLOCK_INTERNAL_TYPE& elapsedTime,
			  Traial& traial) const;

	/**
	 * Basically the same as the \ref jump(const Label&, const float&, Traial&)
	 * "passive jump" but for reachability purposes only
	 *
	 * @param label Output label to which we may react
	 * @param state State to update if we have a transition reacting to 'label'
	 *
	 * @note Useful for ImportanceFunction construction, not for simulations
	 * @note <b>Complexity:</b> <i>O(t*v)</i>, where
	 *       <ul>
	 *       <li> <i>t</i> is the number of transitions of this module,</li>
	 *       <li> <i>v</i> is the number of  variables  of this module.</li>
	 *       </ul>
	 * @warning seal() must have been called beforehand
	 * \ifnot NDEBUG
	 *   @throw FigException if the module hasn't been sealed yet
	 * \endif
	 */
	void jump(const Label& label, State<STATE_INTERNAL_TYPE>& state) const;

	/// @todo TODO write docstring, Leo!
	void jump_committed(const Label &label, Traial &traial) const;

private:  // Class utils

	/// Check whether all clocks have an exponential distribution
	/// and update the markovian_ field correspondingly
	void markovian_check();

	/// Does the clock reside in this ModuleInstance?
	bool is_our_clock(const std::string& clockName) const;

	/// Build mapping of our clock names to their global positions
	/// @warning mark_added() must have been called beforehand
	PositionsMap map_our_clocks() const;

	/// Reference transitions into the semi-ordered maps by clock and by label
	/// @warning seal() must have been called beforehand
	/// \ifnot NDEBUG
	///   @throw FigException if seal() not called yet or the mapping failed
	/// \endif
	void order_transitions();

private:  // Callback utilities offered to the ModuleNetwork

	/**
	 * @brief Report this module has been added to the network
	 *
	 *        Used by the ModuleNetwork to fill up the global-aware
	 *        information needed later during simulations.
	 *
	 * @param globalIndex  @copydoc globalIndex_
     * @param firstVar     @copydoc firstVar_
	 * @param firstClock   @copydoc firstClock_
	 *
	 * @return Const reference to our local state, to append to the global one
	 *
	 * @note Synchronous callback to be called <b>exactly once</b>
	 *
	 * @warning No more transitions can be added with add_transition()
	 *          after this invocation
	 * \ifnot NDEBUG
	 *   @throw FigException if called more than once
	 * \endif
	 */
	const State< STATE_INTERNAL_TYPE >&
    mark_added(const int& globalIndex, const int& firstVar, const int& firstClock);

	/**
	 * @brief Fill up the global-aware information needed by simulations
	 *
	 * @param globalVars Map of variable names to their global positions
	 *
	 * @note Asynchronous callback to be called <b>exactly once</b>
	 *
	 * @warning mark_added() must have been called beforehand
	 * \ifnot NDEBUG
	 *   @throw FigException if called more than once
	 * \endif
	 */
	void seal(const PositionsMap& globalVars);

	/**
	 * @brief Fill up the global-aware information needed by simulations
	 *
	 * @param globalState State with the position of the variables of this
	 *                    module within the global state of the system
	 *
	 * @note Asynchronous callback to be called <b>exactly once</b>
	 *
	 * @warning mark_added() must have been called beforehand
	 * \ifnot NDEBUG
	 *   @throw FigException if called more than once
	 * \endif
	 */
	void seal(const fig::State<STATE_INTERNAL_TYPE>& globalState);


        /**
         * @brief Apply postcondition of the first enabled transition.
         * Advance clocks by elapsedTime if there is an enabled transition.
         * @param elapsedTime  Time lapse for the clock to expire
         * @param traial       Instance of Traial to update
         * @return true if a transition was enabled, false otherwise
         */
        bool
        apply_postcondition(const CLOCK_INTERNAL_TYPE &elapsedTime,
                            Traial &traial,
                            const transition_vector_t &transitions) const;
        /**
         * @brief Apply postcondition of the first enabled transition.
         * @param state to update
         * @return true if a transition was enabled, false otherwise
         */
        bool
        apply_postcondition(State<STATE_INTERNAL_TYPE>& state,
                            const transition_vector_t& transitions) const;


public: //Debug
        void print_info(std::ostream &out) const;
};


// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template<
	template< typename, typename... > class Container1,
		typename ValueType1,
		typename... OtherContainerArgs1,
	template< typename, typename... > class Container2,
		typename ValueType2,
		typename... OtherContainerArgs2 >
ModuleInstance::ModuleInstance(
	const std::string& thename,
	const State< STATE_INTERNAL_TYPE >& state,
	const Container1< ValueType1, OtherContainerArgs1... >& clocks,
	const Container2< ValueType2, OtherContainerArgs2... >& transitions) :
		lState_(state),
		name(thename),
		globalIndex_(-1),
        firstVar_(-1),
        firstClock_(-1),
		sealed_(false)
{
	// Copy clocks
	static_assert(std::is_constructible< Clock, ValueType1 >::value,
				  "ERROR: type mismatch. ModuleInstance ctors require a "
				  "container with the clocks defined in this module");
	lClocks_.insert(begin(lClocks_), begin(clocks), end(clocks));
	markovian_check();
	// Copy transitions
	static_assert(std::is_same< Transition, ValueType2 >::value,
				  "ERROR: type mismatch. ModuleInstance can only be copy-"
				  "constructed from a container with Transition objects");
	transitions_.insert(begin(transitions_), begin(transitions), end(transitions));
	for(const Transition& tr: transitions_) {
		transitions_by_label_[tr.label().str].emplace_back(tr);
		transitions_by_clock_[tr.triggeringClock].emplace_back(tr);
	}
}


template<
	template< typename, typename... > class Container1,
		typename ValueType1,
		typename... OtherContainerArgs1,
	template< typename, typename... > class Container2,
		typename ValueType2,
		typename... OtherContainerArgs2 >
ModuleInstance::ModuleInstance(
	const std::string& thename,
	const State< STATE_INTERNAL_TYPE >& state,
	const Container1< ValueType1, OtherContainerArgs1... >& clocks,
	Container2< ValueType2, OtherContainerArgs2... >&& transitions) :
		lState_(state),
		name(thename),
		globalIndex_(-1),
        firstVar_(-1),
        firstClock_(-1),
		sealed_(false)
{
	// Copy clocks
	static_assert(std::is_constructible< Clock, ValueType1 >::value,
				  "ERROR: type mismatch. ModuleInstance ctors require a "
				  "container with the clocks defined in this module");
	lClocks_.insert(begin(lClocks_), begin(clocks), end(clocks));
	markovian_check();
	// Move transitions
	static_assert(std::is_same< Transition, ValueType2 >::value,
				  "ERROR: type mismatch. ModuleInstance can only be move-"
				  "constructed from a container with instances or raw pointers "
				  "to Transition objects");
	transitions_.insert(begin(transitions_), begin(transitions), end(transitions));
	for(const Transition& tr: transitions_) {
		transitions_by_label_[tr.label().str].emplace_back(tr);
		transitions_by_clock_[tr.triggeringClock].emplace_back(tr);
	}
	transitions.clear();
}


template<
	template< typename, typename... > class Container1,
		typename ValueType1,
		typename... OtherContainerArgs1,
	template< typename, typename... > class Container2,
		typename ValueType2,
		typename... OtherContainerArgs2 >
ModuleInstance::ModuleInstance(
	const std::string& thename,
	const State< STATE_INTERNAL_TYPE >& state,
	const Container1< ValueType1, OtherContainerArgs1... >& clocks,
	Container2< ValueType2*, OtherContainerArgs2... >&& transitions) :
		lState_(state),
		name(thename),
		globalIndex_(-1),
        firstVar_(-1),
        firstClock_(-1),
		sealed_(false)
{
	// Copy clocks
	static_assert(std::is_constructible< Clock, ValueType1 >::value,
				  "ERROR: type mismatch. ModuleInstance ctors require a "
				  "container with the clocks defined in this module");
	lClocks_.insert(begin(lClocks_), begin(clocks), end(clocks));
	markovian_check();
	// Move transitions
	static_assert(std::is_same< Transition, ValueType2 >::value,
				  "ERROR: type mismatch. ModuleInstance can only be move-"
				  "constructed from a container with instances or raw pointers "
				  "to Transition objects");
	for(auto tr_ptr: transitions) {
		assert(nullptr != tr_ptr);
		transitions_.emplace_back(std::forward<Transition&&>(*tr_ptr));
		tr_ptr = nullptr;
		const Transition& tr = transitions_.back();
		transitions_by_label_[tr.label().str].emplace_back(tr);
		transitions_by_clock_[tr.triggeringClock].emplace_back(tr);
	}
	transitions.clear();
}


template<
	template< typename, typename... > class Container,
		typename ValueTypeContainer,
		typename... OtherContainerArgs,
	template< typename, typename... > class Iterator,
		typename ValueTypeIterator,
		typename... OtherIteratorArgs >
ModuleInstance::ModuleInstance(
	const std::string& thename,
	const State< STATE_INTERNAL_TYPE >& state,
	const Container< ValueTypeContainer, OtherContainerArgs... >& clocks,
	Iterator< ValueTypeIterator, OtherIteratorArgs... > from,
	Iterator< ValueTypeIterator, OtherIteratorArgs... > to) :
		lState_(state),
		name(thename),
		globalIndex_(-1),
        firstVar_(-1),
		firstClock_(-1),
		sealed_(false)
{
	// Copy clocks
	static_assert(std::is_constructible< Clock, ValueTypeContainer >::value,
				  "ERROR: type mismatch. ModuleInstance ctors require a "
				  "container with the clocks defined in this module");
	lClocks_.insert(begin(lClocks_), begin(clocks), end(clocks));
	markovian_check();
	// Move transitions
	static_assert(std::is_same< Transition, ValueTypeIterator >::value,
				  "ERROR: type mismatch. ModuleInstance ctor needs iterators "
				  "poiting to Transition objects");
	transitions_.insert(begin(transitions_), from, to);
	for(const Transition& tr: transitions_) {
		transitions_by_label_[tr.label().str].emplace_back(tr);
		transitions_by_clock_[tr.triggeringClock].emplace_back(tr);
	}
}

} // namespace fig

#endif // MODULEINSTANCE_H

