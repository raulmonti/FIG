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
#include <Precondition.h>
#include <State.h>


namespace fig
{

/**
 * @brief Abstract base logical property
 *
 *        Properties are what the user wants to study about the system model
 *        provided. There are different kinds of them but in general they
 *        speak about the propability of certain events chain taking place.
 *        FIG estimates these values through efficient simulation.
 *
 * @see PropertyType
 */
class Property
{
    friend class ModelSuite;  // for variables mapping callback

protected:

    /// Nasty hack to avoid code duplication (talk of poor design...)
    typedef  Precondition  Formula;

public:  // Attributes

    /// Property per se in string form
    const std::string expression;

    /// Which type of property it is
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

protected:  // Modifyers

	/**
	 * @copydoc fig::MathExpression::pin_up_vars(const PositionMap&)
	 * \ifnot NDEBUG
	 *   @throw FigException if there was some error in our math expressions
	 * \endif
	 */
	virtual void pin_up_vars(const PositionsMap &globalVars) = 0;

	/**
	 * @copydoc fig::MathExpression::pin_up_vars()
	 * \ifnot NDEBUG
	 *   @throw FigException if there was some error in our math expressions
	 * \endif
	 */
	virtual void pin_up_vars(const fig::State<STATE_INTERNAL_TYPE>& globalState) = 0;

public:  // Utils

    /**
     * @brief Is the property satisfied by the given variables valuation?
     *
     * @param s  Valuation of the module/model variables to use
     *
     * @note pin_up_vars() should have been called before to register the
     *       position of the expression's variables in the global State
     *
     * @throw mu::ParserError
     * @ifnot NDEBUG
     *   @throw FigException if pin_up_vars() hasn't been called yet
     * @endif
     */
    virtual bool satisfied_by(const StateInstance& s) const = 0;

    /// @copydoc satisfied_by()
    virtual bool satisfied_by(const State<STATE_INTERNAL_TYPE>& s) const = 0;

    /**
     * @brief Is this state considered "rare" for importance simulation?
     *
     * @param s  Valuation of the module/model variables to use
     *
     * @note pin_up_vars() should have been called before to register the
     *       position of the expression's variables in the global State
     *
     * @throw mu::ParserError
     * @ifnot NDEBUG
     *   @throw FigException if pin_up_vars() hasn't been called yet
     * @endif
     */
    virtual bool is_rare(const StateInstance& s) const = 0;

    /// @copydoc is_rare()
    virtual bool is_rare(const State<STATE_INTERNAL_TYPE>& s) const = 0;
};

} // namespace fig

#endif // PROPERTY_H

