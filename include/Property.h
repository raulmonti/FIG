//==============================================================================
//
//  Property.h
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


#ifndef PROPERTY_H
#define PROPERTY_H

// C++
#include <string>
// FIG
#include <core_typedefs.h>
#include <State.h>


namespace fig
{

/**
 * @brief Abstract base logical property
 *
 *        Properties are what the user wants to study about the system model
 *        he provides. There are different kinds of them but in general they
 *        speak about the propability of certain events chain taking place.
 *        FIG estimates these values through efficient simulation.
 *
 * @see PropertyType
 */
class Property
{

public:  // Attributes

    // Property per se in string form
    const std::string expression;

    // Which type of property is it
    const PropertyType type;

public:  // Ctors

    Property(const std::string& theexpression, PropertyType thetype) :
        expression(theexpression),
        type(thetype)
        {}
    Property(const Property& that) = default;
    Property(Property&& that)      = default;

    /// Can't have empty ctor due to const data members
    Property()                                = delete;
    /// Can't have copy assignment due to const data members
    Property& operator=(const Property& that) = delete;
    /// Can't have move assignment due to const data members
    Property& operator=(Property&& that)      = delete;

public:  // Utils

    /// Is the property satisfied by the given valuation of the variables?
    virtual bool satisfied_by(const StateInstance& s) const = 0;
};

} // namespace fig

#endif // PROPERTY_H

