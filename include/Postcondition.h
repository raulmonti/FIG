//==============================================================================
//
//  Postcondition.h
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


#ifndef POSTCONDITION_H
#define POSTCONDITION_H

// C++
#include <vector>
#include <iterator>  // std::distance()
#include <type_traits>  // std::is_constructible<>
// FIG
#include <MathExpression.h>
#include <State.h>


namespace fig
{

/**
 * @brief Transition postcondition:
 *        a list of comma separated updates on variables values.
 *
 *        Each 'update' consists of a regular MathExpression and,
 *        with a vector of variable names passed on construction,
 *        the user indicates which variable will hold the result
 *        of each update.
 *
 *        For instance the following string specifies two updates:
 *
 *        "max(x,10), x^3"
 *
 *        If the corresponding array of variable names to update is "[x,y]",
 *        then the postcondition updates will be evaluated as follows:
 *
 *        x_copy = x         <br>
 *        y_copy = y         <br>
 *        x = max(x_copy,10) <br>
 *        y = x_copy^3       <br>
 *
 *        Continuing with this example, for the state [x,y] = [2,0]
 *        the resulting values would be [10,8].
 *        Notice 'y' is assigned 8 == 2^3, since the value of 'x' prior
 *        its own update, namely 'x_copy == 2', was used for the evaluation
 *        of the MathExpressions on the RHS of the updates.
 */
class Postcondition : public MathExpression
{
	/// Number of variables updated by this postcondition
	int numUpdates_;

	/// Positions of the variables to which the updates will be applied,
	/// following the unique order given in the State 'gState'
	std::vector<size_t> updatesPositions_;

	/**
	 * @brief Perform a fake evaluation to exercise our expression
	 * @note  Useful to reveal parsing errors in MathExpression
	 * @throw FigException if badly parsed expression
	 */
	void fake_evaluation();

public:  // Ctors

	/**
	 * @brief Data ctor from generic lvalue containers
	 *
	 * @param exprStr    String with the comma-separated updates to evaluate
	 * @param varNames   Container with names of all variables ocurring in exprStr
	 * @param updateVars Container with the names of the variables taking the updates
	 *
	 * @note The content of updateVars is interpreted possitionally w.r.t.
	 *       the exprStr, e.g. input data: ("x^2,Rand[0,1]", {x}, {y,x})
	 *       means that on each update, 'y' will be assigned the square of 'x',
	 *       and 'x' will be assigned some random value between 0 and 1.
	 *
	 * @throw FigException if exprStr doesn't define a valid expression
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if either 'varNames' or 'updateVars' contain
	 *          a variable name not appearing in the system State
	 * \endif
	 */
	template< template< typename, typename... > class Container1,
				  typename ValueType1,
				  typename... OtherContainerArgs1,
			  template< typename, typename... > class Container2,
				  typename ValueType2,
				  typename... OtherContainerArgs2
			>
	Postcondition(const std::string& exprStr,
				  const Container1<ValueType1, OtherContainerArgs1...>& varNames,
				  const Container2<ValueType2, OtherContainerArgs2...>& updateVars);

	/**
	 * @brief Data ctor from iterator ranges
	 *
	 * @param exprStr String with the comma-separated updates to evaluate
	 * @param from1   Iterator to  first name of variables ocurring in exprStr
	 * @param to1     Iterator past last name of variables ocurring in exprStr
	 * @param from2   Iterator to  first name of the variables taking the updates
	 * @param to2     Iterator past last name of the variables taking the updates
	 *
	 * @note The variables whose names are given in the range [from2, to2)
	 *       are interpreted possitionally w.r.t. the exprStr, e.g. input
	 *       data equivalent to: ("x^2,Rand[0,1]", {y,x}) means that
	 *       on each update, 'y' will be assigned the square of 'x',
	 *       and 'x' will be assigned some random value between 0 and 1.
	 *
	 * @throw FigException if exprStr doesn't define a valid expression
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if either 'varNames' or 'updateVars' contain
	 *          a variable name not appearing in the system State
	 * \endif
	 */
	template< template< typename, typename... > class Iterator1,
				  typename ValueType1,
				  typename... OtherIteratorArgs1,
			  template< typename, typename... > class Iterator2,
				  typename ValueType2,
				  typename... OtherIteratorArgs2
			>
	Postcondition(const std::string& exprStr,
				  Iterator1<ValueType1, OtherIteratorArgs1...> from1,
				  Iterator1<ValueType1, OtherIteratorArgs1...> to1,
				  Iterator2<ValueType2, OtherIteratorArgs2...> from2,
				  Iterator2<ValueType2, OtherIteratorArgs2...> to2);

public:  // Accessors

	/**
	 * @brief Update state's variables values according to our expression
	 * @throw mu::ParserError
	 */
	void operator()(StateInstance& state);
};


// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< template< typename, typename... > class Container1,
			  typename ValueType1,
			  typename... OtherContainerArgs1,
		  template< typename, typename... > class Container2,
			  typename ValueType2,
			  typename... OtherContainerArgs2
		>
Postcondition::Postcondition(
	const std::string& exprStr,
	const Container1<ValueType1, OtherContainerArgs1...>& varNames,
	const Container2<ValueType2, OtherContainerArgs2...>& updateVars) :
		MathExpression(exprStr, varNames)
{
	static_assert(std::is_constructible< std::string, ValueType2 >::value,
				  "ERROR: type missmatch. Postcondition needs containers "
				  "with variable names");
	// Setup updates mapping in updatesPositions_
	for (const auto& name: updateVars)
		updatesPositions_.emplace_back(gState.position_of_var(name));
	numUpdates_ = static_cast<int>(updatesPositions_.size());
#ifndef NDEBUG
	// Reveal parsing errors in this early stage
	fake_evaluation();
#endif
}


template< template< typename, typename... > class Iterator1,
			  typename ValueType1,
			  typename... OtherIteratorArgs1,
		  template< typename, typename... > class Iterator2,
			  typename ValueType2,
			  typename... OtherIteratorArgs2
		>
Postcondition::Postcondition(
	const std::string& exprStr,
	Iterator1<ValueType1, OtherIteratorArgs1...> from1,
	Iterator1<ValueType1, OtherIteratorArgs1...> to1,
	Iterator2<ValueType2, OtherIteratorArgs2...> from2,
	Iterator2<ValueType2, OtherIteratorArgs2...> to2) :
		MathExpression(exprStr, from1, to1),
		numUpdates_(static_cast<int>(std::distance(from2, to2)))
{
	static_assert(std::is_constructible< std::string, ValueType2 >::value,
				  "ERROR: type missmatch. Postcondition needs iterators "
				  "pointing to variable names");
	// Setup updates mapping in updatesPositions_
	do {
		updatesPositions_.emplace_back(gState.position_of_var(*from2));
	} while (++from2 != to2);
#ifndef NDEBUG
	// Reveal parsing errors in this early stage
	fake_evaluation();
#endif
}

} // namespace fig

#endif // POSTCONDITION_H
