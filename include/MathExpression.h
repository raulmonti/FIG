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
#include <stdexcept>    // std::out_of_range
#include <functional>
// External code
#include <muParser.h>
// FIG
#include <State.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


namespace fig
{

/**
 * @brief Mathematical expression with variables mapping
 *
 *        A mathematical expression is built from an expression string using
 *        the <a href="http://muparser.beltoforion.de/">MuParser library</a>.
 *        It requires a separate explicit specification of which literals
 *        within that expression refer to variables names.
 *
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

	std::string exprStr_;

	/// Names and positions of the variables in our expression.
	/// The positional order is ("later") given by the global system State.
	std::vector< std::pair< std::string, int > > varsMap_;

public:  // Ctors/Dtor

	/**
	 * @brief Data ctor from generic lvalue container
	 *
	 * @param exprStr   String with the matemathical expression to evaluate
	 * @param varnames  Container with names of variables ocurring in exprStr
	 *
	 * @throw FigException if exprStr doesn't define a valid expression
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if names of variables not appearing
	 *                       in our expression were passed
	 * \endif
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	MathExpression(const std::string& exprStr,
				   const Container<ValueType, OtherContainerArgs...>& varnames);

	/**
	 * @brief Data ctor from generic rvalue container
	 *
	 * @param exprStr   String with the matemathical expression to evaluate
	 * @param varnames  Container with names of variables ocurring in exprStr
	 *
	 * @throw FigException if exprStr doesn't define a valid expression
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if names of variables not appearing
	 *                       in our expression were passed
	 * \endif
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	MathExpression(const std::string& exprStr,
				   Container<ValueType, OtherContainerArgs...>&& varnames);

	/**
	 * @brief Data ctor from iterator range
	 *
	 * @param exprStr  String with the matemathical expression to evaluate
	 * @param from     Iterator to  first name of variables ocurring in exprStr
	 * @param to       Iterator past last name of variables ocurring in exprStr
	 *
	 * @throw FigException if exprStr doesn't define a valid expression
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if names of variables not appearing
	 *                       in our expression were passed
	 * \endif
	 */
	template< template< typename, typename... > class Iterator,
			  typename ValueType,
			  typename... OtherIteratorArgs >
	MathExpression(const std::string& exprStr,
				   Iterator<ValueType, OtherIteratorArgs...> from,
				   Iterator<ValueType, OtherIteratorArgs...> to);

	inline virtual ~MathExpression() { varsMap_.clear(); expr_.~Parser(); }

protected:  // Modifyers

	/**
	 * @brief Register the global-system-state position of our variables
	 *
	 * @param posOfVar    Member function of State which given a variable name
	 *                    returns the position where this resides internally
	 * @param globalState Global state instance, owner of the member function
	 *
	 * @warning Intended as callback to be called <b>exactly once</b>
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if some of our variables isn't mapped
	 * \endif
	 */
	inline void pin_up_vars(std::function<size_t(const fig::State<STATE_INTERNAL_TYPE>&,
												 const std::string&)
										 > posOfVar,
							const fig::State<STATE_INTERNAL_TYPE>& globalState)
		{
			for(auto& pair: varsMap_)
				pair.second = posOfVar(globalState, pair.first);
		}

	/**
	 * @brief Register the global-system-state position of our variables
	 *
	 * @param globalVars Mapping of our variables names to their
	 *                   respective positions in a global array
	 *
	 * @warning Intended as callback to be called <b>exactly once</b>
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if some of our variables isn't mapped
	 * \endif
	 */
	inline void pin_up_vars(const PositionsMap& globalVars)
		{
			for(auto& pair: varsMap_)
#ifndef NRANGECHK
				pair.second = globalVars.at(pair.first);
#else
				pair.second = globalVars[pair.first];
#endif
		}

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
				  "ERROR: type missmatch. MathExpression needs a container "
				  "with variable names");
	// Setup MuParser expression
	parse_our_expression();
	// Register our variables
	for (const auto& name: varnames) {
#ifndef NRANGECHK
		if (std::string::npos == exprStr.find(name))
			throw std::out_of_range(std::string("invalid variable name: \"")
									.append(name).append("\""));
#endif
		varsMap_.emplace_back(std::move(std::make_pair(name, -1)));
		// Real mapping is later done with pin_up_vars()
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
				  "ERROR: type missmatch. MathExpression needs a container "
				  "with variable names");
	// Setup MuParser expression
	parse_our_expression();
	// Register our variables
	for (auto& name: varnames) {
#ifndef NRANGECHK
		if (std::string::npos == exprStr.find(name))
			throw std::out_of_range(std::string("invalid variable name: \"")
									.append(name).append("\""));
#endif
		varsMap_.emplace_back(std::move(std::make_pair(std::move(name), -1)));
		// Real mapping is later done with pin_up_vars()
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
		exprStr_(exprStr)
{
	static_assert(std::is_constructible< std::string, ValueType >::value,
				  "ERROR: type missmatch. MathExpression needs iterators "
				  "pointing to variable names");
	// Setup MuParser expression
	parse_our_expression();
	// Register our variables
	varsMap_.reserve(std::distance(from,to));
	for (; from != to ; from++) {
		std::string name = *from;
#ifndef NRANGECHK
		if (std::string::npos == exprStr.find(name))
			throw std::out_of_range(std::string("invalid variable name: \"")
									.append(name).append("\""));
#endif
		varsMap_.emplace_back(std::move(std::make_pair(name, -1)));
		// Real mapping is later done with pin_up_vars()
	}
}

} // namespace fig

#endif // MATHEXPRESSION_H
