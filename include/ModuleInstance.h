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
#include <unordered_map>
#include <type_traits>  // std::is_same<>, std::is_constructible<>
// FIG
#include <Module.h>
#include <Transition.h>
//#include <ImportanceFunction.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


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
	// Our range of variables
	/// Position in State of our first Variable
	const unsigned firstVar_;
	/// Number of \ref Variable "variables" in this module
	const unsigned numVars_;

	// Our range of clocks
	/// Position in the global 'gClocks' of our first Clock
	const unsigned firstClock_;
	/// Number of \ref Clock "clocks" in this module
	const unsigned numClocks_;

	// Our transitions
	std::unordered_map< std::string,
						std::vector< std::shared_ptr< Transition> > >
		transitions_by_clock_;
	// NOTE: tell input/output apart here?
	std::unordered_map< std::string,
						std::vector< std::shared_ptr< Transition> > >
		transitions_by_label_;

public:  // Ctors

	/**
	 * @brief Copy ctor from lvalue container with \ref Transition "transitions"
	 *
	 * @param firstVar     Position in State of our first Variable
	 * @param numVars      Number of \ref Variable "variables" in this module
	 * @param firstClock   Position in global 'gClocks' of our first Clock
	 * @param numClocks    Number of \ref Clock "clocks" in this module
	 * @param transitions  Transitions defined in this module
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	ModuleInstance(const unsigned& firstVar,
				   const unsigned& numVars,
				   const unsigned& firstClock,
				   const unsigned& numClocks,
				   const Container< ValueType, OtherContainerArgs... >& transitions);

	/**
	 * @brief Move ctor from rvalue container with \ref Transition "transition"
	 *        objects
	 *
	 * @param firstVar     Position in State of our first Variable
	 * @param numVars      Number of \ref Variable "variables" in this module
	 * @param firstClock   Position in global 'gClocks' of our first Clock
	 * @param numClocks    Number of \ref Clock "clocks" in this module
	 * @param transitions  Transitions defined in this module
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	ModuleInstance(const unsigned& firstVar,
				   const unsigned& numVars,
				   const unsigned& firstClock,
				   const unsigned& numClocks,
				   Container< ValueType, OtherContainerArgs... >&& transitions);

	/**
	 * @brief Move ctor from rvalue container with raw pointers to
	 *        \ref Transition "transitions"
	 *
	 * @param firstVar     Position in State of our first Variable
	 * @param numVars      Number of \ref Variable "variables" in this module
	 * @param firstClock   Position in global 'gClocks' of our first Clock
	 * @param numClocks    Number of \ref Clock "clocks" in this module
	 * @param transitions  Transitions defined in this module
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	ModuleInstance(const unsigned& firstVar,
				   const unsigned& numVars,
				   const unsigned& firstClock,
				   const unsigned& numClocks,
				   Container< ValueType*, OtherContainerArgs... >&& transitions);

	/**
	 * @brief Copy ctor from \ref Transition "transitions" iterator range
	 *
	 * @param firstVar    Position in State of our first Variable
	 * @param numVars     Number of \ref Variable "variables" in this module
	 * @param firstClock  Position in global 'gClocks' of our first Clock
	 * @param numClocks   Number of \ref Clock "clocks" in this module
	 * @param from        Iterator to  first transition defined in this module
	 * @param to          Iterator past last transition defined in this module
	 */
	template< template< typename, typename... > class Iterator,
			  typename ValueType,
			  typename... OtherIteratorArgs >
	ModuleInstance(const unsigned& firstVar,
				   const unsigned& numVars,
				   const unsigned& firstClock,
				   const unsigned& numClocks,
				   Iterator< ValueType, OtherIteratorArgs... > from,
				   Iterator< ValueType, OtherIteratorArgs... > to);


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
};


// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
ModuleInstance::ModuleInstance(
	const unsigned& firstVar,
	const unsigned& numVars,
	const unsigned& firstClock,
	const unsigned& numClocks,
	const Container< ValueType, OtherContainerArgs... >& transitions) :
		firstVar_(firstVar),
		numVars_(numVars),
		firstClock_(firstClock),
		numClocks_(numClocks)
{
	static_assert(std::is_same< Transition, ValueType >::value,
				  "ERROR: type missmatch. ModuleInstance can only be copy-"
				  "constructed from a container with Transition objects");

	for(const auto& tr: transitions) {
		auto ptr = std::make_shared<Transition>(tr);
		transitions_by_label_[tr.label().str].emplace_back(ptr);
		transitions_by_clock_[tr.triggeringClock()].emplace_back(ptr);
	}
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
ModuleInstance::ModuleInstance(
	const unsigned& firstVar,
	const unsigned& numVars,
	const unsigned& firstClock,
	const unsigned& numClocks,
	Container< ValueType, OtherContainerArgs... >&& transitions) :
		firstVar_(firstVar),
		numVars_(numVars),
		firstClock_(firstClock),
		numClocks_(numClocks)
{
	static_assert(std::is_same< Transition, ValueType >::value,
				  "ERROR: type missmatch. ModuleInstance can only be move-"
				  "constructed from a container with instances or raw pointers "
				  "to Transition objects");

	for(const auto& tr: transitions) {
		auto ptr = std::make_shared<Transition>(tr);
		transitions_by_label_[tr.label().str].emplace_back(ptr);
		transitions_by_clock_[tr.triggeringClock()].emplace_back(ptr);
	}
	transitions.clear();
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
ModuleInstance::ModuleInstance(
	const unsigned& firstVar,
	const unsigned& numVars,
	const unsigned& firstClock,
	const unsigned& numClocks,
	Container< ValueType*, OtherContainerArgs... >&& transitions) :
		firstVar_(firstVar),
		numVars_(numVars),
		firstClock_(firstClock),
		numClocks_(numClocks)
{
	static_assert(std::is_same< Transition, ValueType >::value,
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


template< template< typename, typename... > class Iterator,
		  typename ValueType,
		  typename... OtherIteratorArgs >
ModuleInstance::ModuleInstance(
	const unsigned& firstVar,
	const unsigned& numVars,
	const unsigned& firstClock,
	const unsigned& numClocks,
	Iterator< ValueType, OtherIteratorArgs... > from,
	Iterator< ValueType, OtherIteratorArgs... > to) :
		firstVar_(firstVar),
		numVars_(numVars),
		firstClock_(firstClock),
		numClocks_(numClocks)
{
	static_assert(std::is_same< Transition, ValueType >::value,
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

