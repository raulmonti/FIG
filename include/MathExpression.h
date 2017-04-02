//==============================================================================
//
//  MathExpression.h
//
//  Copyleft 2015-
//  Authors:
//  - Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
//  - Leandro Rodríguez: changed from MuParser to Exprtk
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
#include <algorithm>    // std::find()
#include <iterator>     // std::begin(), std::end(), std::distance()
#include <utility>      // std::pair<>, std::move()
#include <string>
#include <stdexcept>    // std::out_of_range
// External code
#include "exprtk.hpp"   // 1.3mb   <-- shieeeit!
// FIG
#include <string_utils.h>
#include <core_typedefs.h>
#include <State.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

// ADL
using std::begin;
using std::end;

//typed used internally by Exprtk.
//STATE_INTERNAL_TYPE must be convertible to NUMTYPE
#ifndef NUMTYPE
#  define NUMTYPE float
#endif

namespace fig
{

/**
 * @brief Mathematical expression with variables mapping
 *
 *        A mathematical expression is built from an expression string using
 *        the <a href="https://github.com/ArashPartow/exprtk">Exprtk</a>.
 *        It requires a separate explicit specification of which literals
 *        within that expression refer to variables names.
 *
 * @note  Offers generic construction from the following STL containers:
 *        vector, list, forward_list, set, unordered_set, deque.
 * @note  Will not build from the following STL containers:
 *        queue, stack, array.
 *
 * @todo This class is currently used only by \ref ImportanceFunction. Replace
 * this class with a subclass of \ref ExprStateEvaluator
 */
class MathExpression
{
protected:

    typedef exprtk::expression<NUMTYPE> expression_t;
    typedef exprtk::symbol_table<NUMTYPE> symbol_table_t;

	/// Is the expression empty?
    /// @note Needed since Exprtk doesn't tolerate empty string expressions
	bool empty_;

    /// String describing the mathematical expression
    std::string exprStr_;

	/// Mathematical expression per se
    expression_t expr_;

    /// Symbol table of mathematical expression
    symbol_table_t table_;

    /// Exprtk parser
    exprtk::parser<NUMTYPE> parser;

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
    mutable std::vector<NUMTYPE> varsValues_;

	/// Whether the global positional order of our variables
	/// (i.e. varsPos_) has already been defined and the local values
	/// (i.e. varsValues_) have been referenced into the Expression
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

	/// Copy ctor
	/// @note Explicitly defined for variables pinning into expr_
    MathExpression(const MathExpression& that) = delete;

	/// Default move ctor
    MathExpression(MathExpression&& that) = delete;

	/// Copy assignment with copy&swap idiom
	/// @note Explicitly defined for variables pinning into expr_
    // MathExpression& operator=(MathExpression that);

protected:  // Modifyers

	/// Bind variables to mathematical expression,
	/// i.e. embed \a varsValues_ into \a expr_
    void compile_expression();

public:  // Accessors

	/// @copydoc exprStr_
	inline const std::string expression() const noexcept { return empty_ ? "" : exprStr_; }

	/// @copydoc pinned_
	inline const bool& pinned() const noexcept { return pinned_; }

protected:  // Class utils

    /// Return a "Exprtk friendly" formatted version of the expression string
    std::string exprtk_format(const std::string& expr) const;
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
        exprStr_(exprtk_format(exprStr)),
        pinned_(false)
{
	static_assert(std::is_constructible< std::string, ValueType >::value,
				  "ERROR: type mismatch. MathExpression needs iterators "
				  "pointing to variable names");
	// Register our variables names
	varsNames_.reserve(std::distance(from, to));
	for (; from != to ; from++)
		if (std::find(begin(varsNames_),end(varsNames_),*from) == end(varsNames_)
				&& exprStr_.find(*from) != std::string::npos)
			varsNames_.emplace_back(*from);  // copy elision
	varsNames_.shrink_to_fit();
	NVARS_ = varsNames_.size();
	// Positions mapping is done later in compile_expression()
	varsPos_.resize(NVARS_);
	varsValues_.resize(NVARS_);
    compile_expression();
}

} // namespace fig

#endif // MATHEXPRESSION_H
