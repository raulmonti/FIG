//==============================================================================
//
//  PropertyTBoundSS.h
//
//  Copyleft 2019-
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

#ifndef PROPERTYTBOUNDSS_H
#define PROPERTYTBOUNDSS_H

#include <Property.h>

namespace fig
{

/**
 * @copydoc TBOUNDED_SS
 *
 * @details Time-Bounded steady-state properties are essentially
 *          \ref PropertyRate "rate poperties" with explicit time bounds,
 *          used to determine the transient phase and the batch size:
 *          <ul>
 *	        <li> the lower time value indicates the transient phase to be
 *               discarded: in a simulation run, the subformula \p expr
 *               starts being monitored only after time > \p Tlow.
 *          </li>
 *          <li> the upper time value indicates the (time) length of a batch:
 *               a simulation is truncated as soon as its time > \p Tupp,
 *               so batches have (simulation) time length \p Tupp - \p Tlow.
 *          </li>
 *          </ul>
 *          This way, the \ref ConfidenceInterval "confidence interval"
 *          is updated with samples that measure the proportion of time that
 *          the condition \p expr is true, in the time period \p Tupp - \p Tlow.
 */
class PropertyTBoundSS : public Property
{
	/// Lower time bound, from which condition_ starts being monitored
	long tbound_low_;

	/// Upper time bound, after which simulations stop (to start a new batch)
	long tbound_upp_;

	/// This identifies the special states whose visiting times are monitored
	Precondition condition_;

public:  // Ctors

	/**
	 * @brief Data ctor from generic lvalue containers
	 *
	 * @param tbl   Constant MathExpression for the lower time-bound
	 * @param tbu   Constant MathExpression for the upper time-bound
	 * @param expr  Mathematical expression for the only subformula
	 *
	 * @throw FigException if "expr" isn't a valid expressions
	 * \ifnot NRANGECHK
	 *   @throw out_of_range if names of variables not appearing
	 *                       in the expression string were passed
	 * \endif
	 */
	PropertyTBoundSS(std::shared_ptr<IConst> tbl,
	                 std::shared_ptr<IConst> tbu,
	                 std::shared_ptr<Exp> expr) :
	    Property(PropertyType::TBOUNDED_SS),
	    tbound_low_(tbl->get_value()),
	    tbound_upp_(tbu->get_value()),
	    condition_(expr)
	{
		assert(0 < tbound_low_);
		assert(tbound_low_ < tbound_upp_);
	}

	/// Copy/Move constructor deleted to avoid dealing with the unique id.
	PropertyTBoundSS(const PropertyTBoundSS& that) = delete;
	PropertyTBoundSS(PropertyTBoundSS&& that)      = delete;

	/// No empty ctor due to const data members from Property
	PropertyTBoundSS()                                        = delete;
	/// No copy assignment due to const data members from Property
	PropertyTBoundSS& operator=(const PropertyTBoundSS& that) = delete;
	/// No move assignment due to const data members from Property
	PropertyTBoundSS& operator=(PropertyTBoundSS&& that)      = delete;

	inline ~PropertyTBoundSS() override {}

public:  // Utils

	inline void prepare(const State<STATE_INTERNAL_TYPE>& state) override
	    { condition_.prepare(state); }

	inline void prepare(const PositionsMap& posMap) override
	    { condition_.prepare(posMap); }

	inline std::string to_string() const override
	    {
		    return "S ["
			        + std::to_string(tbound_low_)
			        + ":"
			        + std::to_string(tbound_upp_)
			        + "]( ("
			        + condition_.get_expression()->to_string()
			        + ") / total_time )";
	    }

public:  // Accessors

	/** Lower time bound */
	inline decltype(tbound_low_) tbound_low() const
	    { return tbound_low_; }

	/** Upper time bound */
	inline decltype(tbound_upp_) tbound_upp() const
	    { return tbound_upp_; }

	/**
	 * Is the subformula satisfied by the given variables valuation?
	 * @param s Valuation of the system's global state
	 * @note To work with local states from the \ref ModuleInstace
	 *       "system modules" use the State variant
	 * @see PropertyTBoundSS::expr_
	 */
	inline bool expr(const StateInstance& s) const
	    { return condition_(s); }

	/**
	 * Is the subformula satisfied by the given state?
	 * @param s The state of any Module (ModuleInstace or ModuleNetwork)
	 * @note Slower than the StateInstance variant
	 * @see PropertyTBoundSS::expr_
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

#endif // PROPERTYTBOUNDSS_H
