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

#include <Property.h>
#include <MathExpression.h>


namespace fig
{

/**
 * @brief Transient properties speak of the probability of reaching some goal
 *        before something bad happens.
 *
 *        These can be generally categorized as "safety properties" and are
 *        described by the PCTL formula "P(!stop U goal)". Here "stop"
 *        implies an unsafe event has taken place and activity must stop,
 *        whereas "goal" means the final condition event has been observed.
 */
class PropertyTransient : public Property
{
    MathExpression stop;
    MathExpression goal;

public:  // Ctors

    template< template< typename, typename... > class Container,
              typename ValueType,
              typename... OtherContainerArgs >
    PropertyTransient(const std::string& stopExpr,
                      const Container<ValueType, OtherContainerArgs...>& stopExprVars,
                      const std::string& goalExpr,
                      const Container<ValueType, OtherContainerArgs...>& goalExprVars);

    template< template< typename, typename... > class Iterator,
              typename ValueType,
              typename... OtherIteratorArgs >
    PropertyTransient(const std::string& stopExpr,
                      Iterator<ValueType, OtherIteratorArgs...> stopExprVarsFrom,
                      Iterator<ValueType, OtherIteratorArgs...> stopExprVarsTo,
                      const std::string& goalExpr,
                      Iterator<ValueType, OtherIteratorArgs...> goalExprVarsFrom,
                      Iterator<ValueType, OtherIteratorArgs...> goalExprVarsTo);

    /// Can't have empty ctor due to const data members
    PropertyTransient()                                         = delete;
    /// Can't have copy assignment due to const data members
    PropertyTransient& operator=(const PropertyTransient& that) = delete;
    /// Can't have move assignment due to const data members
    PropertyTransient& operator=(PropertyTransient&& that)      = delete;

public:  // Utils

     virtual bool satisfied_by(const StateInstance &s) const;
};

// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< template< typename, typename... > class Iterator,
          typename ValueType,
          typename... OtherIteratorArgs >
PropertyTransient::PropertyTransient(
    const std::string& stopExpr,
    Iterator<ValueType, OtherIteratorArgs...> stopExprVarsFrom,
    Iterator<ValueType, OtherIteratorArgs...> stopExprVarsTo,
    const std::string& goalExpr,
    Iterator<ValueType, OtherIteratorArgs...> goalExprVarsFrom,
    Iterator<ValueType, OtherIteratorArgs...> goalExprVarsTo) :
        Property(std::string("P( !(").append(stopExpr).append(") U ("
                                     .append(goalExpr).append(") )")),
                 PropertyType::TRANSIENT),
        stop(stopExpr, stopExprVarsFrom, stopExprVarsTo),
        goal(goalExpr, goalExprVarsFrom, goalExprVarsTo)
{

}

} // namespace fig

#endif // PROPERTYTRANSIENT_H

