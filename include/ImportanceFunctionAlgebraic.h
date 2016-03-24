//==============================================================================
//
//  ImportanceFunctionAlgebraic.h
//
//  Copyleft 2016-
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

#ifndef IMPORTANCEFUNCTIONALGEBRAIC_H
#define IMPORTANCEFUNCTIONALGEBRAIC_H

// C++
#include <vector>
// FIG
#include <core_typedefs.h>
#include <ImportanceFunction.h>
#include <MathExpression.h>


namespace fig
{

/**
 * @brief Importance function for on-the-fly importance assessment
 *
 *        This kind of ImportanceFunction keeps an internal algebraic formula
 *        which is evaluated on the fly everytime the importance of a
 *        StateInstance is requested. It can therefore compute the importance
 *        of the whole "symbolic state space" <b>on demand</b>.<br>
 *        This is less CPU efficient than keeping an internal vector with
 *        importance information as \ref ImportanceFunctionConcrete "concrete
 *        importance functions" do, but it's way more memory efficient.
 * 
 * @see ImportanceFunction
 * @see ImportanceFunctionConcrete
 */
class ImportanceFunctionAlgebraic : public ImportanceFunction
{
public:  // Ctor/Dtor

	/// Empty ctor
	ImportanceFunctionAlgebraic();

	/// Dtor
	virtual ~ImportanceFunctionAlgebraic();

public:  // Accessors

	inline bool concrete() const noexcept override final { return false; }

	/// @copydoc ImportanceFunction::importance_of()
	/// @note <b>Complexity:</b> <i>O(size(state))</i> +
	///                          <i>O(mu::Parser::Eval(state))</i>
	ImportanceValue importance_of(const StateInstance& state) const override;

	void print_out(std::ostream& out, State<STATE_INTERNAL_TYPE> s) const override;

public:  // Utils

	/**
	 * @brief Set a new mathematical formula to assess the importance
	 *        of the symbolic states in the system model.
	 *
	 * @param strategy       Importance assessment strategy ("flat" or "adhoc")
	 * @param formulaExprStr String with the new mathematical expression
	 * @param varnames       Names of variables ocurring in formulaExprStr,
	 *                       viz. substrings in it that refer to variable names
	 * @param gState         Model's global state in its initial valuation
	 * @param property       Property identifying the rare states
	 *
	 * @note After a successfull invocation this ImportanceFunction
	 *       is considered to hold \ref has_importance_info() "importance
	 *       information" for the passed assessment strategy.
	 *
	 * @throw FigException if badly formatted 'formulaExprStr' or 'varnames'
	 *                     has names not appearing in 'formulaExprStr'
	 */
	template< template< typename... > class Container, typename... OtherArgs >
	void set_formula(const std::string& strategy,
					 const std::string& formulaExprStr,
					 const Container< std::string, OtherArgs... >& varnames,
					 const State<STATE_INTERNAL_TYPE>& gState,
					 const Property& property);
};

} // namespace fig

#endif // IMPORTANCEFUNCTIONALGEBRAIC_H
