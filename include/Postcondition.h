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

// FIG
#include <MathExpression.h>
#include <State.h>


namespace fig
{

/**
 * @brief Transition postcondition:
 *        a list of comma separated updates on variables values.
 *
 *        Each 'update' consists of a single variable name on the LHS,
 *        the symbol '=' and any properly typed expression on the RHS.
 *        For instance the following string specifies two updates:
 *
 *        "x = max(x,10), y = x^3"
 *
 *        Updates are accumulative, so for instance in the string above
 *        variable 'y' will be assigned the third power of the new value
 *        of variable 'x' after the "max(x,10)" assignment.
 */
class Postcondition : public MathExpression
{
	/// Number of variables updated by this postcondition
	int numUpdates_;

	/**
	 * @brief Perform a fake evaluation to exercise our expression
	 * @note  Useful to reveal parsing errors in MathExpression
	 * @throw FigException if badly parsed expression
	 */
	void fake_evaluation();

public:  // Ctors

	/// @copydoc MathExpression::MathExpression
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	Postcondition(const std::string& exprStr,
				  const size_t& numUpdates,
				  const Container<ValueType, OtherContainerArgs...>& varnames);

	/// Data ctor from generic rvalue container
	/// @see Equivalent ctor in MathExpression
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	Postcondition(const std::string& exprStr,
				  const size_t& numUpdates,
				  Container<ValueType, OtherContainerArgs...>&& varnames);

	/// Data ctor from iterator range
	/// @see Equivalent ctor in MathExpression
	template< template< typename, typename... > class Iterator,
			  typename ValueType,
			  typename... OtherIteratorArgs >
	Postcondition(const std::string& exprStr,
				  const size_t& numUpdates,
				  Iterator<ValueType, OtherIteratorArgs...> from,
				  Iterator<ValueType, OtherIteratorArgs...> to);

public:  // Accessors

	inline const std::string& expression() const { return exprStr_; }

	/**
	 * @brief Update state's variables values according to our expression
	 * @throw mu::ParserError
	 */
	void operator()(State& state);
};


// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
Postcondition::Postcondition(
	const std::string& exprStr,
	const size_t& numUpdates,
	const Container<ValueType, OtherContainerArgs...>& varnames) :
		MathExpression(exprStr, varnames),
		numUpdates_(numUpdates)
{
#ifndef NDEBUG
	// Reveal parsing errors in this early stage
	fake_evaluation();
#endif
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
Postcondition::Postcondition(
	const std::string& exprStr,
	const size_t& numUpdates,
	Container<ValueType, OtherContainerArgs...>&& varnames) :
		MathExpression(exprStr, std::move(varnames)),
		numUpdates_(numUpdates)
{
#ifndef NDEBUG
	// Reveal parsing errors in this early stage
	fake_evaluation();
#endif
}


template< template< typename, typename... > class Iterator,
		  typename ValueType,
		  typename... OtherIteratorArgs >
Postcondition::Postcondition(
	const std::string& exprStr,
	const size_t& numUpdates,
	Iterator<ValueType, OtherIteratorArgs...> from,
	Iterator<ValueType, OtherIteratorArgs...> to) :
		MathExpression(exprStr, from, to),
		numUpdates_(numUpdates)
{
#ifndef NDEBUG
	// Reveal parsing errors in this early stage
	fake_evaluation();
#endif
}

} // namespace fig

#endif // POSTCONDITION_H
