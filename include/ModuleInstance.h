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
#include <type_traits>  // std::is_constructible<>
// FIG
#include <Module.h>
#include <Transition.h>

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
 */
class ModuleInstance : public Module
{
	// Our range of variables
	/// Position in GlobalState of our first Variable
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
						const std::vector< std::shared_ptr< const Transition> > >
		transitions_by_clock_;
	// TODO: tell input/output apart here?
	std::unordered_map< Label,
						const std::vector< std::shared_ptr< const Transition> > >
		transitions_by_label_;

public:  // Ctors

	/**
	 * @brief Copy ctor from \ref Transition "transitions" container
	 *
	 * @param firstVar     Position in GlobalState of our first Variable
	 * @param numVars      Number of \ref Variable "variables" in this module
	 * @param firstClock   Position in global 'gClocks' of our first Clock
	 * @param numClocks    Number of \ref Clock "clocks" in this module
	 * @param transitions  Transitions defined in this module   // TODO always populate with std::make_shared() !!!
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	ModuleInstance(const unsigned& firstVar,
				   const unsigned& numVars,
				   const unsigned& firstClock,
				   const unsigned& numClocks,
				   const Container< ValueType, OtherContainerArgs... >& transitions);
	:
		firstVar_(firstVar),
		numVars_(numVars),
		firstClock_(numClocks),
		numClocks_(numClocks)
	{
		assert(0 < numVars_);
		assert(0 < numClocks_);
		assert(0 < transitions_.size());
		for (const auto& tp: transitions) {
			// Register by label   TODO: tell input/output apart?
			transitions_by_label_[tp->label].emplace_back(tp);
			// Register by clock (name)
			transitions_by_clock_[tp->triggeringClock].emplace_back(tp);
		}
	}


public:  // Utils

	virtual std::shared_ptr<const Label> jump(const std::string& clockName,
											  const CLOCK_INTERNAL_TYPE& elapsedTime,
											  Traial& traial) const;

	virtual void jump(const Label& label,
					  const CLOCK_INTERNAL_TYPE& elapsedTime,
					  Traial& traial) const;
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
		firstClock_(numClocks),
		numClocks_(numClocks)
{
	static_assert(std::is_constructible< Transition, ValueType >::value,
				  "ERROR: type missmatch. ModuleInstance copy ctor needs "
				  "a container with Transitions to copy");


	// TODO: fill this ctor and define the others
	//
	//       Take care to properly use move semantics whenever possible.
	//
	//       Also, we want both transitions_by_label_ and transitions_by_clock_
	//       to have shared_ptr to dynamically allocated transitions owned
	//       by us and referenced in those two maps.
	//
	//       It can be tricky to take ownership of transitions from the
	//       containers with which they created us, take care to do it right.
	//


	/*

	  Dummy test to take ownership of objects,
	  and keep stored in std::shared_ptr.


struct S1
{
	const std::string& str;

	S1(const char* ss) : str(ss) {}
	S1(const std::string& ss) : str(ss) {}
};

struct S2
{
	std::shared_ptr< S1 > S1_ptr;

	S2(S1* s1) : S1_ptr(std::shared_ptr<S1>(s1))
	{
		std::cout << "From raw ptr" << std::endl;
		s1 = nullptr;
	}

	S2(S1&& s1) : S1_ptr(std::move(std::make_shared<S1>(s1)))
	{
		std::cout << "Move ctor" << std::endl;
	}
};

int main()
{
	std::string str("s1");
//	S1* s1_ptr = new S1("s1");
//	S2 s2(s1_ptr);
	S2 s2(S1("s1"));
	assert(nullptr != s2.S1_ptr);
//	std::cout << (*(s2.S1_ptr)).str << std::endl;
	return 0;


	*/



	for (const auto& tp: transitions) {
		// Register by label   TODO: tell input/output apart?
		transitions_by_label_[tp->label].emplace_back(tp);
		// Register by clock (name)
		transitions_by_clock_[tp->triggeringClock].emplace_back(tp);
	}
}

} // namespace fig

#endif // MODULEINSTANCE_H

