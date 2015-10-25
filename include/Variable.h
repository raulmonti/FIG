//==============================================================================
//
//  Variable.h
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


#ifndef VARIABLE_H
#define VARIABLE_H

// C++
#include <type_traits>  // std::is_integral<>
#include <string>
#include <vector>
#include <tuple>
// C
#include <cassert>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


namespace fig
{

/**
 * @brief Most abstract Variable concept, the one to populate the (unique)
 *        global vector of Variables in the system.
 * @note  This class exists only for value validation of State instances.
 */
template< typename T_ >
struct Variable
{
	// Friend template class: http://stackoverflow.com/a/8967610
	template< typename TT_ > friend class GlobalState;

public:  // Constructors

	Variable(const std::string& thename);
	Variable(std::string&& thename);
	virtual ~Variable() {}

public:  // Accessors

	inline size_t range() const noexcept { return range_; }
	virtual T_ min() const noexcept = 0;
	virtual T_ max() const noexcept = 0;
	virtual T_ val() const noexcept = 0;

public:  // Relational operators

	/// @note Also compares "current value", i.e. offset_
	virtual bool operator==(const Variable<T_>& that) const = 0;
	inline  bool operator!=(const Variable<T_>& that) const { return !(*this == that);}
	/// @brief Tell whether 'val' is a valid value for this Variable
	virtual bool is_valid_value(const T_& val) const = 0;

public:  // Attributes

	const std::string name;  // FIXME remove const qualifier and implement fresh variables ???

protected:
	/// Number of distinct values this variable can take
	size_t range_;
	/// Position in [0, 1, ..., range) for the "current" value of the Variable
	size_t offset_;

public:  // Invariant

#ifndef NDEBUG
	inline virtual void assert_invariant() const
		{
			assert(!name.empty());
			assert(offset_ < range_);
		}
#else
	inline void assert_invariant() const {}
#endif
};


/// Variable declaration: name, min, max
template< typename T_ > using VariableDeclaration =
	std::tuple< std::string, T_, T_ >;


/// Variable definition: name, min, max, initial value
template< typename T_ > using VariableDefinition =
	std::tuple< std::string, T_, T_, T_ >;


/**
 * @brief Variable defined by the closed interval [ min_value , max_value ]
 */
template< typename T_ >
class VariableInterval : Variable< T_ >
{
	static_assert(std::is_integral<T_>::value,
				  "ERROR: class VariableInterval<T> can only be instantiated "
				  "with integral types, e.g. int, short, unsigned.");
	const T_ min_;
	const T_ max_;

public:  // Ctors

	// It should be safe to use compiler's both move and copy ctors
	// VariableInterval(const VariableInterval& that);
	// VariableInterval(VariableInterval&& that);

	// A bunch of data ctors
	VariableInterval(const std::string& thename, const T_& min, const T_& max);
	VariableInterval(const std::string& thename, const T_& min, const T_& max, const T_& val);
	VariableInterval(std::string&& thename, const T_& min, const T_& max);
	VariableInterval(std::string&& thename, const T_& min, const T_& max, const T_& val);
	VariableInterval(const VariableDeclaration< T_ >& dec);
	VariableInterval(const VariableDefinition< T_ >& def);
	VariableInterval(VariableDeclaration< T_ >&& dec);
	VariableInterval(VariableDefinition< T_ >&& def);

	virtual ~VariableInterval() {}

public:  // Accessors

	inline T_ min() const noexcept { return min_; }
	inline T_ max() const noexcept { return max_; }
	inline T_ val() const noexcept { return min_ + offset_; }

public:  // Relational operators

	virtual bool operator==(const Variable<T_>& that) const;
	inline virtual bool is_valid_value(const T_& val) const final
		{ return min_ <= val && val <= max_; }  // http://stackoverflow.com/a/19954164

public:  // Invariant

#ifndef NDEBUG
	inline virtual void assert_invariant() const
		{
			Variable::assert_invariant();
			assert(min_ <= max_);
		}
#else
	inline void assert_invariant() const {}
#endif
};


/**
 * Variable defined by a set of possible values: { val1, val2, ..., valN }
 */
template< typename T_ >
class VariableSet : Variable<T_>
{
	std::vector< T_ > values;
	T_ min_;
	T_ max_;

public:  // Ctors/Dtor

	/// TODO
	//  Implement constructors

	virtual ~VariableSet() { values.clear(); }

public:  // Accessors

	inline T_ min() const noexcept { return min_; }
	inline T_ max() const noexcept { return max_; }
	inline T_ val() const noexcept { return values[offset_]; }

public:  // Relational operators

	virtual bool operator==(const Variable<T_>& that) const;
	virtual bool is_valid_value(const T_& val) const;

public:  // Invariant

#ifndef NDEBUG
	virtual void assert_invariant() const;
#else
	inline void assert_invariant() const {}
#endif
};

} // namespace fig

#endif // VARIABLE_H
