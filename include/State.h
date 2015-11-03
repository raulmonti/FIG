//==============================================================================
//
//  State.h
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
//	along with PRISM; if not, write to the Free Software Foundation,
//	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//==============================================================================


#ifndef STATE_H
#define STATE_H

// C++
#include <type_traits>  // std::is_constructible<>, std::is_integral<>
#include <iterator>     // std::distance()
#include <utility>      // std::move()
#include <ostream>
#include <memory>       // std::shared_ptr<>
#include <string>
#include <vector>
// C
#include <cassert>
// External code
#include <muParserDef.h>  // MUP_BASETYPE
// Project code
#include <Variable.h>
#include <VariableInterval.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


namespace fig
{

// States internal storage type must match that of MuParser library
typedef  MUP_BASETYPE  STATE_INTERNAL_TYPE;

static_assert(std::is_same<short, MUP_BASETYPE>::value,
			  "Error: for now we restrict State internal storage to shorts");


/**
 * @brief State (instance): an array of Variable values.
 *        Each State is an instantiation of values, which follows the ordering
 *        given in the (unique) GlobalState vector of the system.
 *        A State can be compared to the GlobalState for consistency checks.
 *        There is a one-to-one correspondence between States and Traials.
 */
typedef std::vector< STATE_INTERNAL_TYPE > State;



/**
 * @brief Unique vector of Variables in the system (singleton)
 *        It is used for consistency check of State instances (see ::State)
 *        and for conversions between the concrete and symbolic representations
 *        of a system state.
 * @note  Offers generic construction from the following STL containers:
 *        vector, list, forward_list, set, unordered_set, deque.
 * @note  Will not build from the following STL containers:
 *        queue, stack, array.
 * @note  Generic construction is achieved through variadic template templates,
 *        see: http://eli.thegreenplace.net/2014/variadic-templates-in-c/
 */
template< typename T_ >
class GlobalState
{
	static_assert(std::is_integral<T_>::value,
				  "ERROR: class GlobalState<T> can only be instantiated "
				  "with integral types, e.g. int, short, unsigned.");

	/// Variables vector
	std::vector< std::shared_ptr< Variable< T_ > > > pvars_;

	/// Concrete size, i.e. cross product of all variables ranges
	size_t maxConcreteState_;

	/// @brief Compute and store value of maxConcreteState_
	void build_concrete_bound();

public:  // Ctors/Dtor

	// Void ctor
	inline GlobalState() : pvars_(), maxConcreteState_(0) {}
	// Data ctors
	/// Copy content from any container with proper internal data type
	template< template< typename, typename...> class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	GlobalState(const Container<ValueType, OtherContainerArgs...>& vars);
	/// Move content from any container with proper internal data type
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	GlobalState(Container<ValueType, OtherContainerArgs...>&& vars);
	/// Copy content between iterators 'from' and 'to' pointing to proper data type
	template< template< typename, typename... > class Iterator,
			  typename ValueType,
			  typename... OtherIteratorArgs >
	GlobalState(Iterator<ValueType, OtherIteratorArgs...> from,
				Iterator<ValueType, OtherIteratorArgs...> to);
	// Move ctor
	GlobalState(GlobalState<T_>&& that);

	// Reinforce uniqueness (no two GlobalVectors should be ever needed)
	GlobalState(const GlobalState<T_> &that)                = delete;
	GlobalState<T_>& operator=(const GlobalState<T_>& that) = delete;
	GlobalState<T_>& operator=(GlobalState<T_> that)        = delete;

	// Dtor
	virtual ~GlobalState() { pvars_.clear(); }

public:  // Accessors

	/// @brief Symbolic size, i.e. number of variables
	inline size_t size() const noexcept { return pvars_.size(); }

	/// @brief Concrete size, i.e. cross product of all variables ranges
	inline size_t concrete_size() const noexcept { return maxConcreteState_; }

	/**
	 * @brief Retrieve pointer to i-th variable (const or not)
	 * @note <b>Complexity:</b> <i>O(1)</i>
	 * @note Range check with throw only in DEBUG build
	 */
	inline std::shared_ptr< const Variable< T_ > >& operator[](const size_t& i) const
		{
#		ifndef NDEBUG
			return pvars_.at(i);
#		else
			return pvars_[i];
#		endif
		}
	inline std::shared_ptr< Variable< T_ > >& operator[](const size_t& i)
		{
#		ifndef NDEBUG
			return pvars_.at(i);
#		else
			return pvars_[i];
#		endif
		}

