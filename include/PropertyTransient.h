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
#include <ExpStateEvaluator.h>


namespace fig
{

/**
 * @copydoc TRANSIENT
 *
 * @details Transient properties describe finite execution traces.
 *          For instance "safety properties" described by the PCTL formula
 *          P(!stop U fail), which expresses the probability of remaining
 *          in "non-stopping" states until a "failure" is reached, are
 *          transient properties.<br>
 *          The general idea is to visit only states that satisfy a set of
 *          conditions (described by the logical expression "expr1", say),
 *          <i>until</i> a state that satisfies another set of conditions
 *          (described by "expr2", say) is visited.
 *          Execution is thus terminated when a state that doesn't satisfy
 *          "expr1" or that satisfies "expr2" is visited, whichever happens
 *          first.
 */
class PropertyTransient : public Property
{
    /// This should be continuously satisfied, otherwise the simulation is
    /// "prematurely interrupted" (it kinda failed)
    /// This is the subformula on the LHS of the '<i>until</i>'
    Property::Formula expr1_;

    /// When this becomes true the simulation reached its "final destination"
    /// (it kinda succeeded)
    /// This is the subformula on the RHS of the '<i>until</i>'
    Property::Formula expr2_;

public:  // Ctors

    /**
     * @brief Data ctor from generic lvalue containers
     *
     * @param expr1     Mathematical expression for the "expr1" subformula
     * @param expr1Vars Names of the variables ocurring in expr1
     * @param expr2     Mathematical expression for the "expr2" subformula
     * @param expr2Vars Names of the variables ocurring in expr2
     *
     * @throw FigException if "expr1" or "expr2" aren't valid expressions
     * \ifnot NRANGECHK
     *   @throw out_of_range if names of variables not appearing
     *                       in the expression strings were passed
     * \endif
     */
    PropertyTransient(shared_ptr<Exp> expr1, shared_ptr<Exp> expr2) :
        Property(PropertyType::TRANSIENT),
        expr1_(expr1),
        expr2_(expr2)
    {}


    // Copy/Move constructor deleted to avoid dealing with the unique id.
    PropertyTransient(const PropertyTransient& that) = delete;
    PropertyTransient(PropertyTransient&& that)      = delete;

    /// Can't have empty ctor due to const data members from Property
    PropertyTransient()                                         = delete;
    /// Can't have copy assignment due to const data members from Property
    PropertyTransient& operator=(const PropertyTransient& that) = delete;
    /// Can't have move assignment due to const data members from Property
    PropertyTransient& operator=(PropertyTransient&& that)      = delete;

public:  // Accessors

    void prepare(const PositionsMap& globalVars) override {
        expr1_.prepare(globalVars);
        expr2_.prepare(globalVars);
    }

    void prepare(const fig::State<STATE_INTERNAL_TYPE>& globalState)
    override {
        expr1_.prepare(globalState);
        expr2_.prepare(globalState);
    }

public:  // Utils

    std::string to_str() const override {
        return "[STRING OF PROPERTY TRANSIENT]";
    }


    /**
     * Is the "expr1" subformula satisfied by the given variables valuation?
     * @param s Valuation of the system's global state
     * @note To work with local states from the \ref ModuleInstace
     *       "system modules" use the State variant
     * @see PropertyTransient::expr1_
     */
    inline bool expr1(const StateInstance& s) const { return expr1_(s); }

    /**
     * Is the "expr1" subformula satisfied by the given state?
     * @param s The state of any Module (ModuleInstace or ModuleNetwork)
     * @note Slower than the StateInstance variant
     * @see PropertyTransient::expr1_
     */
    inline bool expr1(const State<STATE_INTERNAL_TYPE>& s) const {
        return expr1_(s);
    }

    /**
     * Is the "expr2" subformula satisfied by the given variables valuation?
     * @param sValuation of the system's global state
     * @note To work with local states from the \ref ModuleInstace
     *       "system modules" use the State variant
     * @see PropertyTransient::expr2_
     */
    inline bool expr2(const StateInstance& s) const {
        return expr2_(s);
    }

    /**
     * Is the "expr2" subformula satisfied by the given state?
     * @param s The state of any Module (ModuleInstace or ModuleNetwork)
     * @note Slower than the StateInstance variant
     * @see PropertyTransient::expr2_
     */
    inline bool expr2(const State<STATE_INTERNAL_TYPE>& s) const {
        return expr2_(s);
    }

    inline bool is_rare(const StateInstance& s) const override {
        return expr2(s);
    }

    inline bool is_rare(const State<STATE_INTERNAL_TYPE>& s)
    const override {
        return expr2(s);
    }

public: //Debug
    void print_info(std::ostream &out) const override;
};

} // namespace fig

#endif // PROPERTYTRANSIENT_H

