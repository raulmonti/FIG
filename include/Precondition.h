//==============================================================================
//
//  Precondition.h
//
//  Copyleft 2015-
//  Authors:
//  - Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
//  - Leonardo Rodríguez
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

#include <core_typedefs.h>
#include <State.h>
#include <ExpStateEvaluator.h>


namespace fig
{

/**
 * @brief Transition precondition:
 *        a boolean guard with predicates over variables values.
 *
 */
class Precondition : public ExpStateEvaluator
{
	std::shared_ptr<Exp> expr_;

public:  // Ctors

	Precondition(std::shared_ptr<Exp> expr)
	    : ExpStateEvaluator(expr), expr_(expr)
	    { /* Not much to do around here*/ }

    /// @brief Copy Constructor
    Precondition(const Precondition& that) = default;

    /// @brief Move Constructor
    Precondition(Precondition&& that) = default;

	/// No copy assignment
    Precondition& operator=(Precondition that) = delete;

	~Precondition() override {}

public:  // Utils

	inline bool operator()(const StateInstance& state) const
	    { return (eval(state)); }

	inline bool operator()(const State<STATE_INTERNAL_TYPE>& state) const
	    { return (eval(state)); }

	shared_ptr<Exp> get_expression() const noexcept
	    { return (expr_); }

public:  // Debug

    void print_info(std::ostream& out) const;
};

} // namespace fig

#endif // PRECONDITION_H
