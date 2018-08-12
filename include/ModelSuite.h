//==============================================================================
//
//  ModelSuite.h
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

#ifndef MODELSUITE_H
#define MODELSUITE_H

// C++
#include <mutex>  // std::call_once(), std::once_flag
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <iterator>     // std::begin(), std::end(), std::distance()
#include <type_traits>  // std::is_constructible<>
#include <unordered_map>
#include <functional>
// FIG
#include <ModuleNetwork.h>
#include <ImportanceFunction.h>
#include <ImportanceFunctionConcrete.h>
#include <ThresholdsBuilder.h>
#include <SimulationEngine.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

// ADL
using std::distance;
using std::begin;
using std::end;


namespace fig
{

class Property;
class ThresholdsBuilder;
class StoppingConditions;
class ConfidenceIntervalResult;
class SignalSetter;

/**
 * @brief One class to bring them all, and in the FIG tool bind them.
 *
 *        ModelSuite holds most components required for the estimation of the
 *        properties values on the user provided system model.
 *        It is FIG's general access point for launching and controlling
 *        simulations, as well as recording the resulting data.
 *
 * @note  There should be exactly one ModelSuite at all times,
 *        which starts out empty and gets filled with \ref Property
 *        "properties" and \ref ModuleInstance "module instances"
 *        as these are parsed and created. For that reason this class
 *        follows the
 *        <a href="https://sourcemaking.com/design_patterns/singleton">
 *        singleton design pattern</a>. It was implemented using C++11
 *        facilities to make it
 *        <a href="http://silviuardelean.ro/2012/06/05/few-singleton-approaches/">
 *        thread safe</a>.
 */
class ModelSuite
{
	friend class Traial;

	/// Network of user-defined modules, viz. the system model
	static std::shared_ptr< ModuleNetwork > model;

	/// Properties to estimate
	static std::vector< std::shared_ptr< Property > > properties;

	/// Global effort used by all splitting engines
	/// @note Relevant only when a \ref ThresholdsBuilderAdaptiveSimple
	///       "global-effort thresholds builder" is used
	static unsigned globalEffort;

	/// Does the \ref ModuleNetwork "IOSA model" come from a Dynamic Fault Tree?
	/// Then this is the probability of observing a failure in a system component
	/// before any repair takes place, e.g. of increasing one level of importance
	static double failProbDFT;

	/// Importance functions available
	static std::unordered_map<
		std::string,
		std::shared_ptr< ImportanceFunction > > impFuns;

	/// Thresholds builders available
	static std::unordered_map<
		std::string,
		std::shared_ptr< ThresholdsBuilder > > thrBuilders;

	/// Simulation engines available
	static std::unordered_map<
		std::string,
		std::shared_ptr< SimulationEngine > > simulators;

	/// Main system log
	static std::ostream& mainLog_;

	/// Technical system log
	static std::ostream& techLog_;

	/// Extra-verbose output printing in logs (aka Italian mode)
	static bool highVerbosity_;

	/// Starting time (according to omp_get_wtime) of last estimation launched
	static double lastEstimationStartTime_;

	/// Wall-clock-time execution limit for simulations (in seconds)
	static std::chrono::seconds timeout_;

	/// Confidence intervals produced during the last call to estimate()
	static std::vector< ConfidenceIntervalResult > lastEstimates_;

	/// Whether the single instance of ModelSuite is "empty as new"
	static bool pristineModel_;

	// Interruptions handling

	/// Signal handler for when we're interrupted (e.g. ^C) mid-estimation
	static SignalSetter SIGINThandler_;

	/// Signal handler for when we're terminated (e.g. kill) mid-estimation
	static SignalSetter SIGTERMhandler_;

	/// ConfidenceInterval to show currently reached estimation if interrupted
	static const ConfidenceInterval* interruptCI_;

	/// Practical confidence coefficients to show if interrupted
	static const std::vector< float > confCoToShow_;

	// Singleton design-pattern specifics

	/// Single existent instance of the class
	static std::unique_ptr< ModelSuite > instance_;

	/// Single instance thread safety
	static std::once_flag singleInstance_;

	/// Private ctors (singleton design pattern)
	ModelSuite() {}
	ModelSuite(ModelSuite&& that)                 = delete;
	ModelSuite& operator=(const ModelSuite& that) = delete;

public:  // Access to the ModelSuite instance

