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
#include <algorithm>    // std::find()
#include <iterator>     // std::begin(), std::end(), std::distance()
#include <utility>      // std::pair<>, std::move()
#include <string>
#include <stdexcept>    // std::out_of_range
// FIG
#include <string_utils.h>
#include <core_typedefs.h>
#include <State.h>
#include <ModelAST.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

// ADL
using std::begin;
using std::end;
using std::shared_ptr;


namespace fig
{

/**
 * @brief Mathematical expression with variables mapping
 *
 */
class MathExpression
{
protected: 
    /// Mathematical expression per se
    shared_ptr<Exp> expr_;

    /// Number of variables defined in our expression
    size_t NVARS_;

    /// @brief Names of our variables
    std::vector< std::string > varsNames_;

    /// @brief Global position of our variables
    std::vector< size_t > varsPos_;

    /// @brief Values of our variables
    /// @details "Current values" of our variables in a running simulation
    mutable StateInstance varsValues_;

    /// Whether the global positional order of our variables
    /// (i.e. varsPos_) has already been defined and the local values
    /// (i.e. varsValues_) have been referenced into the Expression
    bool pinned_;

public:  // Ctors/Dtor

	/**
         * @brief Data ctor
         * @param expr   AST with the matemathical expression to evaluate
	 * @throw FigException if exprStr doesn't define a valid expression
	 */
    MathExpression(shared_ptr<Exp> expr);

	/// Copy ctor
	/// @note Explicitly defined for variables pinning into expr_
    /// @todo shallow copy of a shared pointer.
    MathExpression(const MathExpression& that) = default;

	/// Default move ctor
	MathExpression(MathExpression&& that) = default;

	/// Copy assignment with copy&swap idiom
	/// @note Explicitly defined for variables pinning into expr_
    /// @todo implement this
    MathExpression& operator=(MathExpression that) = delete;

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
	 * @todo TODO unify with the other version using templates;
	 *            see ImportanceFunction::Formula::set()
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
	 * @todo TODO unify with the other version using templates;
	 *            see ImportanceFunction::Formula::set()
	 */
#ifndef NRANGECHK
	virtual void pin_up_vars(const PositionsMap& globalVars);
#else
	virtual void pin_up_vars(PositionsMap& globalVars);
#endif

public:  // Accessors

        // inline const std::string expression() const noexcept { return empty_ ? "" : exprStr_; }
        /// @copydoc pinned_
        inline const bool& pinned() const noexcept { return pinned_; }
};

// // // // // // // // // // // // // // // // // // // // // // // // // // //

} // namespace fig

#endif // MATHEXPRESSION_H
