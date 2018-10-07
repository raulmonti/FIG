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
#include <memory>  // std::enable_shared_from_this<>
#include <string>
#include <array>
// FIG
#include <core_typedefs.h>
#include <State.h>
#include <MathExpression.h>


namespace fig
{

class ThresholdsBuilder;
class ThresholdsBuilderAdaptive;
class ThresholdsBuilderAdaptiveSimple;
class ModuleInstance;
class ModuleNetwork;
class Property;
class Traial;

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
class ImportanceFunction : public std::enable_shared_from_this<ImportanceFunction>
{
public:

	/// Mathematical formula to evaluate an algebraic expression,
	/// e.g. ad hoc function or combination of split importance values,
	/// to compute the importance of the <em>discrete state space</em>.
	class Formula : public MathExpression
	{
	public:
		/// Empty ctor
		Formula();

		/// Set internal mathematical expression to the given formula
		/// @param formula  String with mathematical expression to evaluate
		/// @param varNames Names of variables ocurring in exprStr
		/// @param obj      Either a global State<...> or a PositionsMap
		///                 mapping all names in 'varnames' to positions
		/// @throw FigException if badly formatted mathematical expression
		template< template< typename... > class Container,
							typename... OtherArgs,
				  class Mapper >
		void set(const std::string& formula,
				 const Container< std::string, OtherArgs... >& varNames,
				 const Mapper& obj);

		/// Reset internal mathematical expression to (void) creation values
		void reset() noexcept;

		/// Evaluate current formula expression on given symbolic state
		/// @throw mu::Parser::exception_type if undefined internal
		///        mathematical expression.
		/// @throw FigException if set() hasn't been called yet/last
		ImportanceValue operator()(const StateInstance& state) const;

		/// Evaluate current formula expression on given vector
		/// @throw mu::Parser::exception_type if undefined internal
		///        mathematical expression.
		/// @throw FigException if set() hasn't been called yet/last
		ImportanceValue operator()(const ImportanceVec& localImportances) const;

		/// Return the free variables (or modules) names occurring in our
		/// expression, viz. the 'varnames' from the last call to set()
		const std::vector< std::string >& get_free_vars() const noexcept;
	};

	/// Like Formula but for time, aka <em>continuous state space</em>:
	/// instances of this class operate on valuations of clock variables.
	class TimeFormula : public Formula
	{
	public:
		/// Evaluate current formula expression on given symbolic state
		/// and clocks valuations
		/// @copydetails Formula::operator()
		ImportanceValue operator()(const Traial& traial) const;
	};

public:  // Class attributes

	/// Long story short: number of concrete derived classes.
	/// More in detail this is the size of the array returned by names(), i.e.
	/// how many ImportanceFunction implementations are offered to the end user.
	static constexpr size_t NUM_NAMES = 3ul;

	/// Size of the array returned by strategies() as a constexpr, i.e.
	/// how many importance assessment strategies are offered to the end user.
	static constexpr size_t NUM_STRATEGIES = 3ul;

	/// Impose a limit on the amount of memory the user can request
	static constexpr size_t MAX_MEM_REQ = (1ul<<32ul);  // 4 GB

private:

	/// Name of the ImportanceFunction implemented by this instance.
	/// Check names() for available options.
	std::string name_;

protected:  // Attributes for derived classes

	/// Do we hold importance information about the states?
	bool hasImportanceInfo_;

	/// Can this instance be used for simulations?
	bool readyForSims_;

	/// Strategy used last to assess the importance with this function
	std::string strategy_;

	/// Technique used last to build the importance thresholds in this function
	std::string thresholdsTechnique_;

	/// Name of the SimulationEngine currently bound to the instance, if any
	mutable std::string simEngine_;

	/// Minimum importance currently held
	ImportanceValue minValue_;

	/// Maximum importance currently held
	ImportanceValue maxValue_;

	/// Importance of the rare state with lowest value
	ImportanceValue minRareValue_;

	/// Importance of the system's initial state
	ImportanceValue initialValue_;