	/// Global access point to the unique instance of this class
	static inline ModelSuite& get_instance()
		{
			std::call_once(singleInstance_,
						   [] () { instance_.reset(new ModelSuite); });
			return *instance_;
		}

	/// Allow syntax "auto varname = fig::ModelSuite::get_instance();"
	inline ModelSuite(const ModelSuite&) {}
		// { instance_.swap(that.instance_); }

	~ModelSuite();

public:  // Populating facilities and other modifyers

	/// @copydoc ModuleNetwork::add_module(std::shared_ptr<ModuleInstance>&)
	/// @note Supresses the \ref pristineModel_ "pristine condition"
	/// @throw FigException if the model's clocks quota is exceeded
	void add_module(std::shared_ptr<ModuleInstance>&);

	/**
	 * @brief Add a new property to estimate during experimentation
	 * @details Properties are added orderly; they can be later accessed
	 *          by specifying the order in which they were added.
	 * @return The property index, as it should be passed to get_property()
	 *         to get a reference to it
	 * @note Supresses the \ref pristineModel_ "pristine condition"
	 * @warning Do not invoke after seal()
	 * \ifnot NDEBUG
	 *   @throw FigException if the network has already been sealed()
	 * \endif
	 * @see get_property()
	 */
	size_t add_property(std::shared_ptr<Property>);

	/**
	 * @brief Shut the system model to begin with simulations
	 *
	 *        Once everything was built and attached to the ModelSuite, global
	 *        information needs to be broadcasted among the internal objects
	 *        to allow cross-referencing (e.g. of variables) while simulating.
	 *        To that purpose seal() must be called by the user exactly once,
	 *        after all \ref Property "properties" and \ref ModuleInstance
	 *        "module instances" have been added to the ModelSuite,
	 *
	 * @param initialClocksNames Container with the names of the clocks which
	 *                           need to be reset on system initialization.
	 *                           If empty then all are considered initial.
	 *
	 * @note seal() must have been invoked before the beginning of simulations,
	 *       also to create the \ref SimulationEngine "engines" and
	 *       \ref ImportanceFunction "importance functions" required.
	 * @note Supresses the \ref pristineModel_ "pristine condition"
	 *
	 * @warning No more modules or properties can be added after this invocation
	 * \ifnot NDEBUG
	 *   @throw FigException if called more than once
	 * \endif
	 *
	 * @see ModuleNetwork::seal()
	 */
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	void seal(const Container<ValueType, OtherContainerArgs...>& initialClocksNames);

	/// Alias for seal() taking all system clocks as initial
	inline void seal() { seal(std::vector<std::string>()); }

	/**
	 * @brief Set the global effort for all Importance Splitting engines
	 *
	 *        If \p ge == 0, the default global effort of the SimulationEngine
	 *        named \p defaultFromEngine will be used.<br>
	 *        If \p ge == 0 and \p defaultFromEngine == "", the global effort
	 *        is unset.
	 *
	 * @param ge                @copydoc globalEffort
	 * @param defaultFromEngine Name of the SimulationEngine from which the
	 *                          default global effort value shall be copied
	 *
	 * @note If a non-global thresholds selection mechanism is chosen,
	 *       e.g. \ref ThresholdsBuilderES "Expected Success", this value
	 *       is ignored. Else <i>"global effort"</i> means:
	 *       <ul>
	 *       <li>In RESTART is 1 + #(replicas) made of a Traial
	 *           when it crosses a threshold-level upwards;</li>
	 *       <li>In Fixed Effort is the #(simulations) launched
	 *           on each threshold-level.</li>
	 *       </ul>
	 *
	 * @warning The ModelSuite must have been \ref seal() "sealed" beforehand
	 * @throw FigException if the model isn't \ref sealed() "sealed" yet
	 *
	 * @see globalEffort
	 */
	void set_global_effort(const unsigned& ge,
	                       const std::string& defaultFromEngine = "");
	
	/// @brief Inform the system model was translated from a Dynamic Fault Tree
	/// @param failProbDFT A <i>rough and unified</i> probability of observing
	///                    a failure in a system component before any repairs
	///                    occurs; if == 0.0 then an automatic choice is used.
	/// isDFT Whether the IOSA comes from a DFT translation
	/// @warning This is used to trigger mechanism that rely on a specific
	///          naming of the IOSA modules and their variables,
	///          viz. <b>this is implementation-specific</b> to the DFT-->IOSA-C
	///          translation applied, which was developed by Monti et al.
	void set_DFT(double failProbDFT = 0.0);

