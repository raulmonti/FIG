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
// External code
#include <muParser.h>
// FIG
#include <string_utils.h>
#include <core_typedefs.h>
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

	/// Is the expression empty?
	/// @note Needed since MuParser doesn't tolerate empty string expressions
	bool empty_;

    /// String describing the mathematical expression
    std::string exprStr_;

	/// Mathematical expression per se
	Expression expr_;

	/// Number of variables defined in our expression
	size_t NVARS_;

	/// @brief Names of our variables
	/// @details Symbols in exprStr_ which map to variable names
	std::vector< std::string > varsNames_;

	/// @brief Global position of our variables
	/// @details Position of the variables from exprStr_ in a global State
	std::vector< size_t > varsPos_;

	/// @brief Values of our variables
	/// @details "Current values" of our variables in a running simulation
	mutable StateInstance varsValues_;

	/// Whether the global positional order of our variables
	/// (i.e. varsPos_) has already been defined
	bool pinned_;

public:  // Ctors/Dtor

	/**
	 * @brief Data ctor from generic lvalue container
	 *
	 * @param exprStr   String with the matemathical expression to evaluate
	 * @param varnames  Container with names of variables ocurring in exprStr
	 *
	 * @throw FigException if exprStr doesn't define a valid expression
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherArgs >
	MathExpression(const std::string& exprStr,
				   const Container<ValueType, OtherArgs...>& varnames);

	/**
	 * @brief Data ctor from generic rvalue container
	 *
	 * @param exprStr   String with the mathematical expression to evaluate
	 * @param varnames  Container with names of variables ocurring in exprStr
	 *
	 * @throw FigException if exprStr doesn't define a valid expression
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherArgs >
	MathExpression(const std::string& exprStr,
				   Container<ValueType, OtherArgs...>&& varnames);

	/**
	 * @brief Data ctor from iterator range
	 *
	 * @param exprStr  String with the mathematical expression to evaluate
	 * @param from     Iterator to  first name of variables ocurring in exprStr
	 * @param to       Iterator past last name of variables ocurring in exprStr
	 *
	 * @throw FigException if exprStr doesn't define a valid expression
	 */
	template< template< typename, typename... > class Iterator,
			  typename ValueType,
			  typename... OtherArgs >
	MathExpression(const std::string& exprStr,
				   Iterator<ValueType, OtherArgs...> from,
				   Iterator<ValueType, OtherArgs...> to);

	/// Default copy ctor
	MathExpression(const MathExpression& that) = default;

	/// Default move ctor
	MathExpression(MathExpression&& that) = default;

	/// Copy assignment with copy&swap idiom
	MathExpression& operator=(MathExpression that);

protected:  // Modifyers

	/**
	 * @brief Register the global-system-state position of our variables
	 *
	 * @param globalState State who knows all the variables mappings required
	 *
	 * @warning Asynchronous callback to be called <b>exactly once</b>
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if some of our variables isn't mapped
	 * \endif
	 */
	virtual void pin_up_vars(const State<STATE_INTERNAL_TYPE>& globalState);

	/**
	 * @brief Register the global-system-state position of our variables
	 *
	 * @param globalVars Mapping of our variables names to their
	 *                   respective positions in a global array
	 *
	 * @warning Asynchronous callback to be called <b>exactly once</b>
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if some of our variables isn't mapped
	 * \endif
	 */
#ifndef NRANGECHK
	virtual void pin_up_vars(const PositionsMap& globalVars);
#else
	virtual void pin_up_vars(PositionsMap& globalVars);
#endif

public:  // Accessors

	/// @copydoc exprStr_
	inline const std::string expression() const noexcept { return empty_ ? "" : exprStr_; }

	/// @copydoc pinned_
	inline const bool& pinned() const noexcept { return pinned_; }

protected:  // Class utils

    /// Return a "MuParser friendly" formatted version of the expression string
    std::string muparser_format(const std::string& expr) const;

	/**
	 * @brief Set 'exprStr_' as the expression to MuParser's 'expr_'
	 * @note  After successfully parsing the expression string,
	 *        all offered builtin functions are bound to 'expr_'
	 * @throw FigException if badly formatted mathematical expression
	 */
	void parse_our_expression();
};


// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< template< typename, typename... > class Iterator,
		  typename ValueType,
		  typename... OtherArgs >
MathExpression::MathExpression(
	const std::string& exprStr,
	Iterator<ValueType, OtherArgs...> from,
	Iterator<ValueType, OtherArgs...> to) :
		empty_(trim(exprStr).empty()),
        exprStr_(muparser_format(exprStr)),
		NVARS_(std::distance(from, to)),
		varsValues_(NVARS_),
        pinned_(false)
{
	static_assert(std::is_constructible< std::string, ValueType >::value,
				  "ERROR: type mismatch. MathExpression needs iterators "
				  "pointing to variable names");
	// Setup MuParser expression
	parse_our_expression();
	// Register our variables names
	varsNames_.reserve(NVARS_);
	for (; from != to ; from++)
		if (exprStr_.find(*from) != std::string::npos)
			varsNames_.emplace_back(*from);  // copy elision
	// Positions mapping is done later in pin_up_vars()
	varsPos_.reserve(NVARS_);
}

} // namespace fig

#endif // MATHEXPRESSION_H