	/**
	 * @brief Map from a <i>threshold-level</i> to the ImportanceValue
	 *        and splitting/effort that defines it.
	 *
	 *        The i-th <i>("threshold-") level</i> comprises all importance
	 *        values between threshold2importance_[i] (including it)
	 *        and threshold2importance_[i+1] (excluding it).<br>
	 *        The pair at the i-th position of this vector holds:
	 *        <ol>
	 *        <li>the minimum ImportanceValue in the i-th level,</li>
	 *        <li>the splitting/effort to perform on that level.</li>
	 *        </ol>
	 *
	 * @see importance2threshold_
	 */
	ThresholdsVec threshold2importance_;

	/**
	 * @brief Like threshold2importance_ but swapping threshold and importance
	 *
	 *        Map from the ImportanceValue of a state to a pair containing:
	 *        <ol>
	 *        <li>the ("threshold-") level that holds that importance,</li>
	 *        <li>the splitting/effort to perform on that level.</li>
	 *        </ol>
	 *
	 *  @see threshold2importance_
	 *  @note Built only when the importance range is "small"
	 */
	ThresholdsVec importance2threshold_;

	/// Minimum splitting/effort selected among all threshold levels
	unsigned long minThresholdsEffort_;

	/// Maximum splitting/effort selected among all threshold levels
	unsigned long maxThresholdsEffort_;

	/// @brief Algebraic formula defined by the user
	/// @note Useful both for ad hoc strategy and concrete_split functions
	Formula userFun_;

	/// @brief Algebraic formula on clocks defined by the user
	/// @details The value of this formula will be the <em>time factor</em> that
	///          comes from the "continuous state space," and is multiplied by
	///          the ImportanceValue that comes from the discrete state space
	/// @note This formula should yield values in the [0.0, 1.0] interval
	TimeFormula timeFun_;

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

	/// Names of the importance functions offered to the user,
	/// as he should requested them through the CLI.
	/// @note Implements the <a href="https://goo.gl/yhTgLq"><i>Construct On
	///       First Use</i> idiom</a> for static data members,
	///       to avoid the <a href="https://goo.gl/chH5Kg"><i>static
	///       initialization order fiasco</i>.
	static const std::array< std::string, NUM_NAMES >& names() noexcept;

	/// Importance assessment strategies offered to the user,
	/// as he should requested them through the CLI.
	/// @note Implements the <a href="https://goo.gl/yhTgLq"><i>Construct On
	///       First Use</i> idiom</a> for static data members,
	///       to avoid the <a href="https://goo.gl/chH5Kg"><i>static
	///       initialization order fiasco</i>.
	static const std::array< std::string, NUM_STRATEGIES>& strategies() noexcept;

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

	/// @copydoc simEngine_
	const std::string& sim_engine_bound() const noexcept;

	/// @returns Algebraic formula for ad hoc importance assessment if function
	///          has_importance_info() and current strategy is "adhoc",
	///          empty string otherwise
	const std::string adhoc_fun() const noexcept;

	/// @returns Time function expression if set, empty string otherwise
	/// @see timeFun_
	const std::string time_fun() const noexcept;

	/// @copydoc minValue_
	/// @param returnImportance Return min <b>importance value</b>
	///                         regardless of whether the thresholds are built
	/// @returns Zero if function doesn't has_importance_info(),
	///          last and lowest value assessed otherwise.
	/// @note If the \ref TresholdsBuilder::build_thresholds() "thresholds
	///       were already built" then the value returned will be the lowest
	///       <b>threshold level</b>.
	ImportanceValue min_value(bool returnImportance = false) const noexcept;

	/// @copydoc maxValue_
	/// @param returnImportance Return max <b>importance value</b>
	///                         regardless of whether the thresholds are built
	/// @returns Zero if function doesn't has_importance_info(),
	///          last and highest value assessed otherwise
	/// @note If the \ref TresholdsBuilder::build_thresholds() "thresholds
	///       were already built" the value returned will be the highest
	///       <b>threshold level</b>.
	/// @see num_thresholds()
	ImportanceValue max_value(bool returnImportance = false) const noexcept;