	/**
	 * @brief Set a wall-clock-time limit for simulations
	 *
	 *        This timeout applies to all simulations launched:
	 *        "value simulations" will stop when they reach either the
	 *        requested confidence criteria or this time bound, whichever
	 *        happens first; "time simulations" will stop for the lower of
	 *        the time bounds (their own limit or this global limit)
	 *
	 * @param timeLimit Wall-clock-time execution limit for simulations
	 *
	 * @warning The ModelSuite must have been \ref seal() "sealed" beforehand
	 * @throw FigException if the model isn't \ref sealed() "sealed" yet
	 */
	void set_timeout(const std::chrono::duration<size_t>& timeLimit);

	/// @brief Set a wall-clock-time limit for simulations (in seconds)
	/// @copydetails set_timeout(const std::chrono::duration&)
	/// @see set_timeout(const std::chrono::duration&)
	void set_timeout(const size_t& timeLimitSeconds);

	/**
	 * @brief Set RNG specs for time sampling
	 *
	 *        When a clock is reset, its new timeout value is sampled from a
	 *        distribution based on a Random Number Generator Engine (aka RNG).
	 *        This routine specifies which RNG algorithm to use,
	 *        and which value to seed it with on each independent simulation.
	 *
	 * @param rngType  Any of available_RNGs()
	 * @param rngSeed  Seed to use with the RNG;
	 *                 null value turns on randomized seeding
	 *
	 * @throw FigException if invalid rngType is specified
	 */
	void set_rng(const std::string& rngType, const size_t& rngSeed = Clock::rng_seed());

	/// Set (high) verbosity output printing in logs
	static void set_verbosity(bool verboseOutput) noexcept;

public:  // Accessors

	/// Is output printing in logs highly verbose?
	static inline bool get_verbosity() noexcept { return highVerbosity_; }

	/// @copydoc ModuleNetwork::sealed()
	inline bool sealed() const noexcept { return (nullptr != model) && model->sealed(); }

	/// @copydoc ModuleNetwork::is_markovian()
	inline bool is_markovian() const noexcept { return model->is_markovian(); }

	/// @copydoc ModuleNetwork::num_clocks()
	inline size_t num_clocks() const noexcept { return model->num_clocks(); }

	/// @copydoc ModuleNetwork::state_size()
	inline size_t state_size() const noexcept { return model->state_size(); }

	/// @copydoc ModuleNetwork::concrete_state_size()
	inline size_t concrete_state_size() const noexcept
		{ return model->concrete_state_size(); }

	/// @copydoc ModuleNetwork::num_modules()
	inline size_t num_modules() const noexcept { return model->num_modules(); }

	/// How many properties have been added to the ModelSuite
	inline size_t num_properties() const noexcept { return properties.size(); }

	/// @copydoc ModuleNetwork
	inline std::shared_ptr< const ModuleNetwork > modules_network() const noexcept
		{ return model; }

	/**
	 * Get the i-th property which was added to the ModelSuite
	 * @param i Index identifying the Property: the property \ref add_property()
	 *          "added first" has index 0, the next has index 1, and so on.
	 * @return Pointer to const version of the Property if i < num_properties(),
	 *         nullptr otherwise.
	 * @see add_property()
	 */
	std::shared_ptr< const Property > get_property(const size_t& i) const noexcept;

	/// Get the global effort used by all Importance Splitting engines
	/// @see set_global_effort()
	const unsigned& get_global_effort() const noexcept;

	/// For models translated from a Dynamic Fault Tree specification,
	/// e.g. GALILEO, this is the user specified probability of observing
	/// a failure in any 'OK' component before repair is observed
	/// in any 'DOWN' component
	/// @return The paremater specified by the user by set_DFT();
	///         -1.0 if set_DFT() was never called
	/// @see set_DFT()
	static double get_DFT() noexcept;

	/// Get the wall-clock-time execution limit imposed to simulations,
	/// in seconds
	/// @see set_timeout()
	const std::chrono::seconds& get_timeout() const noexcept;

	/// Get the wall-clock-time elapsed since the beginning of the current
	/// estimation, in seconds
	double get_running_time() const noexcept;

