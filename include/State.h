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
#include <type_traits>  // std::is_same<>
#include <string>
#include <vector>
// C
#include <cassert>
// External code
#include <muParserDef.h>  // MUP_BASETYPE

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
 *        It is used for consitency check of State instances (see above)
 *        and for conversions between the concrete and symbolic representations
 *        of a system state.
 */
template< typename T_ >
class GlobalState
{
	// TODO: review whole, and build as singleton

	static_assert(std::is_integral<T_>::value,
				  "ERROR: class GlobalState<T> can only be instantiated "
				  "with integral types, e.g. int, short, unsigned.");
protected:

	size_t maxConcreteState_;
	shared_ptr< vector< Variable< T_ > > > vars_p;

protected:
	void build_concrete_bound();  // compute&store value of maxConcreteState_

public:  // Constructors XXX

	// Void ctor
	State();
	// Data ctor
	State(const vector< VarDec< T_ > >& vars);
	State(const vector< VarDef< T_ > >& vars);
	// Copy ctor
	State(const State< T_ >& that) noexcept;
	// Move ctor
	State(State< T_ >&& that) noexcept = default;
	// Copy assignment with copy&swap (no need for move assignment)
	State< T_ >& operator=(State< T_ > that) noexcept;
	// Dtor
	virtual ~State() {}

public:  // Accessors XXX

	// @brief Symbolic size, i.e. number of variables
	inline size_t size() const
		{ return nullptr == vars_p ? 0 : vars_p->size(); }

	// @brief Concrete size, i.e. cross product of all variables ranges
	inline size_t concrete_size() const
		{ return maxConcreteState_; }

	// @brief Retrieve i-th variable as const reference
	// @compl Constant
	inline const Variable< T_ >& operator[](size_t i) const
		{
			assert(nullptr != vars_p);
#		ifndef NDEBUG
			return const_cast<const Variable< T_ >& >(vars_p->at(i));
#		else
			return (*vars_p)[i];
#		endif
		}

	// @brief Retrieve i-th variable
	// @compl Constant
	inline Variable< T_ >& operator[](size_t i)
		{
			assert(nullptr != vars_p);
#		ifndef NDEBUG
			return vars_p->at(i);
#		else
			return (*vars_p)[i];
#		endif
		}

	// @brief Retrieve variable named "varname" if existent
	// @compl Linear on the size of State
	inline shared_ptr< Variable< T_ > > operator[](const string& varname)
		{
			for (auto& e: *vars_p)
				if (varname == e.name())
					return std::make_shared< Variable< T_ >>(e);
			return nullptr;
		}

	// @brief Print formatted vector of variables into 'out'
	inline void print_out(std::ostream& out) const
		{
			for (size_t i=0 ; i < size() ; i++)
				out << (*vars_p)[i].name() << "=" << (*vars_p)[i].val() << ", ";
			out << "\b\b  \b\b";
		}

	// @brief Encode current state (viz. vector of Variables values) as a number,
	//        i.e. the "concrete" representation of the current state.
	// @compl Quadratic on the size of State
	size_t encode_state() const;

	// @brief Decode number as vector of Variables values and apply to State,
	//        i.e. store "symbolically" the "concrete state" n.
	// @param n  Concrete state to interpret and apply to our symbolic existence
	// @compl Quadratic on the size of State
	void decode_state(const size_t& n);

	// @brief Decode number into corresponding Variable value
	// @param n  Concrete state to interpret
	// @param i  Variable index whose value (decoded from n) is requested
	// @compl Linear on the size of State
	T_ decode_state(const size_t& n, const size_t& i) const;

	// @brief Decode number into corresponding Variable value
	// @param n  Concrete state to interpret
	// @param i  Variable name whose value (decoded from n) is requested
	// @compl Linear on the size of State
	T_ decode_state(const size_t& n, const std::string& varname) const;

public:  // Relational operators XXX

	// @brief Tell whether 'this' and 'that' hold compatible/same variables
	// @compl Linear on the size of State
	bool compatible(const State< T_ >& that) const;
	bool operator==(const State< T_ >& that) const;
	inline bool operator!=(const State< T_ >& that) const
		{ return ( ! (that == *this) ); }
};

} // namespace fig

#endif // VARIABLE_H
