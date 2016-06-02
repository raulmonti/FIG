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
//	along with FIG; if not, write to the Free Software Foundation,
//	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//==============================================================================


#ifndef PRECONDITION_H
#define PRECONDITION_H

#include <MathExpression.h>
#include <core_typedefs.h>
#include <State.h>


namespace fig
{

/**
 * @brief Transition precondition:
 *        a boolean guard with predicates over variables values.
 *
 *        The names of the variables appearing in the expression string
 *        of a Precondition must refer to existing Variables in the global
 *        State of the system, 'gState'.
 */
class Precondition : public MathExpression
{
	/// @brief Perform a fake evaluation to exercise our expression
	/// @note  Useful to reveal parsing errors in MathExpression
	/// @throw FigException if badly parsed expression
	void test_evaluation() const;

public:  // Ctors

	/// @copydoc MathExpression::MathExpression
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	inline Precondition(const std::string& exprStr,
						const Container<ValueType, OtherContainerArgs...>& varnames) :
        MathExpression(exprStr, varnames)
		{}

	/// Data ctor from generic rvalue container
	/// @see Equivalent ctor in MathExpression
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	inline Precondition(const std::string& exprStr,
						Container<ValueType, OtherContainerArgs...>&& varnames) :
        MathExpression(exprStr, varnames)
        {}

	/// Data ctor from iterator range
	/// @see Equivalent ctor in MathExpression
	template< template< typename, typename... > class Iterator,
			  typename ValueType,
			  typename... OtherIteratorArgs >
	inline Precondition(const std::string& exprStr,
						Iterator<ValueType, OtherIteratorArgs...> from,
						Iterator<ValueType, OtherIteratorArgs...> to) :
        MathExpression(exprStr, from, to)
        {}

	/// Default copy ctor
	Precondition(const Precondition& that) = default;

	/// Default move ctor
	Precondition(Precondition&& that) = default;

	/// Default copy assignment
	Precondition& operator=(const Precondition& that) = default;

	/// Default move assignment
	Precondition& operator=(Precondition&& that) = default;

public:  // Modifyers, made public since too many other classes use Precondition

	/**
	 * @copydoc fig::MathExpression::pin_up_vars()
	 * \ifnot NDEBUG
	 *   @throw FigException if there was some error in our math expression
	 * \endif
	 * @todo TODO unify with the other version using templates;
	 *            see ImportanceFunction::Formula::set()
	 */
	void pin_up_vars(const State<STATE_INTERNAL_TYPE>& globalState) override;

	/**
	 * @copydoc fig::MathExpression::pin_up_vars(const PositionMap&)
	 * \ifnot NDEBUG
	 *   @throw FigException if there was some error in our math expression
	 * \endif
	 * @todo TODO unify with the other version using templates;
	 *            see ImportanceFunction::Formula::set()
	 */
#ifndef NRANGECHK
	void pin_up_vars(const PositionsMap& globalVars) override;
#else
	void pin_up_vars(PositionsMap& globalVars) override;
#endif

public:  // Utils

	/**
	 * @brief Compute truth value of our expression for given variables valuation
	 * @param state Valuation of the system's global state
	 * @note pin_up_vars() should have been called before to register the
	 *       position of the expression's variables in the global State
	 * @note To work with local states from the \ref ModuleInstace
	 *       "system modules" use the State variant of this operator
	 * @throw mu::ParserError
	 * \ifnot NDEBUG
	 *   @throw FigException if pin_up_vars() hasn't been called yet
	 * \endif
	 */
	bool operator()(const StateInstance& state) const;

	/**
	 * @brief Compute truth value of our expression for given state
	 * @param state The state of any Module (ModuleInstace or ModuleNetwork)
	 * @note Slower than the StateInstance variant of this operator,
	 *       since it has to search for the variables positions in 'state'
	 * @throw mu::ParserError
	 * \ifnot NDEBUG
	 *   @throw FigException if pin_up_vars() hasn't been called yet
	 * \endif
	 */
	bool operator()(const State<STATE_INTERNAL_TYPE>& state) const;
};

} // namespace fig

#endif // PRECONDITION_H