	/// @copydoc confCoToShow_
	static const std::vector< float >& get_cc_to_show() noexcept;

	/// Get the \ref ConfidenceInterval "confidence intervals" produced
	/// during the last call to estimate()
	/// @note For <i>observation</i>, not for modification
	static const std::vector< ConfidenceIntervalResult >& get_last_estimates() noexcept;

	/// @copydoc Module::initial_state()
	/// @throw FigException if there is no ModuleNetwork loaded
	static State<STATE_INTERNAL_TYPE> get_initial_state();

	/// Names of available simulation engines,
	/// as they should be requested by the user.
	static const std::vector< std::string >& available_simulators() noexcept;

	/// Names of available importance function,
	/// as they should be requested by the user.
	static const std::vector< std::string >& available_importance_functions() noexcept;

	/// Importance assessment strategies,
	/// as they should be requested by the user.
	static const std::vector< std::string >& available_importance_strategies() noexcept;

	/// Post-processings available for ImportanceFunctionConcrete,
	/// as they should be requested by the user.
	static const std::vector< std::string >& available_importance_post_processings() noexcept;

	/// Thresholds building techniques,
	/// as they should be requested by the user.
	static const std::vector< std::string >& available_threshold_techniques() noexcept;

	/// Pseudo Random Number Generator algorithms (aka RNGs),
	/// as they should be requested by the user.
	static const std::vector< std::string >& available_RNGs() noexcept;

	/// Size of the vector returned by available_simulators()
	/// as a constexpr
	static constexpr size_t num_simulators() noexcept
		{ return SimulationEngine::NUM_NAMES; }

	/// Size of the vector returned by available_importance_functions()
	/// as a constexpr
	static constexpr size_t num_importance_functions() noexcept
		{ return ImportanceFunction::NUM_NAMES; }

	/// Size of the vector returned by available_importance_strategies()
	/// as a constexpr
	static constexpr size_t num_importance_strategies() noexcept
		{ return ImportanceFunction::NUM_STRATEGIES; }

	/// Size of the vector returned by available_importance_post_processings()
	/// as a constexpr
	static constexpr size_t num_importance_post_processings() noexcept
		{ return ImportanceFunctionConcrete::NUM_POST_PROCESSINGS; }

	/// Size of the vector returned by available_threshold_techniques()
	/// as a constexpr
	static constexpr size_t num_threshold_techniques() noexcept
		{ return ThresholdsBuilder::NUM_TECHNIQUES; }

	/// Size of the vector returned by available_RNGs()
	/// as a constexpr
	static constexpr size_t num_RNGs() noexcept
		{ return Clock::NUM_RNGS; }

public:  // Utils

	/// Is \a engineName the name of an available simulation engine?
	/// @see available_simulators()
	static bool exists_simulator(const std::string& engineName) noexcept;

	/// Is \a ifunName the name of an available importance function?
	/// @see available_importance_functions()
	static bool exists_importance_function(const std::string& ifunName) noexcept;

	/// Is \a ifunStrategy an available importance assessment strategy?
	/// @see available_importance_strategies()
	static bool exists_importance_strategy(const std::string& impStrategy) noexcept;

	/// Is \a postProc an available pos-processing for ImportanceFunctionConcrete?
	/// @see available_importance_post_processings()
	static bool exists_importance_post_processing(const std::string& postProc) noexcept;

	/// Is \a thrTechnique an available thresholds building technique?
	/// @see available_threshold_techniques()
	static bool exists_threshold_technique(const std::string& thrTechnique) noexcept;

	/// Is \a rng an available thresholds building technique?
	/// @see available_RNGs()
	static bool exists_rng(const std::string& rng) noexcept;

	/// Print message in main log
	static void main_log(const std::string& msg);

	/// Print message in technical log
	static void tech_log(const std::string& msg);

	/// Print message in technical log if DEBUG mode is on; else NOP
#ifndef NDEBUG
	static void debug_log(const std::string& msg);
#else
	static inline void debug_log(const std::string&) {}
#endif

	/// Print message both in main and technical log
	static void log(const std::string& msg);

