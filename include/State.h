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
// Implemented support for arrays: Leonardo Rodríguez.

#ifndef STATE_H
#define STATE_H

// C
#include <cassert>
// C++
#include <type_traits>  // std::is_constructible<>, std::is_integral<>
#include <algorithm>    // std::find_if()
#include <iterator>     // std::distance()
#include <utility>      // std::move()
#include <ostream>
#include <memory>       // std::shared_ptr<>
#include <string>
#include <vector>
#include <unordered_map>
// External code
#include <uint128_t.h>  // uint128::uint128_1
// FIG
#include <core_typedefs.h>
#include <Variable.h>
#include <VariableInterval.h>

// ADL
using std::begin;
using std::end;

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


namespace fig
{

/**
 * @brief Set of \ref Variable "variables" managed by a Module
 *
 *        This TAD is mostly used for consistency check of the Traials'
 *        \ref StateInstance "state instances", and for conversions between
 *        the concrete and symbolic representations of a system state.
 *
 * @note  Offers generic construction from the following STL containers:
 *        vector, list, forward_list, set, unordered_set, deque.
 * @note  Will not build from the following STL containers:
 *        queue, stack, array.
 * @note  Generic construction is achieved through
 *        <a href="http://eli.thegreenplace.net/2014/variadic-templates-in-c/">
 *        variadic template templates</a>.
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
	uint128_t maxConcreteState_;

#ifndef NRANGECHK
	/// Lookup { varname --> varpos }
	std::unordered_map<std::string, size_t> positionOfVar_;  // http://stackoverflow.com/a/13799886
#else
	/// Lookup { varname --> varpos }
	mutable std::unordered_map<std::string, size_t> positionOfVar_;
#endif

    /// Lookup { arrayname -> (first var position, size) }
    typedef std::unordered_map<std::string, std::pair<size_t,size_t>> ArrayMap;
    mutable ArrayMap arrayData_;

public:  // Ctors/Dtor

	// Void ctor
	inline State() : pvars_(), maxConcreteState_(uint128::uint128_1) {}

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

	/// Copy ctor
	/// @note Deep copy by default: variables are duplicated
	/// @see shallow_copy()
	State(const State<T_>& that);

	/// Move ctor
	State(State<T_>&& that);

	/// Copy assignment with copy&swap
	State<T_>& operator=(State<T_> that);

	/// Dtor
	virtual ~State() { pvars_.clear(); positionOfVar_.clear(); }

public:  // Modifyers

	/**
	 * @brief Append variables of 'tail', effectively increasing our size
	 *        by tail.size()
	 * \ifnot NDEBUG
	 *   @throw FigException if some variable in 'tail' already existed in this state
	 * \endif
	 */
	void append(const State<T_>& tail);

    /** @brief Same as \ref append except that the state to be appended
     *  is considered as the description of an array. Each variable on
     *  the state is a position of the array.
     *  @param name of the array
     *  @param array must have the form ["name[0]" -> v0, ..., "name[N]" -> vN]
     */
    void append_array(const std::string &name, const State<T_> &array);

    /** @brief Update an array position with the given value
     * @param name of the array
     * @param position of the array to be updated
     */
    void update_array(const std::string &name, size_t pos, T_ value);

	/**
	 * @brief Copy values for our Variables from passed State
	 * @details Notice 'that' should contain a superset of our variables
	 * @param that State to copy our variables' values from
	 * @note <b>Complexity:</b> <i>O(size()*size(that))</i>
	 * @throw FigException if 'that' doesn't hold values for all our variables
	 */
	void extract_valuation_from(const State<T_>& that);

public:  // Accessors

	/// Symbolic size, i.e. number of variables
	inline size_t size() const noexcept { return pvars_.size(); }

	/// Does the state hold any variable at all?
	inline bool empty() const noexcept { return pvars_.empty(); }

	/// Concrete size, i.e. cross product of all variables ranges
	inline uint128_t concrete_size() const noexcept { return maxConcreteState_; }

	/// Names of the variables in creation order
	/// @note Calls to append() modify the value returned by this function
	std::vector< std::string > varnames() const noexcept;

	/// Make shallow copy from 'that', i.e. share its variables through pointers
	/// @note 'that' isn't modified, but it's not const qualified since
	///       future changes to 'this' will alter the values in 'that'
	/// @see State(const State&)
	void shallow_copy(State<T_>& that);

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
		{ return std::begin(pvars_); }

	/// end() for range iteration
	typename std::vector< std::shared_ptr< Variable<T_> > >::const_iterator
	end() const
		{ return std::end(pvars_); }

	/**
	 * @brief Retrieve position of variable named "varname" if existent
	 * @note  <b>Complexity:</b> average case is <i>O(1)</i>,
	 *        worst case (rare) is <i>O(size())</i>
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if variable "varname" doesn't exist
	 * \endif
	 */
    size_t position_of_var(const std::string& varname) const;


    /** @brief Position of the first element of the array with
     * the given name.
     * @param name of the array
     */
    size_t position_of_array_fst(const std::string &name) const;

    /** @brief Size of the array with the given name.
     * @param name of the array
     */
    size_t array_size(const std::string &name) const;

	/// @brief Value of the array in the given position.
    T_ array_value(const std::string &name, size_t position) const;

	/**
	 * @brief Return formatted string with a printout of the state's variables
	 * @param condensed  Whether to print a summarized version of the data
	 * @warning Currently ommits array info
	 */
	std::string to_string(bool condensed = true) const;

