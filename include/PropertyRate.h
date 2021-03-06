//==============================================================================
//
//  PropertyRate.h
//
//  Copyleft 2016-
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
    Precondition condition_;

public:  // Ctors

    /**
     * @brief Data ctor from generic lvalue container
     *
     * @param expr     Mathematical expression for the only subformula
     *
     * @throw FigException if "expr" isn't a valid expressions
     * \ifnot NRANGECHK
     *   @throw out_of_range if names of variables not appearing
     *                       in the expression string were passed
     * \endif
     */
	PropertyRate(std::shared_ptr<Exp> expr) :
        Property(PropertyType::RATE),
	    condition_(expr)
	{ /* Not much to do around here... */ }

    /// Copy/Move constructor deleted to avoid dealing with the unique id.
    PropertyRate(const PropertyRate& that) = delete;
    PropertyRate(PropertyRate&& that)      = delete;

	/// No empty ctor due to const data members from Property
	PropertyRate()                                    = delete;
	/// No copy assignment due to const data members from Property
    PropertyRate& operator=(const PropertyRate& that) = delete;
	/// No move assignment due to const data members from Property
    PropertyRate& operator=(PropertyRate&& that)      = delete;

	inline ~PropertyRate() override {}

public:  // Utils

	void prepare(const State<STATE_INTERNAL_TYPE>& state) override
	    { condition_.prepare(state); }

	void prepare(const PositionsMap& posMap) override
	    { condition_.prepare(posMap); }

	std::string to_string() const override
	    {
		    return "S( ("
			        + condition_.get_expression()->to_string()
			        + ") / total_time )";
	    }

public:  // Accessors

    /**
     * Is the subformula satisfied by the given variables valuation?
     * @param s Valuation of the system's global state
     * @note To work with local states from the \ref ModuleInstace
     *       "system modules" use the State variant
	 * @see PropertyRate::condition
     */
	inline bool expr(const StateInstance& s) const
	    { return condition_(s); }

    /**
     * Is the subformula satisfied by the given state?
     * @param s The state of any Module (ModuleInstace or ModuleNetwork)
     * @note Slower than the StateInstance variant
	 * @see PropertyRate::condition
     */
	inline bool expr(const State<STATE_INTERNAL_TYPE>& s) const
	    { return condition_(s); }

	inline bool is_rare(const StateInstance& s) const override
	    { return condition_(s); }

	inline bool is_rare(const State<STATE_INTERNAL_TYPE>& s) const override
	    { return condition_(s); }

public:  // Debug

    void print_info(std::ostream &out) const override;
};

} // namespace fig

#endif // PROPERTYRATE_H