	/**
	 * @brief Assess importance for the currently loaded user model
	 *        using the "flat" strategy
	 *
	 *        This leaves the ImportanceFunction "ifunName" with internal
	 *        \ref ImportanceFunction::has_importance_info()
	 *        "importance information" but not quite
	 *        \ref ImportanceFunction::ready() "ready for simulations",
	 *        since the thresholds haven't been built yet.
	 *
	 * @param ifunName Any from available_importance_functions()
	 * @param property The Property whose value is to be estimated
	 * @param force    Assess importance again, even if importance info
	 *                 already exists for this importance function and strategy
	 *
	 * @note Supresses the \ref pristineModel_ "pristine condition"
	 *
	 * @throw FigException if 'ifunName' is invalid or incompatible with the
	 *                     "flat" importance assessment strategy.
	 * @throw FigException if the model isn't \ref sealed() "sealed" yet
	 *
	 * @see build_thresholds()
	 */
	void
	build_importance_function_flat(const std::string& ifunName,
								   const Property& property,
								   bool force = false);

	/// Same as build_importance_function_flat() for the property
	/// added to the system in the specified index
	/// @throw FigException if there's no property at index 'propertyIndex'
	/// @see get_property()
	void
	build_importance_function_flat(const std::string& ifunName,
								   const size_t& propertyIndex,
								   bool force = false);

	/**
	 * @brief Assess importance for the currently loaded user model
	 *        using the "adhoc" strategy
	 *
	 *        This leaves the ImportanceFunction "impFun.name" with internal
	 *        \ref ImportanceFunction::has_importance_info()
	 *        "importance information" but not quite
	 *        \ref ImportanceFunction::ready() "ready for simulations",
	 *        since the thresholds haven't been built yet.
	 *
	 * @param impFun   Specification of the ad hoc importance function name,
	 *                 mathematical formula, and any additional parameter
	 *                 such as the user-defined min and max values it can take
	 * @param property  The Property whose value is to be estimated
	 * @param force     Assess importance again, even if importance info
	 *                  already exists for this importance function and strategy
	 *
	 * @note Supresses the \ref pristineModel_ "pristine condition"
	 *
	 * @throw FigException if 'impFun.name' is invalid or incompatible with the
	 *                     "adhoc" importance assessment strategy.
	 * @throw FigException if badly formatted 'formulaExprStr' or 'varnames'
	 *                     has names not appearing in 'formulaExprStr'
	 * @throw FigException if the model isn't \ref sealed() "sealed" yet
	 *
	 * @see build_thresholds()
	 */
	void
	build_importance_function_adhoc(const ImpFunSpec& impFun,
									const Property& property,
									bool force = false);

	/// Same as build_importance_function_adhoc() for the property
	/// added to the system in the specified index
	/// @throw FigException if there's no property at index 'propertyIndex'
	/// @see get_property()
	void
	build_importance_function_adhoc(const ImpFunSpec& impFun,
									const size_t& propertyIndex,
									bool force = false);

	/**
	 * @brief Assess importance for the currently loaded user model
	 *        using the "auto" strategy
	 *
	 *        This leaves the ImportanceFunction "impFun.name" with internal
	 *        \ref ImportanceFunction::has_importance_info()
	 *        "importance information" but not quite
	 *        \ref ImportanceFunction::ready() "ready for simulations",
	 *        since the thresholds haven't been built yet.
	 *
	 * @param impFun   Specification of the importance function name, type
	 *                 (split vs. coupled) and any additional parameter
	 *                 such as the composition function or the neutral element
	 * @param property The Property whose value is to be estimated
	 * @param force    Assess importance again, even if importance info
	 *                 already exists for this importance function and strategy
	 *
	 * @note Supresses the \ref pristineModel_ "pristine condition"
	 *
	 * @throw FigException if 'impFun.name' is invalid or incompatible with the
	 *                     "auto" importance assessment strategy.
	 * @throw FigException if the model isn't \ref sealed() "sealed" yet
	 *
	 * @see ImportanceFunctionConcreteSplit::set_composition_fun()
	 * @see build_thresholds()
	 */
	void
	build_importance_function_auto(const ImpFunSpec& impFun,
								   const Property& property,
								   bool force = false);

	/// Same as build_importance_function_auto() for the property
	/// added to the system in the specified index
	/// @throw FigException if there's no property at index 'propertyIndex'
	/// @see get_property()
	void
	build_importance_function_auto(const ImpFunSpec& impFun,
								   const size_t& propertyIndex,
								   bool force = false);

private:  // Embedded into prepare_simulation_engine()

