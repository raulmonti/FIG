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
	std::vector< ImportanceValue > invertedThresholds_;

public:  // Ctor/Dtor

	ImportanceFunctionAlgebraic();

	virtual ~ImportanceFunctionAlgebraic();

public:  // Accessors

	inline virtual bool concrete() const noexcept { return false; }

	virtual ImportanceValue importance_of(const StateInstance& state) const;

	virtual ImportanceValue level_of(const StateInstance& state) const;

	virtual void print_out(std::ostream& out) const;

public:  // Utils


	/// Set a new mathematical formula to assess the importance of the
	/// symbolic states in the system model.
	/// @param formulaExprStr String with the new mathematical expression
	/// @param varnames       Names of variables ocurring in formulaExprStr
	void set_formula(const std::string& formulaExprStr,
					 const std::vector< std::string > varnames);

	virtual void build_thresholds(ThresholdsBuilder& tb,
								  const unsigned& splitsPerThreshold);

	virtual void clear() noexcept;
};

} // namespace fig

#endif // IMPORTANCEFUNCTIONALGEBRAIC_H