	/// @copydoc minRareValue_
	/// @param returnImportance Return min <b>importance value</b> of rare state
	///                         regardless of whether the thresholds are built
	/// @returns Zero if function doesn't has_importance_info(),
	///          last and lowest value assessed for a rare state otherwise.
	/// @note If the \ref TresholdsBuilder::build_thresholds() "thresholds
	///       were already built" the value returned will be the lowest
	///       <b>threshold level</b> containing a rare state.
	ImportanceValue min_rare_value(bool returnImportance = false) const noexcept;

	/// @copydoc initialValue_
	/// @param returnImportance Return initial <b>importance value</b>
	///                         regardless of whether the thresholds are built
	/// @returns Zero if function doesn't has_importance_info(),
	///          importance value of the system's initial state otherwise.
	/// @note If the \ref TresholdsBuilder::build_thresholds() "thresholds
	///       were already built" the value returned will be the <b>threshold
	///       level</b> containing the system's initial state.
	ImportanceValue initial_value(bool returnImportance = false) const noexcept;

	/// @copydoc thresholdsTechnique_
	/// @returns Empty string if function isn't ready(),
	///          last thresholds building technique used otherwise
	const std::string thresholds_technique() const noexcept;

	/// Number of thresholds built on last call to build_thresholds(),
	/// equal to max_value() after building the thresholds
	/// \ifnot NDEBUG
	///   @throw FigException if this instance isn't \ref ready()
	///                       "ready for simulations"
	/// \endif
	/// @see max_value()
	/// @see build_thresholds()
	unsigned num_thresholds() const;

	/// @copydoc threshold2importance_
	const ThresholdsVec& thresholds() const;

	/// @copydoc minThresholdEffort_
	/// @param dryrun If no thresholds are available, return a dummy value
	/// @returns 0 if function isn't ready(), else: min { effort per threshold }
	/// \ifnot NDEBUG
	///   @throw FigException if \p !dryrun and this instance isn't \ref ready()
	///                       "ready for simulations"
	/// \endif
	/// @see build_thresholds()
	unsigned long min_thresholds_effort(bool dryrun = false) const;

	/// @copydoc maxThresholdEffort_
	/// @param dryrun If no thresholds are available, return a dummy value
	/// @returns 0 if function isn't ready(), else: max { effort per threshold }
	/// \ifnot NDEBUG
	///   @throw FigException if \p !dryrun and this instance isn't \ref ready()
	///                       "ready for simulations"
	/// \endif
	/// @see build_thresholds()
	unsigned long max_thresholds_effort(bool dryrun = false) const;

	/// Post-processing applied to the \ref ImportanceValue "importance values"
	/// computed last; an empty first component means none was.
	virtual PostProcessing post_processing() const noexcept;

	/// @brief Whether the instance derives from ImportanceFunctionConcrete
	/// @details Concrete importance functions store info for the concrete state
	///          space as internal vectors of ImportanceValue.
	///          This can be taken advantage of during simulations by using the
	///          \ref ImportanceFunctionConcrete::info_of() "info_of()" member
	///          function they offer.
	/// @see concrete_simulation()
	virtual bool concrete() const noexcept = 0;

	/// @brief Whether the instance offers a reliable info_of() member function
	///        to use during simulations
	/// @details A concrete() importance functions may fail to tell properly
	///          when a global state is RARE (or STOP or whatever) via its
	///          \ref ImportanceFunctionConcrete::info_of() "info_of()" member
	///          function. This method tells whether its safe to use that
	///          function to identify special states during simulations.
	/// @see concrete()
	/// @note concrete_simulation() => concrete()
	virtual bool concrete_simulation() const noexcept = 0;

