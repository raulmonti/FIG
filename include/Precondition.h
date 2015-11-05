//==============================================================================
//
//  Precondition.h
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


#ifndef PRECONDITION_H
#define PRECONDITION_H

// Project code
#include <MathExpression.h>
#include <Traial.h>

namespace fig
{


/**
 * @brief Transition precondition:
 *        a boolean guard on variables of the GlobalState "gState"
 */
class Precondition : public MathExpression
{
	/**
	 * @brief Perform a fake evaluation to exercise our expression
	 * @note  Mostly used to reveal parsing errors in the expression
	 * @throw FigException if badly parsed MathExpression
	 */
	void fake_evaluation();

public:  // Ctors

	/// @copydoc MathExpression::MathExpression(const std::string&, const Container<>&)
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	Precondition(const std::string& exprStr,
				 const Container<ValueType, OtherContainerArgs...>& varnames);

	/// @copydoc MathExpression::MathExpression(const std::string&, Container<>&&)
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	Precondition(const std::string& exprStr,
				 Container<ValueType, OtherContainerArgs...>&& varnames);

	/// @copydoc MathExpression::MathExpression(const std::string&,
	template< template< typename, typename... > class Iterator,
			  typename ValueType,
			  typename... OtherIteratorArgs >
	Precondition(const std::string& exprStr,
				 Iterator<ValueType, OtherIteratorArgs...> from,
				 Iterator<ValueType, OtherIteratorArgs...> to);

public:  // Accessors

	inline const std::string& expression() const { return MathExpression::expression(); }

	/// @brief Compute truth value in the current state of this traial
	bool operator()(const Traial& traial);
};


// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
Precondition::Precondition(
	const std::string& exprStr,
	const Container<ValueType, OtherContainerArgs...>& varnames) :
		MathExpression(exprStr, varnames)
{
	// Reveal parsing errors in this early stage
	fake_evaluation();
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
Precondition::Precondition(
	const std::string& exprStr,
	Container<ValueType, OtherContainerArgs...>&& varnames) :
		MathExpression(exprStr, varnames)
{
	// Reveal parsing errors in this early stage
	fake_evaluation();
}


template< template< typename, typename... > class Iterator,
		  typename ValueType,
		  typename... OtherIteratorArgs >
Precondition::Precondition(
	const std::string& exprStr,
	Iterator<ValueType, OtherIteratorArgs...> from,
	Iterator<ValueType, OtherIteratorArgs...> to) :
		MathExpression(exprStr, from, to)
{
	// Reveal parsing errors in this early stage
	fake_evaluation();
}

} // namespace fig

#endif // PRECONDITION_H
