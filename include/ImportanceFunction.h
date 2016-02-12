//==============================================================================
//
//  ImportanceFunction.h
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


#ifndef IMPORTANCEFUNCTION_H
#define IMPORTANCEFUNCTION_H

// C++
#include <ostream>
#include <string>
#include <array>
// FIG
#include <core_typedefs.h>
#include <State.h>
#include <MathExpression.h>


namespace fig
{

class ThresholdsBuilder;
class ModuleInstance;
class ModuleNetwork;
class Property;

/**
 * @brief Abstract base importance assessor (or function)
 *
 *        Importance functions are required for the application of
 *        importance splitting techniques during Monte Carlo simulations.
 *        Based on an identifyable set of "rare states", importance functions
 *        are in charge of assessing how likely it is to visit such set
 *        from <i>each other</i> (reachable) system state.
 *
 *        Besides from the "name" which specifies the ImportanceFunction
 *        derived class, importance assessment requires the choice of a
 *        "strategy" (flat, auto, ad hoc...) to decide how the relative
 *        importance between states will be measured.
 */
class ImportanceFunction
{
protected:

	/// Mathematical formula to evaluate the importance of symbolic states
	class Formula : public MathExpression
	{
		Formula();

		/// Reset internal mathematical expression to the given formula
		/// @param formula     String with mathematical expression to evaluate
		/// @param varnames    Names of variables ocurring in exprStr
		/// @param globalState State of the whole system model
		/// @throw FigException if badly formatted mathematical expression
		/// @throw out_of_range if 'varnames' has names not in exprStr
		void reset(const std::string& formula,
				   const std::vector< std::string >& varnames,
				   const State<STATE_INTERNAL_TYPE>& globalState);

		/// Evaluate current formula expression on given symbolic state
		STATE_INTERNAL_TYPE operator()(const StateInstance& state) const;
	};

public:

	/// Names of the importance functions offered to the user,
	/// as he should requested them through the CLI/GUI.
	/// Defined in ImportanceFunction.cpp
	static const std::array< std::string, 2 > names;

	/// Importance assessment strategies offered to the user,
	/// as he should requested them through the CLI/GUI.
	/// Defined in ImportanceFunction.cpp
	static const std::array< std::string, 4 > strategies;

private:

	/// Importance function implemented by this instance
	/// Check ImportanceFunction::names for available options.
	std::string name_;

protected:

	/// Do we hold importance information to assess the states' importance?
	bool hasImportanceInfo_;

	/// Can this instance be used for simulations?
	bool readyForSims_;

	/// Last used strategy to assess the importance with this function
	std::string strategy_;

	/// Last used technique to build the importance thresholds in this function
	std::string thresholdsTechnique_;

	/// Maximum importance assigned during the last assessment
	ImportanceValue maxImportance_;

	/// Importance of the rare state with lowest importance from last assessment
	ImportanceValue minRareImportance_;

	/// Algebraic formula for ad hoc importance strategy
	Formula assessor_;

public:  // Ctor/Dtor

	/**
	 * Data ctor
	 * @param name @copydoc name_
	 * @throw FigException if the name doesn't match a valid function
	 */
	ImportanceFunction(const std::string& name);

	/// Dtor
	virtual ~ImportanceFunction() {}

public:  // Accessors

	/// @copydoc name_
	const std::string& name() const noexcept;

	/**
	 * @copydoc hasImportanceInfo_
	 *
	 *  This becomes true only after a successfull call to either
	 *  ImportanceFunctionConcrete::assess_importance() or
	 *  ImportanceFunctionAlgebraic::set_formula(),
	 *  depending on the derived class this object is an instance of.<br>
	 *  It becomes false again after a call to clear()
	 */
	bool has_importance_info() const noexcept;

	/**
	 * @copydoc readyForSims_
	 *
	 *  This requires having had the \ref build_thresholds() "thresholds built"
	 *  in addition to holding \ref has_importance_info() "importance information"
	 *
	 * @see has_importance_info()
	 */
	bool ready() const noexcept;

	/// @copydoc strategy_
	/// @returns Empty string if function doesn't has_importance_info(),
	///          last used strategy otherwise
	const std::string strategy() const noexcept;

	/// @copydoc maxImportance_
	/// @returns Zero if function doesn't has_importance_info(),
	///          last maximum ImportanceValue assessed importance (or threshold level) otherwise
	ImportanceValue max_importance() const noexcept;

	/// @copydoc minRareImportance_
	/// @returns Zero if function doesn't has_importance_info(),
	///          minimum ImportanceValue (or threshold level) of a rare state otherwise
	ImportanceValue min_rare_importance() const noexcept;

	/// Whether this instance keeps an internal std::vector<ImportanceValue>,
	/// i.e. has info for the concrete state space,
	///      as opposed to the symbolic state space.
	virtual bool concrete() const noexcept = 0;

	/// @copydoc thresholdsTechnique_
	/// @returns Empty string if function isn't ready(),
	///          last thresholds building technique used otherwise
	const std::string thresholds_technique() const noexcept;

	/// Number of thresholds built on last call to build_thresholds()
	/// @throw FigException if this instance isn't \ref ready()
	///                     "ready for simulations"
	unsigned num_thresholds() const;

	/**
	 * Tell the pre-computed importance of the given StateInstance.
	 * @return ImportanceValue requested
	 * \ifnot NDEBUG
	 *   @throw FigException if there's no \ref has_importance_info()
	 *                       "importance information" currently
	 * \endif
	 */
	virtual ImportanceValue importance_of(const StateInstance& state) const = 0;

	/**
	 * Tell the threshold level to which given StateInstance belongs.
	 * @return ImportanceValue requested
	 * \ifnot NDEBUG
	 *   @throw FigException if this instance isn't \ref ready()
	 *                       "ready for simulations"
	 * \endif
	 */
	virtual ImportanceValue level_of(const StateInstance& state) const = 0;

	/**
	 * @brief Print formatted internal importance information
	 * @details States are printed along their importance (or threshold level)
	 *          If events masks are present they are somehow marked,
	 *          and a legend is included to interpret the marking.
	 * @param out Output stream where printing will take place
	 * @warning This can be <b>a lot</b> of printing, use with care.
	 */
	virtual void print_out(std::ostream& out) const = 0;

public:  // Utils

	/**
	 * @brief Build thresholds from precomputed importance information
	 *
	 *        The thresholds are built in the ImportanceFunction itself,
	 *        smashing the finely grained importance values and replacing them
	 *        with coarsely grained threshold levels.
	 *        After a successfull call this ImportanceFunction instance is
	 *        \ref ImportanceFunction::ready() "ready for simulations".
	 *
	 * @param tb  ThresholdsBuilder to use
	 * @param splitsPerThreshold 1 + Number of simulation-run-replicas upon a
	 *                           "threshold level up" event
	 *
	 * @throw FigException if there was no precomputed \ref has_importance_info()
	 *                     "importance information"
	 *
	 * @see ready()
	 */
	virtual void build_thresholds(ThresholdsBuilder& tb,
								  const unsigned& splitsPerThreshold) = 0;

	/**
	 * @brief Release any memory allocated in the heap
	 * @note After this invocation the ImportanceFunction doesn't hold
	 *       \ref has_importance_info() "importance information" any longer
	 *       and it's thus not \ref ready() "ready for simulations"
	 */
	virtual void clear() noexcept = 0;
};

} // namespace fig

#endif // IMPORTANCEFUNCTION_H