	/**
	 * Tell the pre-computed importance of the given StateInstance.
	 * @return ImportanceValue requested
	 * @note This returns the <em>crude importance</em> stripped of Event masks,
	 *       and considering only the discrete state space given by \p state
	 * \ifnot NDEBUG
	 *   @throw FigException if there's no \ref has_importance_info()
	 *                       "importance information" currently
	 * \endif
	 * @see importance_of(const Traial&)
	 */
	virtual ImportanceValue importance_of(const StateInstance& state) const = 0;

	/// @overload
	inline ImportanceValue importance_of(const State<STATE_INTERNAL_TYPE> &state) const
	    { return importance_of(state.to_state_instance()); }

	/// @overload
	/// @return ImportanceValue, from the discrete state space,
	///         multiplied by the time factor (from the continuous state space)<br>
	ImportanceValue importance_of(const Traial& traial) const;

	/// Evaluate the \ref timeFun_ "time formula" on the clocks of the traial
	inline float time_factor(const Traial& traial) const
//		{ return timeFun_(traial); }
	    {
		    /// @todo TODO erase debug (and uncomment line above)
		    const auto timeFactor = timeFun_(traial);
			static int N(10000);
			if (N-- > 0)
				std::cerr << ", tf:" << timeFactor;
			return timeFactor;
	    }

	/**
	 * Threshold level to which given StateInstance belongs.
	 * @note The j-th threshold level is composed of all the states to which
	 *       the ImportanceFunction assigns an ImportanceValue between the
	 *       values of threshold 'j' (included) and 'j+1' (excluded)
	 * @note <b>Complexity:</b> <i>O(log(num_thresholds()))</i>
	 * \ifnot NDEBUG
	 *   @throw FigException if this instance isn't \ref ready()
	 *                       "ready for simulations"
	 * \else
	 *   @warning If this instance isn't \ref ready() "ready for simulations"
	 *            then 0 is always returned
	 * \endif
	 * @see ThresholdsBuilder::build_thresholds()
	 * @see level_of(const Traial&)
	 */
	inline ImportanceValue level_of(const StateInstance& state) const
		{
			assert(has_importance_info());
			return (!importance2threshold_.empty())
			        ? importance2threshold_[importance_of(state)].first  // use direct map
			        : level_of(importance_of(state));  // search threshold level
		}

	/// @overload
	/// @brief Threshold level to which the given ImportanceValue belongs.
	ImportanceValue level_of(const ImportanceValue& imp) const;

	/// @overload
	/// @note Consider also the clocks: multiply the ImportanceValue (from the
	///       discrete state space) by the time factor (from the continuous
	///       state space)
	inline ImportanceValue level_of(const Traial& traial) const
	    {
		    assert(has_importance_info());
			return (!importance2threshold_.empty())
			        ? importance2threshold_[importance_of(traial)].first  // use direct map
			        : level_of(importance_of(traial));  // search threshold level
		}

	/**
	 * Splitting/effort associated with this ("threshold-") level
	 * @param lvl  Threshold-level whose associated effort is queried
	 * \ifnot NDEBUG
	 *   @throw FigException if this instance isn't \ref ready()
	 *                       "ready for simulations"
	 * \else
	 *   @warning If this instance isn't \ref ready() "ready for simulations"
	 *            then 0 is always returned
	 * \endif
	 * @see level_of()
	 */
	unsigned long effort_of(const ImportanceValue& lvl) const;

	/**
	 * @brief Print (formatted) importance information
	 * @details States are printed along their importance (or threshold level)
	 *          If events masks are present they are marked,
	 *          and a legend is included to interpret the marking.
	 * @param out Output stream where printing will take place
	 * @param s   Global system state, i.e. with all variables of the model
	 * @warning This can be <b>a lot</b> of printing, use with care.
	 */
	virtual void print_out(std::ostream& out,
	                       State<STATE_INTERNAL_TYPE> s = State<STATE_INTERNAL_TYPE>()) const = 0;

public:  // Utils


