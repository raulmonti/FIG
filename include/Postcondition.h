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
#include <iterator>     // std::distance(), std::begin()
#include <algorithm>    // std::copy() ranges
#include <type_traits>  // std::is_constructible<>
// FIG
#include <MathExpression.h>
#include <State.h>
#include <ModelAST.h>

// ADL
using std::copy;
using std::begin;
using std::end;
using std::shared_ptr;


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
class Postcondition
{
public:  // Ctors/Dtor

    /**
     * @brief Data ctor from generic lvalue containers
     *
     */
    Postcondition(std::vector<shared_ptr<Assignment>> assignments) {
        (void) assignments;
    }

    /// Default copy ctor
    Postcondition(const Postcondition& that) = default;

    /// Default move ctor
    Postcondition(Postcondition&& that) = default;

    /// Copy assignment with copy&swap idiom
    Postcondition& operator=(Postcondition that);

public:

    inline void prepare(const PositionsMap& globalVars) {
        //expr1_.prepare(globalVars);
        //expr2_.prepare(globalVars);
    }

    inline void prepare(const fig::State<STATE_INTERNAL_TYPE>& globalState) {
        //expr1_.prepare(globalState);
        //expr2_.prepare(globalState);
    }

    /**
     * @brief Update state's variables values according to our expression
     * @param state State to evaluate and update <b>(modified)</b>
     * @note Slower than the StateInstance version of this function,
     *       since it has to search for the variables positions in 'state'
     * @throw mu::ParserError
     * \ifnot NDEBUG
     *   @throw FigException if pin_up_vars() hasn't been called yet
     * \endif
     * @todo TODO unify with the other version using templates;
     *            see ImportanceFunction::Formula::set()
     */
    void operator()(State<STATE_INTERNAL_TYPE>& state) const;

    /**
     * @brief Update variables valuation according to our expression
     * @param state StateInstance to evaluate and update <b>(modified)</b>
     * @note pin_up_vars() should have been called before to register the
     *       position of the expression's variables in the global State
     * @throw mu::ParserError
     * \ifnot NDEBUG
     *   @throw FigException if pin_up_vars() hasn't been called yet
     * \endif
     * @todo TODO unify with the other version using templates;
     *            see ImportanceFunction::Formula::set()
     */
    void operator()(StateInstance& state) const;

public: //Debug
    void print_info(std::ostream& out) const;
};

} // namespace fig

#endif // POSTCONDITION_H
