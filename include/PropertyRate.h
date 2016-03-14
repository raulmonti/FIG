//==============================================================================
//
//  PropertyRate.h
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

#ifndef PROPERTYRATE_H
#define PROPERTYRATE_H

#include <core_typedefs.h>
#include <Property.h>


namespace fig
{

/**
 * @copydoc RATE
 *
 * @details Rate properties describe "steady state" or "long run" behaviour
 *          of a system. For instance CSL properties described by the formula
 *          S(fail), which expresses the proportion of (say) time a system
 *          dwells in some failure states, could be regarded as rate
 *          properties.<br>
 *          The general idea is to excercise the system's behaviour extensively,
 *          keeping track of the total time spent in states satisfying certain
 *          conditions (described by the logical expression "expr", say)
 *          The total simulated time is known and an estimate for the property's
 *          value is given by the quotient between the accumulated time spent in
 *          states satisfying "expr", and the total time. Of course, the longer
 *          the simulation time, the better the quality of the estimate.
 *
 * @note Not to be confused with the more general \ref PropertyType "PropertyRatio"
 */
class PropertyRate : public Property
{
	/// This identifies the special states whose visiting times are monitored
	Property::Formula expr_;

public:  // Ctors

	/**
	 * @brief Data ctor from generic lvalue container
	 *
	 * @param expr     Mathematical expression for the only subformula
	 * @param exprVars Names of the variables ocurring in "expr"
	 *
	 * @throw FigException if "expr" isn't a valid expressions
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if names of variables not appearing
	 *                       in the expression string were passed
	 * \endif
	 */
	template< template <typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherArgs >
	PropertyRate(const std::string& expr,
				 const Container<ValueType, OtherArgs...>& exprVars) :
		Property(std::string("S( (").append(expr).append(") / total_time )"),
				 PropertyType::RATE),
		expr_(expr, exprVars)
		{}

	/**
	 * @brief Data ctor from iterator ranges
	 *
	 * @param expr         Mathematical expression for the only subformula
	 * @param exprVarsFrom Iterator to  first name of variables ocurring in expr
	 * @param exprVarsTo   Iterator past last name of variables ocurring in expr
	 *
	 * @throw FigException if "expr" isn't a valid expressions
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if names of variables not appearing
	 *                       in the expression string were passed
	 * \endif
	 */
	template< template <typename, typename... > class Iterator,
			  typename ValueType,
			  typename... OtherArgs >
	PropertyRate(const std::string& expr,
				 Iterator<ValueType, OtherArgs...> exprVarsFrom,
				 Iterator<ValueType, OtherArgs...> exprVarsTo) :
		Property(std::string("S( (").append(expr).append(") / total_time )"),
				 PropertyType::RATE),
		expr_(expr, exprVarsFrom, exprVarsTo)
		{}

	/// Default copy ctor
	PropertyRate(const PropertyRate& that) = default;
	/// Default move ctor
	PropertyRate(PropertyRate&& that)      = default;

	/// Can't have empty ctor due to const data members from Property
	PropertyRate()                                         = delete;
	/// Can't have copy assignment due to const data members from Property
	PropertyRate& operator=(const PropertyRate& that) = delete;
	/// Can't have move assignment due to const data members from Property
	PropertyRate& operator=(PropertyRate&& that)      = delete;

public:  // Accessors

	/// String expression of the only subformula of this property
	/// @see PropertyRate::expr
	inline const std::string expression() const noexcept
		{ return expr_.expression(); }

protected:  // Modifyers

	inline void pin_up_vars(const PositionsMap &globalVars) override
		{ expr_.pin_up_vars(globalVars); }

	inline void pin_up_vars(const fig::State<STATE_INTERNAL_TYPE>& globalState) override
		{ expr_.pin_up_vars(globalState); }

public:  // Utils

	/**
	 * Is the subformula satisfied by the given variables valuation?
	 * @param s Valuation of the system's global state
	 * @note To work with local states from the \ref ModuleInstace
	 *       "system modules" use the State variant
	 * @see PropertyRate::expr_
	 */
	inline bool expr(const StateInstance& s) const { return expr_(s); }

	/**
	 * Is the subformula satisfied by the given state?
	 * @param s The state of any Module (ModuleInstace or ModuleNetwork)
	 * @note Slower than the StateInstance variant
	 * @see PropertyRate::expr_
	 */
	inline bool expr(const State<STATE_INTERNAL_TYPE>& s) const { return expr_(s); }

	inline bool is_rare(const StateInstance& s) const override
		{ return expr(s); }

	inline bool is_rare(const State<STATE_INTERNAL_TYPE>& s) const override
		{ return expr(s); }
};

} // namespace fig

#endif // PROPERTYRATE_H
