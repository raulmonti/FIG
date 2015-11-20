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
//	along with FIG; if not, write to the Free Software Foundation,
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
#include <unordered_map>
// C
#include <cassert>
// FIG
#include <core_typedefs.h>
#include <Variable.h>
#include <VariableInterval.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


namespace fig
{

static_assert(std::is_same<short, STATE_INTERNAL_TYPE>::value,
			  "Error: for now we restrict states internal storage to shorts");

/**
 * @brief Assignment of values to the Variables (a logical <i>valuation</i>)
 *
 *        This is an instantiation of values, which follows the ordering
 *        given in the (unique) State of the system.
 *        A StateInstance can be compared to the State for consistency
 *        checks. There is a one-to-one correspondence between StateInstance
 *        and Traial objects.
 */
typedef std::vector< STATE_INTERNAL_TYPE > StateInstance;


/**
 * @brief Unique vector of Variables in the system
 *
 *        State is used for consistency check of StateInstance instances
 *        (see fig::State) and for conversions between the concrete and symbolic
 *        representations of a system state.
 *
 * @note  Offers generic construction from the following STL containers:
 *        vector, list, forward_list, set, unordered_set, deque.
 * @note  Will not build from the following STL containers:
 *        queue, stack, array.
 * @note  Generic construction is achieved through variadic template templates,
 *        see: http://eli.thegreenplace.net/2014/variadic-templates-in-c/
 */
template< typename T_ >
class State
{
	/// @cond DEV
	// Since we implement with VariableInterval<T_> for the time being,
	// we forward here its type restrictions.
	static_assert(std::is_integral<T_>::value,
				  "ERROR: class State<T> can only be instantiated "
				  "with integral types, e.g. int, short, unsigned.");
	/// @endcond

	/// Variables vector
	std::vector< std::shared_ptr< Variable< T_ > > > pvars_;

	/// Concrete size, i.e. cross product of all variables ranges
	size_t maxConcreteState_;

	/// Lookup { varname --> varpos } needed by MathExpressions
	std::unordered_map<std::string, size_t> positionOfVar_;  // http://stackoverflow.com/a/13799886

	/// @brief Compute and store value of State::maxConcreteState_
	void build_concrete_bound();

public:  // Ctors/Dtor

	// Void ctor
	inline State() : pvars_(), maxConcreteState_(0) {}

	// Data ctors
	/// Copy content from any container with proper internal data type
	template< template< typename, typename...> class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	State(const Container<ValueType, OtherContainerArgs...>& vars);

	/// Move content from any container with proper internal data type
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	State(Container<ValueType, OtherContainerArgs...>&& vars);

	/// Move content from any container with pointers to proper internal data type
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	State(Container<ValueType*, OtherContainerArgs...>&& vars);

	/// Copy content between iterators 'from' and 'to' pointing to proper data type
	template< template< typename, typename... > class Iterator,
			  typename ValueType,
			  typename... OtherIteratorArgs >
	State(Iterator<ValueType, OtherIteratorArgs...> from,
				Iterator<ValueType, OtherIteratorArgs...> to);

	// Move ctor
	State(State<T_>&& that);

	// Reinforce uniqueness (no two GlobalVectors should be ever needed)
	State(const State<T_> &that)                = delete;
	State<T_>& operator=(const State<T_>& that) = delete;
	State<T_>& operator=(State<T_> that)        = delete;

	// Dtor
	virtual ~State() { pvars_.clear(); positionOfVar_.clear(); }

public:  // Accessors

	/// @brief Symbolic size, i.e. number of variables
	inline size_t size() const noexcept { return pvars_.size(); }

	/// @brief Concrete size, i.e. cross product of all variables ranges
	inline size_t concrete_size() const noexcept { return maxConcreteState_; }

	/**
	 * @brief Retrieve pointer to i-th variable (const or not)
	 * @note <b>Complexity:</b> <i>O(1)</i>
	 * @throw out_of_range if NRANGECHK is not defined and 'i' is out of range
	 */
	inline std::shared_ptr< const Variable< T_ > > operator[](const size_t& i) const
		{
#		ifndef NRANGECHK
			return pvars_.at(i);
#		else
			return pvars_[i];
#		endif
		}
	inline std::shared_ptr< Variable< T_ > >& operator[](const size_t& i)
		{
#		ifndef NRANGECHK
			return pvars_.at(i);
#		else
			return pvars_[i];
#		endif
		}

	/** 
	 * @brief Retrieve pointer to variable named "varname" if existent
	 * @return Pointer to variable "varname", or nullptr if not existent
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 */
	std::shared_ptr< const Variable< T_ > > operator[](const std::string& varname) const;
	std::shared_ptr<       Variable< T_ > > operator[](const std::string& varname);

	/// begin() for range iteration
	typename std::vector< std::shared_ptr< Variable<T_> > >::const_iterator
	begin() const
		{ return pvars_.begin(); }

	/// end() for range iteration
	typename std::vector< std::shared_ptr< Variable<T_> > >::const_iterator
	end() const
		{ return pvars_.end(); }

	/**
	 * @brief Retrieve position of variable named "varname" if existent
	 * @note  <b>Complexity:</b> average case is <i>O(1)</i>,
	 *        worst case (rare) is <i>O(size())</i>
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if variable "varname" doesn't exist
	 * \endif
	 */
	inline size_t position_of_var(const std::string& varname) const
		{
#ifndef NRANGECHK
			return positionOfVar_.at(varname);
#else
			return positionOfVar_[varname];  // creates location if inexistent!
#endif
		}