	/**
	 * @brief Build thresholds from precomputed importance information
	 *
	 *        The thresholds are built and kept inside the ImportanceFunction.
	 *        From this point on the finely grained importance values are
	 *        replaced with coarsely grained threshold levels.
	 *        After a successfull call the corresponding ImportanceFunction is
	 *        \ref ImportanceFunction::ready() "ready for simulations".
	 *
	 * @param technique Any from available_threshold_techniques()
	 * @param ifunName  Any from available_importance_functions(),
	 *                  refering to an ImportanceFunction which has
	 *                  \ref ImportanceFunction::has_importance_info()
	 *                  "importance information"
	 * @param property  User property query being estimated
	 * @param force     Build thresholds again, even if they already have been
	 *                  for this importance function and technique
	 *
	 * @return Whether the thresholds could be successfully built
	 *
	 * @note Supresses the \ref pristineModel_ "pristine condition"
	 *
	 * @throw FigException if "technique" or "ifunName" are invalid
	 * @throw FigException if the ImportanceFunction "ifunName" doesn't have
	 *                     \ref ImportanceFunction::has_importance_info()
	 *                     "importance information"
	 * @throw FigException if "technique" is incompatible with "ifunName"
	 *
	 * @see build_importance_function_flat()
	 * @see build_importance_function_auto()
	 * @see build_importance_function_adhoc()
	 */
	bool
	build_thresholds(const std::string& technique,
	                 const std::string& ifunName,
	                 std::shared_ptr<const Property> property,
	                 bool force = true);

	/// Same as build_thresholds() for the property
	/// added to the system in the specified index
	/// @throw FigException if there's no property at index 'propertyIndex'
	/// @see get_property()
	bool
	build_thresholds(const std::string& technique,
	                 const std::string& ifunName,
	                 const size_t& propertyIndex,
	                 bool force = true);

public:  // Utils

	/**
	 * @brief Make ready a SimulationEngine for upcoming estimations
	 *
	 *        Set the global effort (if any), choose thresholds with technique
	 *        \p thrTechnique for the ImportanceFunction \p ifunName,
	 *        and bind \p ifunName to the SimulationEngine \p engineName
	 *        if compatible.<br>
	 *        After a successfull call the returned engine can be used
	 *        with estimate().
	 *
	 * @param engineName    Any from available_simulators()
	 * @param ifunName      Any from available_importance_functions()
	 * @param thrTechnique  Any from available_threshold_techniques()
	 * @param property      User property query being estimated
	 * @param force         Forcefully perform all operations even when
	 *                      the engine is already ready
	 *
	 * @return Pointer to the SimulationEngine to be used for estimations
	 *
	 * @note Supresses the \ref pristineModel_ "pristine condition"
	 *
	 * @throw FigException if \p engineName, \p ifunName, or \p thrTechnique
	 *                     are invalid identifiers
	 * @throw FigException if \p ifunName   is incompatible with \p thrTechnique
	 * @throw FigException if \p engineName is incompatible with \p ifunName
	 * @throw FigException if thresholds building fails
	 *
	 * @see build_importance_function()
	 * @see build_thresholds()
	 */
	std::shared_ptr< SimulationEngine >
	prepare_simulation_engine(const std::string& engineName,
	                          const std::string& ifunName,
	                          const std::string& thrTechnique,
	                          std::shared_ptr< const Property > property,
	                          bool force = true);

	/// Same as prepare_simulation_engine() for the property
	/// added to the system in the specified index
	/// @throw FigException if there's no property at index 'propertyIndex'
	/// @see get_property()
	std::shared_ptr< SimulationEngine >
	prepare_simulation_engine(const std::string& engineName,
	                          const std::string& ifunName,
	                          const std::string& thrTechnique,
	                          const size_t& propertyIndex,
	                          bool force = true);

	/**
	 * @brief Release memory resources and decouple internals
	 *
	 *        After this call the ImportanceFunction 'ifunName' won't have
	 *        \ref ImportanceFunction::has_importance_info() "importance
	 *        information" any longer and the SimulationEngine 'engineName'
	 *        will be \ref SimulationEngine::bound() "unbound".
	 *
	 * @param ifunName   Name of the ImportanceFunction to clear
	 * @param engineName <i>(Optional)</i> Name of the SimulationEngine to unbind
	 */
	void
	release_resources(const std::string& ifunName,
	                  const std::string& engineName = "");