	/** 
	 * @brief Retrieve pointer to variable named "varname" if existent
	 * @return Pointer to variable "varname", or nullptr if not existent
	 * @note <b>Complexity:</b> <i>O(GlobalState.size())</i>
	 */
	std::shared_ptr< const Variable< T_ > > operator[](const std::string& varname) const;
	std::shared_ptr<       Variable< T_ > > operator[](const std::string& varname);

	/// @brief Print formatted vector of variables into 'out'
	void print_out(std::ostream& out, bool withNewline = false) const;

// public:  // Relational operators
//          // Not needed if there's a unique GlobalState instance
// 	/**
// 	 * @brief Whether 'this' and 'that' hold same variables with same values
// 	 * @note <b>Complexity:</b> <i>O(GlobalState.size()</i>
// 	 */
// 	bool operator==(const State< T_ >& that) const;
// 	inline bool operator!=(const State< T_ >& that) const
// 		{ return ( ! (that == *this) ); }

public:  // Encode/Decode between symbolic and concrete representations

	/**
	 * @brief Encode current state (a vector of Variables) as a number,
	 *        i.e. as the "concrete" representation of the current state.
	 * @note <b>Complexity:</b> <i>O(GlobalState.size()<sup>2</sup></i>
	 */
	size_t encode_state() const;

	/**
	 * @brief Decode number as vector of Variables values and apply to State,
	 *        i.e. store "symbolically" the "concrete state" n.
	 * @param n  Concrete state to interpret and apply to our symbolic existence
	 * @note <b>Complexity:</b> <i>O(GlobalState.size()<sup>2</sup></i>
	 */
	void decode_state(const size_t& n);

	/**
	 * @brief Decode concrete state 'n' into corresponding Variable value
	 * @param n  Concrete state to interpret
	 * @param i  Variable index whose value (decoded from n) is requested
	 * @note <b>Complexity:</b> <i>O(GlobalState.size())</i>
	 */
	T_ decode_state(const size_t& n, const size_t& i) const;

	/**
	 * @brief Decode concrete state 'n' into corresponding Variable value
	 * @param n  Concrete state to interpret
	 * @param i  Variable name whose value (decoded from n) is requested
	 * @note <b>Complexity:</b> <i>O(GlobalState.size())</i>
	 */
	T_ decode_state(const size_t& n, const std::string& varname) const;
};


// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< typename T_ >
template< template< typename, typename...> class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
GlobalState<T_>::GlobalState(const Container<ValueType, OtherContainerArgs...> &vars) :
	maxConcreteState_(1u)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_constructible< VariableInterval<T_>, ValueType >::value,
				  "ERROR: GlobalState can only be constructed from Variables, "
				  "VariableDefinitions or VariableDeclarations");
	for (const auto& e: vars)
		pvars_.emplace_back(std::make_shared< VariableInterval< T_ > >( e ));
	build_concrete_bound();
}


template< typename T_ >
template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
GlobalState<T_>::GlobalState(Container<ValueType, OtherContainerArgs...>&& vars) :
	maxConcreteState_(1u)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_convertible< VariableInterval<T_>, ValueType >::value,
				  "ERROR: GlobalState can only be move-constructed from "
				  "another GlobalState or a container with Variable pointers");
	for (auto& e: vars) {
		pvars_.emplace_back(std::move(
			   dynamic_cast< std::shared_ptr< VariableInterval< T_ > > >( e )));
		e = nullptr;
	}
	build_concrete_bound();
	vars.clear();
}


template< typename T_ >
template< template< typename, typename... > class Iterator,
		  typename ValueType,
		  typename... OtherIteratorArgs >
GlobalState<T_>::GlobalState(Iterator<ValueType, OtherIteratorArgs...> from,
							 Iterator<ValueType, OtherIteratorArgs...> to) :
	pvars_(std::distance(from,to)),
	maxConcreteState_(1u)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_constructible<VariableInterval<T_>, ValueType>::value,
				  "ERROR: GlobalState can only be constructed from Variables, "
				  "VariableDefinitions or VariableDeclarations");
	size_t i(0);
	do {
		pvars_[i++] = std::make_shared< VariableInterval<T_> >(*from);
	} while (++from != to);
	build_concrete_bound();
}


} // namespace fig

#endif // VARIABLE_H
