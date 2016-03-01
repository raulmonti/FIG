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
public:

	typedef std::vector< ImportanceValue > ImportanceVec;

protected:

	/// Mathematical formula to evaluate an algebraic expression,
	/// e.g. ad hoc function or combination of split importance values.
	class Formula : public MathExpression
	{
	public:

		/// Empty ctor
		Formula();

		/// Set internal mathematical expression to the given formula
		/// @param formula  String with mathematical expression to evaluate
		/// @param varnames Names of variables ocurring in exprStr
		/// @param obj      Either a global State<...> or a PositionsMap
		///                 mapping all names in 'varnames' to positions
		/// @throw FigException if badly formatted mathematical expression
		template< template< typename... > class Container,
							typename... OtherArgs,
				  class Mapper >
		void set(const std::string& formula,
				 const Container< std::string, OtherArgs... >& varnames,
				 const Mapper& obj);

		/// Reset internal mathematical expression to (void) creation values
		void reset() noexcept;

		/// Evaluate current formula expression on given symbolic state
		/// @throw mu::Parser::exception_type if undefined internal
		///        mathematical expression.
		ImportanceValue operator()(const StateInstance& state) const;

		/// Evaluate current formula expression on given vector
		/// @throw mu::Parser::exception_type if undefined internal
		///        mathematical expression.
		ImportanceValue operator()(const ImportanceVec& localImportances) const;
	};

public:  // Class attributes

	/// Names of the importance functions offered to the user,
	/// as he should requested them through the CLI/GUI.
	/// Defined in ImportanceFunction.cpp
	static const std::array< std::string, 3 > names;

	/// Importance assessment strategies offered to the user,
	/// as he should requested them through the CLI/GUI.
	/// Defined in ImportanceFunction.cpp
	static const std::array< std::string, 3 > strategies;

private:

	/// Importance function implemented by this instance
	/// Check ImportanceFunction::names for available options.
	std::string name_;