	/**
	 * Erase all internal information: the \ref ModuleNetwork "model",
	 * the \ref ImportanceFunction "importance functions",
	 * the \ref SimulationEngine "simulaion engines", etc.
	 *
	 * @note Restores the \ref pristineModel_ "pristine condition"
	 *
	 * @warning Not to be used when estimations finish!
	 *          clear() is intended for class re-use with a new model file;
	 *          <b>don't invoke this method if you're done using
	 *          FIG</b>---dynamic memory uses smart pointers to free itself.
	 */
	void clear() noexcept;

public:  // Simulation utils

	/**
	 * @brief Estimate the value of a property.
	 *
	 *        The estimation is performed using a single simulation strategy.
	 *        The importance function to use must have been previously bound
	 *        to the SimulationEngine.
	 *        Estimations are performed for all the \ref StoppingConditions
	 *        "simulation bounds" requested for experimentation, and logged
	 *        as they are produced.
	 *
	 * @param property User property query being estimated
	 * @param engine   SimulationEngine already tied to an ImportanceFunction
	 * @param bounds   List of stopping conditions to experiment with
	 *
	 * @throw FigException if engine wasn't \ref SimulationEngine::bound()
	 *                     "ready for simulations"
	 * @throw FigException if a simulation gave an invalid result
	 *
	 * @note Resets previous estimations as returned by get_last_estimates()
	 */
	void estimate(const Property& property,
				  SimulationEngine &engine,
				  const StoppingConditions& bounds,
				  const ImpFunSpec ifunSpec = ImpFunSpec("null","null")) const;

	/// Same as estimate() for the property added to the system in the requested index
	/// @throw FigException if there's no property at index 'propertyIndex'
	/// @see get_property()
	void estimate(const size_t& propertyIndex,
				  SimulationEngine& engine,
	              const StoppingConditions& bounds,
	              const ImpFunSpec ifunSpec = ImpFunSpec("null","null")) const;

	/**
	 * @brief Estimate the value of all \ref Property "stored properties"
	 *        using the specified mechanisms.
	 *
	 *        Consider one Property at a time and, for each estimation bound
	 *        requested (and also for each splitting if detailed), estimate
	 *        the property's value using the specified combination of
	 *        simulation engine, importance function, importance assessment
	 *        strategy and thresholds building technique.
	 *        All outcomes are kept track of in the system's logs.
	 *
	 * @param engineName Any from available_simulators()
	 * @param impFunSpec Specification of the importance function name,
	 *                   strategy, and any additional parameter such as an user
	 *                   defined algebraic function or the extreme values
	 * @param thrTechnique Any from available_threshold_techniques()
	 * @param estimationBounds List of stopping conditions to use for the
	 *                         estimation of each property (all are used)
	 * @param splittingValues List of splittings to test, used for
	 *                        RESTART-like simulation engines only
	 *
	 * @note The model must have been \ref seal() "sealed" beforehand
	 *
	 * @see process_interactive()
	 */
	template<
		template< typename, typename... > class Container1,
			typename ValueType1,
			typename... OtherArgs1,
		template< typename, typename... > class Container2 = std::vector,
			typename ValueType2,
			typename... OtherArgs2
	>
	void process_batch(const std::string& engineName,
					   const ImpFunSpec& impFunSpec,
					   const std::string& thrTechnique,
					   const Container1<ValueType1, OtherArgs1...>& estimationBounds,
	                   const Container2<ValueType2, OtherArgs2...>& globalEffortValues
						   = std::vector<unsigned>());

	/// @todo TODO design and implement user-interactive processing
	void process_interactive();

private:  // Class utils

	/// Specialization of estimate() for time-bound simulations
	void estimate_for_times(const Property& property,
							const SimulationEngine& engine,
							const StoppingConditions& bounds) const;