	/// @brief Print formatted vector of variables into 'out'
	void print_out(std::ostream& out, bool withNewline = false) const;

public:  // Relational operators
		 // FIXME Not needed if there's a unique State instance
	/**
	 * @brief Whether 'this' and 'that' hold same variables with same values
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 */
	bool operator==(const State< T_ >& that) const;
	inline bool operator!=(const State< T_ >& that) const
		{ return ( ! (that == *this) ); }

public:  // Interaction with ::State instances

	/**
	 * @brief Are the values in 's' valid w.r.t. us?
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 */
	bool is_valid_state_instance(StateInstance s) const;

	/**
	 * @brief Copy values for our Variables from the ::State instance 's'.
	 *        Optionally check for validity of 's' beforehand.
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 * @throw FigException if checking validity and invalid value found
	 */
	void copy_from_state_instance(const StateInstance& s, bool checkValidity = false);

	/**
	 * @brief Copy our Variables values to the ::State instance 's'.
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 */
	void copy_to_state_instance(StateInstance& s) const;

	/**
	 * @brief Create a fresh ::State instance reflecting our Variables values
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 */
	std::unique_ptr< StateInstance > to_state_instance() const;

public:  // Encode/Decode between symbolic and concrete representations

	/**
	 * @brief Encode current state (a vector of Variables) as a number,
	 *        i.e. as the "concrete" representation of the current state.
	 * @note <b>Complexity:</b> <i>O(size()<sup>2</sup>)</i>
	 */
	size_t encode_state() const;

	/**
	 * @brief Decode number as vector of Variables values and apply to StateInstance,
	 *        i.e. store "symbolically" the "concrete state" n.
	 * @param n  Concrete state to interpret and apply to our symbolic existence
	 * @note <b>Complexity:</b> <i>O(size()<sup>2</sup>)</i>
	 */
	void decode_state(const size_t& n);

	/**
	 * @brief Decode concrete state 'n' into corresponding Variable value
	 * @param n  Concrete state to interpret
	 * @param i  Variable index whose value (decoded from n) is requested
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 */
	T_ decode_state(const size_t& n, const size_t& i) const;

	/**
	 * @brief Decode concrete state 'n' into corresponding Variable value
	 * @param n  Concrete state to interpret
	 * @param i  Variable name whose value (decoded from n) is requested
	 * @note <b>Complexity:</b> <i>O(size())</i>
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
State<T_>::State(const Container<ValueType, OtherContainerArgs...>& vars) :
	maxConcreteState_(1u)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_constructible< VariableInterval<T_>, ValueType >::value,
				  "ERROR: type missmatch. State can only be constructed "
				  "from Variables, VariableDefinitions or VariableDeclarations");
	size_t i(0u);
	auto last = positionOfVar_.begin();
	for (const auto& e: vars) {
		pvars_.emplace_back(std::make_shared< VariableInterval< T_ > >( e ));
		last = positionOfVar_.emplace_hint(last, pvars_[i]->name_, i);
		++i;
	}
	build_concrete_bound();
}


// This is the move ctor from object references
template< typename T_ >
template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
State<T_>::State(Container<ValueType, OtherContainerArgs...>&& vars) :
	maxConcreteState_(1u)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_convertible< VariableInterval<T_>, ValueType >::value,
				  "ERROR: type missmatch. State can only be move-"
				  "constructed from another State or from a container "
				  "with instances or raw pointers to VariableInterval objects");
	size_t i(0u);
	auto last = positionOfVar_.begin();
	for (auto& e: vars) {
		pvars_.emplace_back( std::make_shared< VariableInterval<T_> >( std::move( e ) ) );
		last = positionOfVar_.emplace_hint(last, pvars_[i]->name_, i);
		++i;
	}
	build_concrete_bound();
	vars.clear();
}


// This is the move ctor from object (raw) pointers
template< typename T_ >
template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
State<T_>::State(Container<ValueType*, OtherContainerArgs...>&& vars) :
	maxConcreteState_(1u)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_convertible< VariableInterval<T_>, ValueType >::value,
				  "ERROR: type missmatch. State can only be move-"
				  "constructed from another State or from a container "
				  "with instances or raw pointers to VariableInterval objects");
	size_t i(0u);
	auto last = positionOfVar_.begin();
	for (auto& e: vars) {
		pvars_.emplace_back(std::shared_ptr< VariableInterval< T_ > >( e ));
		last = positionOfVar_.emplace_hint(last, pvars_[i]->name_, i);
		e = nullptr;
		++i;
	}
	build_concrete_bound();
	vars.clear();
}


template< typename T_ >
template< template< typename, typename... > class Iterator,
		  typename ValueType,
		  typename... OtherIteratorArgs >
State<T_>::State(Iterator<ValueType, OtherIteratorArgs...> from,
							 Iterator<ValueType, OtherIteratorArgs...> to) :
	pvars_(std::distance(from,to)),
	maxConcreteState_(1u)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_constructible<VariableInterval<T_>, ValueType>::value,
				  "ERROR: type missmatch. State can only be constructed "
				  "from Variables, VariableDefinitions or VariableDeclarations");
	size_t i(0);
	auto last = positionOfVar_.begin();
	do {
		pvars_[i] = std::make_shared< VariableInterval<T_> >(*from);
		last = positionOfVar_.emplace_hint(last, pvars_[i]->name_, i);
		++i;
	} while (++from != to);
	build_concrete_bound();
}


} // namespace fig

#endif // VARIABLE_H