	/**
	 * @brief Print formatted vector of variables into 'out'
	 * @param out        Output stream where printing will take place
	 * @param condensed  Whether to print a summarized version of the data
	 * @see to_string()
	 */
	void print_out(std::ostream& out, bool condensed = false) const;

public:  // Relational operators
	/**
	 * @brief Whether 'this' and 'that' hold same variables with same values
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 */
	bool operator==(const State< T_ >& that) const;
	inline bool operator!=(const State< T_ >& that) const
		{ return ( ! (that == *this) ); }

public:  // Interaction with state instances

	/**
	 * @brief Are the values in 's' valid w.r.t. us?
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 */
	bool is_valid_state_instance(StateInstance s) const;

	/**
	 * @brief Copy values for our Variables from passed StateInstance
	 *        starting from its position 'ipos'.
	 *        Optionally check for validity of 's' beforehand.
	 * @param s    StateInstance to copy values from
	 * @param ipos Initial position of 's' from which the values will be extracted
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 * @throw FigException if s.size() < ipos + size()
	 * @throw FigException if checking validity and invalid value found
	 */
	void extract_from_state_instance(const StateInstance& s,
									 const size_t& ipos = 0ul,
									 bool checkValidity = false);

	/**
	 * @brief Copy values for our Variables from passed StateInstance.
	 *        Optionally check for validity of 's' beforehand.
	 * @param s StateInstance, same size as this State, to copy all values from
	 * @return Reference to self after a successfull copy
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 * @throw FigException if StateInstance with different size passed
	 * @throw FigException if checking validity and invalid value found
	 */
	State<T_>&
	copy_from_state_instance(const StateInstance& s, bool checkValidity = false);

	/**
	 * @brief Copy our \ref Variable "variables" values to the StateInstance 's'
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 */
	void copy_to_state_instance(StateInstance& s) const;

	/**
	 * @brief Get a StateInstance reflecting our \ref Variable "variables" values
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 */
	StateInstance to_state_instance() const;

public:  // Encode/Decode between symbolic and concrete representations

	/**
	 * @brief Encode current state (a vector of Variables) as a number,
	 *        i.e. as the "concrete" representation of the current state.
	 * @note <b>Complexity:</b> <i>O(size()<sup>2</sup>)</i>
	 */
	size_t encode() const;

	/**
	 * @brief Decode \a n as vector of Variables values and apply to self,
	 *        i.e. store <i>symbolically</i> the <i>concrete state</i> \a n.
	 * @param n  Concrete state to interpret and apply to our symbolic existence
	 * @return Self with the new decoded valuation
	 * @note <b>Complexity:</b> <i>O(size()<sup>2</sup>)</i>
	 */
	const State<T_>& decode(const size_t& n);

	/// @copydoc decode
	const State<T_>& decode(const uint128_t& n);

	/**
	 * @brief Decode concrete state 'n' into corresponding Variable value
	 * @param n  Concrete state to interpret
	 * @param i  Variable index whose value (decoded from n) is requested
	 * @return Value of the i-th variable decoded from concrete state 'n'
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 */
	T_ decode(const size_t& n, const size_t& i) const;

	/**
	 * @brief Decode concrete state 'n' into corresponding Variable value
	 * @param n  Concrete state to interpret
	 * @param i  Variable name whose value (decoded from n) is requested
	 * @return Value of the variable decoded from concrete state 'n'
	 * @note <b>Complexity:</b> <i>O(size())</i>
	 */
	T_ decode(const size_t& n, const std::string& varname) const;

private:  // Utils

	/// Compute and store value of maxConcreteState_
	void build_concrete_bound();

	/// Do we have a variable with such name?
	bool is_our_var(const std::string& varName);

public: // Debug
    void print_info(std::ostream &out) const;
};


// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< typename T_ >
template< template< typename, typename...> class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
State<T_>::State(const Container<ValueType, OtherContainerArgs...>& vars) :
	maxConcreteState_(uint128::uint128_1)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_constructible< VariableInterval<T_>, ValueType >::value,
				  "ERROR: type mismatch. State can only be constructed "
				  "from Variables, VariableDefinitions or VariableDeclarations");
	size_t i(0u);
	auto last = ::begin(positionOfVar_);
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
	maxConcreteState_(uint128::uint128_1)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_constructible< VariableInterval<T_>, ValueType >::value,
				  "ERROR: type mismatch. State can only be move-"
				  "constructed from another State or from a container "
				  "with instances or raw pointers to VariableInterval objects");
	size_t i(0u);
	auto last = ::begin(positionOfVar_);
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
	maxConcreteState_(uint128::uint128_1)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_constructible< VariableInterval<T_>, ValueType >::value,
				  "ERROR: type mismatch. State can only be move-"
				  "constructed from another State or from a container "
				  "with instances or raw pointers to VariableInterval objects");
	size_t i(0u);
	auto last = ::begin(positionOfVar_);
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
	maxConcreteState_(uint128::uint128_1)
{
	// We chose VariableInterval<> as implementation for our Variables
	static_assert(std::is_constructible<VariableInterval<T_>, ValueType>::value,
				  "ERROR: type mismatch. State can only be constructed "
				  "from Variables, VariableDefinitions or VariableDeclarations");
	size_t i(0);
	auto last = ::begin(positionOfVar_);
	do {
		pvars_[i] = std::make_shared< VariableInterval<T_> >(*from);
		last = positionOfVar_.emplace_hint(last, pvars_[i]->name_, i);
		++i;
	} while (++from != to);
	build_concrete_bound();
}

} // namespace fig

#endif // VARIABLE_H