	/// Specialization of estimate() for confidence-criteria-bound simulations
	void estimate_for_confs(const Property& property,
							const SimulationEngine& engine,
							const StoppingConditions& bounds) const;

public: // Debug
        void print_info(std::ostream &out) const;
        void print_importance_function(std::ostream &out,
                                       const ImportanceFunction &imf) const;
};

// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template<
	template< typename, typename... > class Container1,
		typename ValueType1,
		typename... OtherArgs1,
	template< typename, typename... > class Container2,
		typename ValueType2,
		typename... OtherArgs2
>
void
ModelSuite::process_batch(const std::string& engineName,
    const ImpFunSpec& impFunSpec,
    const std::string& thrTechnique,
    const Container1<ValueType1, OtherArgs1...>& estimationBounds,
    const Container2<ValueType2, OtherArgs2...>& globalEffortValues)
{
	static_assert(std::is_constructible< StoppingConditions, ValueType1 >::value,
				  "ERROR: type mismatch. ModelSuite::process_batch() takes a "
				  "container with stopping conditions as fifth parameter");
	static_assert(std::is_constructible< unsigned, ValueType2 >::value,
				  "ERROR: type mismatch. ModelSuite::process_batch() takes a "
	              "container with global effort values as (optional) sixth parameter");

	// Validity check
	if (!sealed()) {
		log("Model hasn't been sealed yet.\n");
		throw_FigException("model hasn't been sealed yet");
	} else if (!exists_importance_function(impFunSpec.name)) {
		log("Importance function \"" + impFunSpec.name + "\" doesn't exist.\n");
		throw_FigException("inexistent importance function \"" + impFunSpec.name +
						   "\". Call \"available_importance_functions()\" "
						   "for a list of available options.");
	} else if (!exists_importance_strategy(impFunSpec.strategy)) {
		log("Importance assessment strategy \"" + impFunSpec.strategy +
			"\" doesn't exist.\n");
		throw_FigException("inexistent importance assessment strategy\"" +
						   impFunSpec.strategy + "\". Call \"available_importance_"
						   "strategies()\" for a list of available options.");
	} else if (!exists_importance_post_processing(impFunSpec.postProcessing.name)) {
		log("Invalid importance post processing (\""
			+ impFunSpec.postProcessing.name + "\")\n");
		throw_FigException("invalid importance post-processing \"" +
						   impFunSpec.postProcessing.name + "\"Call \"available_"
						   "importance_post_processings()\" for a list of "
						   "available options.");
	} else if (!exists_threshold_technique(thrTechnique)) {
		log("Thresholds building technique \"" + thrTechnique + "\" doesn't exist.");
		throw_FigException("inexistent threshold building technique \"" + thrTechnique +
						   "\". Call \"available_threshold_techniques()\" "
						   "for a list of available options.");
	} else if (!exists_simulator(engineName)) {
		log("Simulation engine \"" + engineName + "\" doesn't exist.");
		throw_FigException("inexistent simulation engine \"" + engineName +
						   "\". Call \"available_simulators()\" for a list "
						   "of available options.");
	} else if (distance(begin(estimationBounds), end(estimationBounds))
			   == 0ul) {
		log("Can't estimate: no stopping conditions were specified.\n");
		throw_FigException("aborting execution since no estimation bounds "
						   "were specified.");
	} else if (distance(begin(globalEffortValues), end(globalEffortValues))
			   == 0ul && "nosplit" != engineName) {
		log("Can't estimate: no global effort value was specified for engine \""
			+ engineName + "\"\n");
		throw_FigException("aborting execution since no global effort values "
						   "were chosen for simulation engine \"" +
						   engineName + "\"");
	}

	// For each property ...
	for (const auto& property: properties) {

		// ... build the importance function ...
		if ("flat" == impFunSpec.strategy)
			build_importance_function_flat(impFunSpec.name, *property, true);
		else if ("auto" == impFunSpec.strategy)
			build_importance_function_auto(impFunSpec, *property, true);
		else if ("adhoc" == impFunSpec.strategy)
			build_importance_function_adhoc(impFunSpec, *property, true);
		assert(impFuns[impFunSpec.name]->has_importance_info());

		// ... and for each global effort specified ...
		for (auto ge: globalEffortValues) {

			// ... choose thresholds, bind engine and ifun, etc ...
			set_global_effort(ge, engineName);
			auto engine = prepare_simulation_engine(engineName,
			                                        impFunSpec.name,
			                                        thrTechnique,
			                                        property);
			assert(impFuns[impFunSpec.name]->ready());
			assert(engine->ready());

			// ... and estimate the property's value for all stopping conditions
			for (const StoppingConditions& bounds: estimationBounds)
				estimate(*property, *engine, bounds, impFunSpec);

			if ("flat" == impFunSpec.strategy)
				break;  // no splits are used, avoid repetitions
		}
	}
}

} // namespace fig

#endif // MODELSUITE_H