protected:  // Attributes for derived classes

	/// Do we hold importance information about the states?
	bool hasImportanceInfo_;

	/// Can this instance be used for simulations?
	bool readyForSims_;

	/// Last used strategy to assess the importance with this function
	std::string strategy_;

	/// Last used technique to build the importance thresholds in this function
	std::string thresholdsTechnique_;

	/// Minimum importance assigned during the last assessment
	ImportanceValue minImportance_;

	/// Maximum importance assigned during the last assessment
	ImportanceValue maxImportance_;

	/// Importance of the rare state with lowest importance from last assessment
	ImportanceValue minRareImportance_;

	/// Number of thresholds built on last call to build_thresholds()
	unsigned numThresholds_;

	/// Algebraic formula defined by the user.
	/// Useful both for ad hoc strategy and concrete_split functions
	Formula userFun_;

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

	/// @returns Algebraic formula for ad hoc importance assessment if function
	///          has_importance_info() and current strategy is "adhoc",
	///          empty string otherwise
	const std::string adhoc_fun() const noexcept;

	/// @copydoc minImportance_
	/// @returns Zero if function doesn't has_importance_info(),
	///          last minimum ImportanceValue assessed importance otherwise
	/// @note If the \ref TresholdsBuilder::build_thresholds() "thresholds were
	///       built" "in situ" the value returned will be that of the lowest
	///       threshold level.
	ImportanceValue min_importance() const noexcept;

	/// @copydoc maxImportance_
	/// @returns Zero if function doesn't has_importance_info(),
	///          last maximum ImportanceValue assessed importance otherwise
	/// @note If the \ref TresholdsBuilder::build_thresholds() "thresholds were
	///       built" "in situ" the value returned will be that of the highest
	///       threshold level.
	ImportanceValue max_importance() const noexcept;

	/// @copydoc minRareImportance_
	/// @returns Zero if function doesn't has_importance_info(),
	///          minimum ImportanceValue of a rare state otherwise
	/// @note If the \ref TresholdsBuilder::build_thresholds() "thresholds were
	///       built" "in situ" the value returned will be the lowest threshold
	///       level containing a rare state.
	ImportanceValue min_rare_importance() const noexcept;

	/// Whether this instance keeps an internal std::vector<ImportanceValue>,
	/// i.e. has info for the concrete state space,
	///      as opposed to the symbolic state space.
	virtual bool concrete() const noexcept = 0;

	/// @copydoc thresholdsTechnique_
	/// @returns Empty string if function isn't ready(),
	///          last thresholds building technique used otherwise
	const std::string thresholds_technique() const noexcept;

	/// @copydoc numThresholds_
	/// @throw FigException if this instance isn't \ref ready()
	///                     "ready for simulations"
	const unsigned& num_thresholds() const;

	/**
	 * Tell the pre-computed importance of the given StateInstance.
	 * @return ImportanceValue requested
	 * @note This returns the importance alone without any kind of Event mask
	 * \ifnot NDEBUG
	 *   @throw FigException if there's no \ref has_importance_info()
	 *                       "importance information" currently
	 * \endif
	 */
	virtual ImportanceValue importance_of(const StateInstance& state) const = 0;

	/**
	 * Threshold level to which given StateInstance belongs.
	 * @note The j-th threshold level is composed of all the states to which
	 *       the ImportanceFunction assigns an ImportanceValue between the
	 *       values of threshold 'j' (included) and 'j+1' (excluded)
	 * \ifnot NDEBUG
	 *   @throw FigException if this instance isn't \ref ready()
	 *                       "ready for simulations"
	 * \endif
	 * @see ThresholdsBuilder::build_thresholds()
	 */
	virtual ImportanceValue level_of(const StateInstance& state) const = 0;

	/**
	 * Threshold level to which given ImportanceValue belongs.
	 * @note The j-th threshold level is composed of all the states to which
	 *       the ImportanceFunction assigns an ImportanceValue between the
	 *       values of threshold 'j' (included) and 'j+1' (excluded)
	 * \ifnot NDEBUG
	 *   @throw FigException if this instance isn't \ref ready()
	 *                       "ready for simulations"
	 * \endif
	 * @see ThresholdsBuilder::build_thresholds()
	 */
	virtual ImportanceValue level_of(const ImportanceValue& val) const = 0;

	/**
	 * @brief Print formatted internal importance information
	 * @details States are printed along their importance (or threshold level)
	 *          If events masks are present they are somehow marked,
	 *          and a legend is included to interpret the marking.
	 * @param out Output stream where printing will take place
	 * @param s   Global system state, i.e. with all variables of the model
	 * @warning This can be <b>a lot</b> of printing, use with care.
	 */
	virtual void print_out(std::ostream& out, State<STATE_INTERNAL_TYPE> s) const = 0;

public:  // Utils

	/**
	 * @brief Build thresholds from precomputed importance information
	 *
	 *        The thresholds may be kept separately or built on top of the
	 *        importance information. In any case after a successfull call
	 *        this instance is \ref ImportanceFunction::ready() "ready for
	 *        simulations": \ref SimulationEngine "simulation engines" will use
	 *        the thresholds info when coupled with this ImportanceFunction.
	 *
	 * @param tb                 ThresholdsBuilder to use
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

	/// @brief  Release memory allocated in the heap during importance assessment
	/// @details This destroys any importance and thresholds info:
	///          the ImportanceFunction won't hold \ref has_importance_info()
	///          "importance information" any longer and will thus not be
	///          \ref ready() "ready for simulations" either.
	virtual void clear() noexcept;

protected:  // Utils for derived classes

	/**
	 * @brief Find extreme \ref ImportanceValue "importance values" for the
	 *        current importance assessment of this ImportanceFunction.
	 *
	 *        On successfull invocation the values of the internal attributes
	 *        minImportance_, maxImportance_ and minRareImportance_ are left
	 *        as they should for the importance information currently held.
	 *
	 * @param state    State whose whole concrete space will be explored
	 * @param property Property identifying the rare State valuations
	 *
	 * @note <b>Complexity:</b> <i>O(state.concrete_size() * state.size())</i>
	 *
	 * @throw FigException if there was no \ref has_importance_info()
	 *                     "importance information"
	 */
	void find_extreme_values(State<STATE_INTERNAL_TYPE> state,
							 const Property& property);
};

} // namespace fig

#endif // IMPORTANCEFUNCTION_H

