//==============================================================================
//
//  PropertyTransient.h
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


#ifndef PROPERTYTRANSIENT_H
#define PROPERTYTRANSIENT_H

#include <core_typedefs.h>
#include <Property.h>


namespace fig
{

/**
 * @copydoc TRANSIENT
 *
 * @details Transient properties describe finite execution traces.
 *          For instance "safety properties" described by the PCTL formula
 *          P(!stop U goal), which expresses the probability of remaining
 *          in safe "non-stopping" states until a goal is reached, are
 *          transient properties.<br>
 *          The general idea is to visit only states that satisfy a set of
 *          conditions (described by the logical expression "expr1", say),
 *          until a state that satisfies another set of conditions (described
 *          by "expr2", say) is visited.
 *          Execution is thus terminated when a state that doesn't satisfy
 *          "expr1" or that satisfies "expr2" is visited, whichever happens first.
 */
class PropertyTransient : public Property
{
    /// This should be continuously satisfied, otherwise the simulation is
    /// "prematurely interrupted" (it kinda failed)
    /// This is the subformula on the LHS of the 'UNTIL'
    Property::Formula expr1_;

    /// When this becomes true the simulation reached its "final destination"
    /// (it kinda succeeded)
    /// This is the subformula on the RHS of the 'UNTIL'
    Property::Formula expr2_;

public:  // Ctors

	/**
	 * @brief Data ctor from generic lvalue containers
	 *
	 * @param stopExpr     Mathematical expression for the "stop" subformula
	 * @param stopExprVars Names of the variables ocurring in stopExpr
	 * @param goalExpr     Mathematical expression for the "goal" subformula
	 * @param goalExprVars Names of the variables ocurring in goalExpr
	 *
	 * @throw FigException if "stopExpr" or "goalExpr" aren't valid expressions
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if names of variables not appearing
	 *                       in the expression strings were passed
	 * \endif
	 */
	template<
		template< typename, typename... > class Container1,
			typename ValueType1,
			typename... OtherArgs1,
		template< typename, typename... > class Container2,
			typename ValueType2,
			typename... OtherArgs2
	>
	PropertyTransient(const std::string& expr1,
					  const Container1<ValueType1, OtherArgs1...>& expr1Vars,
					  const std::string& expr2,
					  const Container2<ValueType2, OtherArgs2...>& expr2Vars) :
		Property(std::string("P( (").append(expr1).append(") U (")
									.append(expr2).append(") )"),
				 PropertyType::TRANSIENT),
		expr1_(expr1, expr1Vars),
		expr2_(expr2, expr2Vars)
		{}

	/**
	 * @brief Data ctor from iterator ranges
	 *
	 * @param stopExpr         Mathematical expression for the "stop" subformula
	 * @param stopExprVarsFrom Iterator to  first name of variables ocurring in stopExpr
	 * @param stopExprVarsTo   Iterator past last name of variables ocurring in stopExpr
	 * @param goalExpr         Mathematical expression for the "goal" subformula
	 * @param goalExprVarsFrom Iterator to  first name of variables ocurring in goalExpr
	 * @param goalExprVarsTo   Iterator past last name of variables ocurring in goalExpr
	 *
	 * @throw FigException if "stopExpr" or "goalExpr" aren't valid expressions
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if names of variables not appearing
	 *                       in the expression strings were passed
	 * \endif
	 */
	template<
		template< typename, typename... > class Iterator1,
			typename ValueType1,
			typename... OtherArgs1,
		template< typename, typename... > class Iterator2,
			typename ValueType2,
			typename... OtherArgs2
	>
	PropertyTransient(const std::string& expr1,
					  Iterator1<ValueType1, OtherArgs1...> expr1VarsFrom,
					  Iterator1<ValueType1, OtherArgs1...> expr1VarsTo,
					  const std::string& expr2,
					  Iterator2<ValueType2, OtherArgs2...> expr2VarsFrom,
					  Iterator2<ValueType2, OtherArgs2...> expr2VarsTo) :
		Property(std::string("P( (").append(expr1).append(") U (")
									.append(expr2).append(") )"),
				 PropertyType::TRANSIENT),
		expr1_(expr1, expr1VarsFrom, expr1VarsTo),
		expr2_(expr2, expr2VarsFrom, expr2VarsTo)
		{}

    /// Default copy ctor
    PropertyTransient(const PropertyTransient& that) = default;
    /// Default move ctor
    PropertyTransient(PropertyTransient&& that)      = default;

    /// Can't have empty ctor due to const data members from Property
    PropertyTransient()                                         = delete;
    /// Can't have copy assignment due to const data members from Property
    PropertyTransient& operator=(const PropertyTransient& that) = delete;
    /// Can't have move assignment due to const data members from Property
    PropertyTransient& operator=(PropertyTransient&& that)      = delete;

public:  // Accessors

    /// String expression of the "expr1" subformula of this property
    /// @see PropertyTransient::expr1_
    const std::string expression1() const noexcept;

    /// String expression of the "expr2" subformula of this property
    /// @see PropertyTransient::expr2_
    const std::string expression2() const noexcept;

protected:  // Modifyers

	virtual void pin_up_vars(const PositionsMap &globalVars);

	virtual void pin_up_vars(const fig::State<STATE_INTERNAL_TYPE>& globalState);

public:  // Utils

	/// Is the "expr1" subformula satisfied by the given variables valuation?
	/// @see PropertyTransient::expr1_
	inline bool expr1(const StateInstance& s) const { return expr1_(s); }

	/// @copydoc expr1()
	inline bool expr1(const State<STATE_INTERNAL_TYPE>& s) const
		{ return expr1(s.to_state_instance()); }

	/// Is the "expr2" subformula satisfied by the given variables valuation?
	/// @see PropertyTransient::expr2_
	inline bool expr2(const StateInstance& s) const { return expr2_(s); }

	/// @copydoc expr2()
	inline bool expr2(const State<STATE_INTERNAL_TYPE>& s) const
		{ return expr2(s.to_state_instance()); }

    inline virtual bool is_rare(const StateInstance& s) const
        { return expr2(s); }

    inline virtual bool is_rare(const State<STATE_INTERNAL_TYPE>& s) const
        { return expr2(s.to_state_instance()); }
};

} // namespace fig

#endif // PROPERTYTRANSIENT_H

