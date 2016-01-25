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
 *     Transient properties speak of the probability of reaching some goal
 *     before something bad happens.
 *     These can be generally categorized as "safety properties" and are
 *     described by the PCTL formula "P(!stop U goal)". Here "stop"
 *     implies an unsafe event has taken place and activity must stop,
 *     whereas "goal" means the final condition event has been observed.
 */
class PropertyTransient : public Property
{
    /// Event triggered when the simulation should be "prematurely interrupted"
    /// (it kinda failed)
    Property::Formula stop;

    /// Event triggered when the simulation reached the final destination
    /// (it kinda succeeded)
    Property::Formula goal;

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
	PropertyTransient(const std::string& stopExpr,
					  const Container1<ValueType1, OtherArgs1...>& stopExprVars,
					  const std::string& goalExpr,
					  const Container2<ValueType2, OtherArgs2...>& goalExprVars) :
		Property(std::string("P( !(").append(stopExpr).append(") U (")
									 .append(goalExpr).append(") )"),
				 PropertyType::TRANSIENT),
		stop(stopExpr, stopExprVars),
		goal(goalExpr, goalExprVars)
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
	PropertyTransient(const std::string& stopExpr,
					  Iterator1<ValueType1, OtherArgs1...> stopExprVarsFrom,
					  Iterator1<ValueType1, OtherArgs1...> stopExprVarsTo,
					  const std::string& goalExpr,
					  Iterator2<ValueType2, OtherArgs2...> goalExprVarsFrom,
					  Iterator2<ValueType2, OtherArgs2...> goalExprVarsTo) :
		Property(std::string("P( !(").append(stopExpr).append(") U (")
									 .append(goalExpr).append(") )"),
				 PropertyType::TRANSIENT),
		stop(stopExpr, stopExprVarsFrom, stopExprVarsTo),
		goal(goalExpr, goalExprVarsFrom, goalExprVarsTo)
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

    /// String expression of the "stop" subformula of this property
    /// @see PropertyTransient::stop
    const std::string stop_expression() const noexcept;

    /// String expression of the "goal" subformula of this property
    /// @see PropertyTransient::goal
    const std::string goal_expression() const noexcept;

protected:  // Modifyers

	virtual void pin_up_vars(const PositionsMap &globalVars);

	virtual void pin_up_vars(const fig::State<STATE_INTERNAL_TYPE>& globalState);

public:  // Utils

	/// Is the "stop" subformula satisfied by the given variables valuation?
	/// @see PropertyTransient::stop
	inline bool is_stop(const StateInstance& s) const { return stop(s); }

	/// @copydoc is_stop()
	inline bool is_stop(const State<STATE_INTERNAL_TYPE>& s) const
		{ return is_stop(s.to_state_instance()); }

	/// Is the "goal" subformula satisfied by the given variables valuation?
	/// @see PropertyTransient::goal
	inline bool is_goal(const StateInstance& s) const { return goal(s); }

	/// @copydoc is_goal()
	inline bool is_goal(const State<STATE_INTERNAL_TYPE>& s) const
		{ return is_goal(s.to_state_instance()); }

    inline virtual bool satisfied_by(const StateInstance& s) const
        { return !is_stop(s) || is_goal(s); }  // weak until... is it OK?

    inline virtual bool satisfied_by(const State<STATE_INTERNAL_TYPE>& s) const
        { return satisfied_by(s.to_state_instance()); }

    inline virtual bool is_rare(const StateInstance& s) const
        { return is_goal(s); }

    inline virtual bool is_rare(const State<STATE_INTERNAL_TYPE>& s) const
        { return is_rare(s.to_state_instance()); }
};

} // namespace fig

#endif // PROPERTYTRANSIENT_H

