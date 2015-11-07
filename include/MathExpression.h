//==============================================================================
//
//  MathExpression.h
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


#ifndef MATHEXPRESSION_H
#define MATHEXPRESSION_H

// C++
#include <type_traits>  // std::is_constructible<>
#include <iterator>     // std::distance()
#include <utility>      // std::pair<>, std::move()
#include <string>
// External code
#include <muParser.h>
// Project code
#include <State.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


namespace fig
{

extern GlobalState< STATE_INTERNAL_TYPE > gState;

/**
 * @brief Mathematical expression with variables mapping
 *
 *        This class assumes a GlobalState variable named 'gState'
 *        was defined somewhere within the fig namespace.
 *        Such instance is needed for defining a unique order of the
 *        mapped variables for all objects of this class.
 *
 * @note  Uses MuParser library (http://muparser.beltoforion.de/)
 * @note  Offers generic construction from the following STL containers:
 *        vector, list, forward_list, set, unordered_set, deque.
 * @note  Will not build from the following STL containers:
 *        queue, stack, array.
 */
class MathExpression
{
protected:

	typedef mu::Parser Expression;

	Expression expr_;

	const std::string& exprStr_;

	/// Names and positions of the variables in our expression.
	/// The positional order is given by the GlobalState of the system.
	std::vector< std::pair< const std::string, size_t > > varsMap_;

public:  // Ctors

	/**
	 * @brief Data ctor from generic lvalue container
	 * @param exprStr   String with the matemathical expression to evaluate
	 * @param varnames  Container with names of variables ocurring in exprStr
	 * @throw out_of_range if NRANGECHK is not defined and 'varnames' contains
	 *        some variable name not appearing in the system GlobalState
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	MathExpression(const std::string& exprStr,
				   const Container<ValueType, OtherContainerArgs...>& varnames);

	/**
	 * @brief Data ctor from generic rvalue container
	 * @param exprStr   String with the matemathical expression to evaluate
	 * @param varnames  Container with names of variables ocurring in exprStr
	 * @throw out_of_range if NRANGECHK is not defined and 'varnames' contains
	 *        some variable name not appearing in the system GlobalState
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	MathExpression(const std::string& exprStr,
				   Container<ValueType, OtherContainerArgs...>&& varnames);

	/**
	 * @brief Data ctor from iterator range
	 * @param exprStr  String with the matemathical expression to evaluate
	 * @param from     Iterator to first  name of variables ocurring in exprStr
	 * @param to       Iterator past last name of variables ocurring in exprStr
	 * @throw out_of_range if NRANGECHK is not defined and there is some
	 *        variable name not appearing in the system GlobalState
	 */
	template< template< typename, typename... > class Iterator,
			  typename ValueType,
			  typename... OtherIteratorArgs >
	MathExpression(const std::string& exprStr,
				   Iterator<ValueType, OtherIteratorArgs...> from,
				   Iterator<ValueType, OtherIteratorArgs...> to);

public:  // Accessors

	inline const std::string& expression() const { return exprStr_; }

private:  // Class utils

	/**
	 * @brief Set 'exprStr_' as the expression to MuParser's 'expr_'
	 * @details After successfully parsing the expression string,
	 *          all offered builtin functions are bound to 'expr_'
	 * @throw FigException if badly formatted mathematical expression
	 */
	void parse_our_expression();
};


// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
MathExpression::MathExpression(
	const std::string& exprStr,
	const Container<ValueType, OtherContainerArgs...>& varnames) :
		exprStr_(exprStr)
{
	static_assert(std::is_constructible< std::string, ValueType >::value,
				  "ERROR: MathExpression needs a container with variable names");
	// Setup MuParser expression
	parse_our_expression();
	// Setup variables mapping
	for (const auto& name: varnames) {
		assert(std::string::npos != exprStr.find(name));  // trust no one
		varsMap_.emplace_back(std::make_pair(name, gState.position_of_var(name)));
	}
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
MathExpression::MathExpression(
	const std::string& exprStr,
	Container<ValueType, OtherContainerArgs...>&& varnames) :
		exprStr_(exprStr)
{
	static_assert(std::is_constructible< std::string, ValueType >::value,
				  "ERROR: MathExpression needs a container with variable names");
	// Setup MuParser expression
	parse_our_expression();
	// Setup variables mapping
	for (auto& name: varnames) {
		assert(std::string::npos != exprStr.find(name));  // trust no one
		varsMap_.emplace_back(std::make_pair(std::move(name),
											 gState.position_of_var(name)));
	}
	varnames.clear();
}


template< template< typename, typename... > class Iterator,
		  typename ValueType,
		  typename... OtherIteratorArgs >
MathExpression::MathExpression(
	const std::string& exprStr,
	Iterator<ValueType, OtherIteratorArgs...> from,
	Iterator<ValueType, OtherIteratorArgs...> to) :
		exprStr_(exprStr),
		varsMap_(std::distance(from,to))
{
	static_assert(std::is_constructible< std::string, ValueType >::value,
				  "ERROR: MathExpression needs iterators pointing to variable names");
	// Setup MuParser expression
	parse_our_expression();
	// Setup variables mapping
	size_t i(0u);
	do {
		std::string name = *from;
		assert(std::string::npos != exprStr.find(name));  // trust no one
		varsMap_[i++] = std::make_pair(std::move(name),
									   gState.position_of_var(name));
	} while (++from != to);
}

} // namespace fig

#endif // MATHEXPRESSION_H