	/**
	 * @brief Set a TimeFormula to scale importance as (simulation) time elapses
	 *
	 *        Set a new mathematical formula to compute the time factor
	 *        from the system clocks (aka the continuous state space).<br>
	 *        During simulations, to evaluate the importance of the system
	 *        state, this time factor will be multiplied by the
	 *        ImportanceValue of the variables (aka the discrete state space).
	 *
	 * @param formulaExprStr String with the mathematical expression
	 * @param allClocksNames Names of all clocks in the system, in the same
	 *                       order in which they are stored in Traial.clocks_
	 *
	 * @throw FigException if badly formatted \p formulaExprStr or if
	 *                     \p allClocksNames has names not appearing in
	 *                     \p formulaExprStr
	 *
	 * @see timeFun_
	 */
	template< template< typename... > class Container, typename... OtherArgs >
	void set_time_factor(const std::string& formulaExprStr,
	                     const Container< std::string, OtherArgs... >& allClocksNames);

	/// Register SimulationEngine called \p name as currently bound to this intance
	void bind_sim_engine(const std::string& name) const;

	/// Deregister any SimulationEngine currently bound to this instance
	void unbind_sim_engine() const;

	/**
	 * @brief Build thresholds from precomputed importance information
	 *
	 *        This fills up the threshold2importance_ vector member. After a
	 *        successfull call this instance is \ref ImportanceFunction::ready()
	 *        "ready for simulations": the \ref SimulationEngine "simulation
	 *        engines" will use these thresholds when coupled with this
	 *        ImportanceFunction.
	 *
	 * @warning It may be needed to \ref ThresholdsBuilder::setup() "setup
	 *          the ThresholdsBuilder" before calling this function.
	 *
	 * @throw FigException if there was no precomputed \ref has_importance_info()
	 *                     "importance information"
	 *
	 * @see ready()
	 * @see ThresholdsBuilder
	 */
	void build_thresholds(ThresholdsBuilder& tb);

	/// Fetch random sample of ImportanceValue
	/// @param s Any global state of the system
	/// @param numValues Max number of importance values to look for
	/// @return (Possibly empty!) Vector of valid state importance values
	/// @note No value will appear repeated, e.g. as in a set
	/// @throw FigException if there was no precomputed \ref has_importance_info()
	///                     "importance information"
	std::vector<ImportanceValue>
	random_sample(State<STATE_INTERNAL_TYPE> s, size_t numValues = 256ul) const;

	/// @copydoc random_sample()
	/// @return (Possibly empty!) Set of valid (state, importance value) pairs
	std::set<std::pair<uint128_t, ImportanceValue> >
	random_sample2(State<STATE_INTERNAL_TYPE> s, size_t numValues = 256ul) const;

	/**
	 * @brief Release memory allocated in the heap during importance assessment
	 *
	 *        This destroys any importance and thresholds info:
	 *        the ImportanceFunction won't hold \ref has_importance_info()
	 *        "importance information" any longer and will thus not be
	 *        \ref ready() "ready for simulations" either.
	 *
	 * @note Non-const static members of the class are also reset
	 */
	virtual void clear() noexcept;

private:  // Class utils

	/// Try to optimize the storage of the thresholds that have been chosen
	/// @param tb ThresholdsBuilder last used
	/// @see build_thresholds()
	void post_process_thresholds(const ThresholdsBuilder& tb);

protected:  // Utils for derived classes

	/**
	 * @brief Find extreme \ref ImportanceValue "importance values" for the
	 *        current importance assessment of this ImportanceFunction.
	 *
	 *        On successfull invocation the values of the internal attributes
	 *        minImportance_, maxImportance_ and minRareImportance_ are left
	 *        as they should for the importance information currently held.
	 *
	 * @param state    State whose <b>whole concrete space</b> will be explored
	 * @param property Property identifying the rare State valuations
	 *
	 * @note <b>Complexity:</b> <i>O(state.concrete_size() * state.size())</i>
	 *
	 * @throw FigException if there was no \ref has_importance_info()
	 *                     "importance information"
	 *
	 * @warning Takes too long for large state spaces
	 */
	void find_extreme_values(State<STATE_INTERNAL_TYPE> state,
							 const Property& property);
};

} // namespace fig

#endif // IMPORTANCEFUNCTION_H

