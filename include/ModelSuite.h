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
#include <iterator>     // std::begin(), std::end()
#include <type_traits>  // std::is_constructible<>
#include <unordered_map>
// FIG
#include <ModuleNetwork.h>
#include <ImportanceFunction.h>
#include <SimulationEngine.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

// ADL
using std::begin;
using std::end;


namespace fig
{

class Property;
class ThresholdsBuilder;
class StoppingConditions;
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
	
	/// Confidence criteria or time budgets bounding simulations
	static StoppingConditions simulationBounds;

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

//	/// Log
//	static WTF? log_;

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

public:  // Global access to general constants

	/// Minimum amount of generated rare events to consider a simulation "good"
	static const unsigned MIN_COUNT_RARE_EVENTS;

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

public:  // Populating facilities

	/// @copydoc ModuleNetwork::add_module(std::shared_ptr<ModuleInstance>&)
	void add_module(std::shared_ptr<ModuleInstance>&);

	/**
	 * Add a new property to estimate during experimentation
	 * @see PropertyType
	 * @warning Do not invoke after seal()
	 * \ifnot NDEBUG
	 *   @throw FigException if the network has already been sealed()
	 * \endif
	 */
	void add_property(std::shared_ptr<Property>);

public:  // Modifyers

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
	 *                           need to be reset on system initialization
	 *
	 * @note seal() must have been invoked before the beginning of simulations,
	 *       also to create the \ref SimulationEngine "engines" and
	 *       \ref ImportanceFunction "importance functions" required.
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

public:  // Stubs for ModuleNetwork

	/// @copydoc ModuleNetwork::sealed()
	inline bool sealed() const noexcept { return model->sealed(); }

	/// @copydoc ModuleNetwork::num_clocks()
	inline size_t num_clocks() const noexcept { return model->num_clocks(); }

	/// @copydoc ModuleNetwork::state_size()
	inline size_t state_size() const noexcept { return model->state_size(); }

	/// @copydoc ModuleNetwork::concrete_state_size()
	inline size_t concrete_state_size() const noexcept
		{ return model->concrete_state_size(); }

	/// @copydoc ModuleNetwork
	inline std::shared_ptr< const ModuleNetwork > modules_network() const noexcept
		{ return model; }

public:  // Utils

	/// Names of available simulation engines,
	/// as they should be requested by the user.
	const std::vector< std::string >& available_simulators() const;

	/// Names of available importance function,
	/// as they should be requested by the user.
	const std::vector< std::string >& available_importance_functions() const;

	/// Importance assessment strategies,
	/// as they should be requested by the user.
	const std::vector< std::string >& available_importance_strategies() const;

	/// Thresholds building techniques,
	/// as they should be requested by the user.
	const std::vector< std::string >& available_threshold_techniques() const;

	/// Is 'engineName' the name of an available simulation engine?
	/// @see available_simulators()
	bool exists_simulator(const std::string& engineName) const noexcept;

	/// Is 'ifunName' the name of an available importance function?
	/// @see available_importance_functions()
	bool exists_importance_function(const std::string& ifunName) const noexcept;

	/// Is 'ifunStrategy' an available importance assessment strategy?
	/// @see available_importance_strategies()
	bool exists_importance_strategy(const std::string& impStrategy) const noexcept;

	/// Is 'thrTechnique' an available thresholds building technique?
	/// @see available_threshold_techniques()
	bool exists_threshold_technique(const std::string& thrTechnique) const noexcept;

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

	/**
	 * @brief Assess importance for the currently loaded user model
	 *        using the "auto" strategy
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
	 * @throw FigException if 'ifunName' is invalid or incompatible with the
	 *                     "auto" importance assessment strategy.
	 * @throw FigException if the model isn't \ref sealed() "sealed" yet
	 *
	 * @see build_thresholds()
	 */
	void
	build_importance_function_auto(const std::string& ifunName,
								   const Property& property,
								   bool force = false);

	/**
	 * @brief Assess importance for the currently loaded user model
	 *        using the "adhoc" strategy
	 *
	 *        This leaves the ImportanceFunction "ifunName" with internal
	 *        \ref ImportanceFunction::has_importance_info()
	 *        "importance information" but not quite
	 *        \ref ImportanceFunction::ready() "ready for simulations",
	 *        since the thresholds haven't been built yet.
	 *
	 * @param ifunName  Any from available_importance_functions()
	 * @param property  The Property whose value is to be estimated
	 * @param formulaExprStr  Mathematical formula to assess the states'
	 *                        importance, expressed as a string
	 * @param varnames  Names of variables ocurring in 'formulaExprStr',
	 *                  i.e. which substrings in the formula expression
	 *                  are actually variable names.
	 * @param force     Assess importance again, even if importance info
	 *                  already exists for this importance function and strategy
	 *
	 * @throw FigException if 'ifunName' is invalid or incompatible with the
	 *                     "adhoc" importance assessment strategy.
	 * @throw FigException if badly formatted 'formulaExprStr' or 'varnames'
	 *                     has names not appearing in 'formulaExprStr'
	 * @throw FigException if the model isn't \ref sealed() "sealed" yet
	 *
	 * @see build_thresholds()
	 */
	template< template< typename... > class Container, typename... OtherArgs >
	void
	build_importance_function_adhoc(const std::string& ifunName,
									const Property& property,
									const std::string& formulaExprStr,
									const Container<std::string, OtherArgs...>& varnames,
									bool force = false);

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
	 * @param force     Build thresholds again, even if they already have been
	 *                  for this importance function and technique
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
	void
	build_thresholds(const std::string& technique,
					 const std::string& ifunName,
					 bool force = true);

	/**
	 * @brief Set a SimulationEngine ready for upcoming estimations
	 *
	 *        Bind the ImportanceFunction 'ifunName' to the SimulationEngine
	 *        'engineName', if compatible. The ImportanceFunction must be
	 *        \ref ImportanceFunction::ready() "ready for simulations".
	 *        After a successfull call the returned engine can be used
	 *        with estimate().
	 *
	 * @param engineName Any from available_simulators()
	 * @param ifunName   Any from available_importance_functions(),
	 *                   refering to an ImportanceFunction which is
	 *                   \ref ImportanceFunction::ready() "ready for simulations"
	 *
	 * @return Pointer to the SimulationEngine to be used for estimations
	 *
	 * @throw FigException if "engineName" or "ifunName" are invalid
	 * @throw FigException if the ImportanceFunction "ifunName" isn't
	 *                     \ref ImportanceFunction::ready() "ready for
	 *                     simulations"
	 * @throw FigException if "engineName" is incompatible with "ifunName"
	 *
	 * @see build_importance_function()
	 * @see build_thresholds()
	 */
	std::shared_ptr< const SimulationEngine >
	prepare_simulation_engine(const std::string& engineName,
							  const std::string& ifunName);

	/**
	 * @brief Release memory resources and decouple internals
	 *
	 *        After this call the ImportanceFunction 'ifunName' won't have
	 *        \ref ImportanceFunction::has_importance_info() "importance
	 *        information" any longer and the SimulationEngine 'engineName'
	 *        will be \ref SimulationEngine::bound() "unbound".
	 *
	 * @param ifunName   Name of the ImportanceFunction to clear
	 * @param engineName Name of the SimulationEngine to unbind
	 */
	void
	release_resources(const std::string& ifunName,
					  const std::string& engineName = "") noexcept;

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
	 * @param engine SimulationEngine already tied to an ImportanceFunction
	 * @param bounds List of stopping conditions to experiment with
	 *
	 * @throw FigException if engine wasn't \ref SimulationEngine::bound()
	 *                     "ready for simulations"
	 * @throw FigException if a simulation gave an invalid result
	 */
	void estimate(const Property& property,
				  const SimulationEngine& engine,
				  const StoppingConditions& bounds) const;

	/**
	 * @brief Estimate the value of the \ref Property "stored properties"
	 *        with all combinations of importance and simulation strategies.
	 *
	 *        Consider one Property at a time and, for each simulation strategy,
	 *        importance function and stopping condition requested, estimate
	 *        the property's value and log the results.
	 *
	 * @param importanceSpecifications String pairs "(name,strategy)" of the
	 *                                 importance functions to test
	 * @param simulationStrategies Names of the simulation strategies to test
	 *
	 * @note The model must have been \ref seal() "sealed" beforehand
	 *
	 * @see process_interactive()
	 */
	template<
		template< typename, typename... > class Container1,
			typename ValueType1,
			typename... OtherArgs1,
		template< typename, typename... > class Container2,
			typename ValueType2,
			typename... OtherArgs2
	>
	void process_batch(const Container1<ValueType1, OtherArgs1...>& importanceSpecifications,
					   const Container2<ValueType2, OtherArgs2...>& simulationStrategies);

	/// @todo TODO design and implement interactive processing
	void process_interactive();
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
ModelSuite::process_batch(
	const Container1<ValueType1, OtherArgs1...>& importanceSpecifications,
	const Container2<ValueType2, OtherArgs2...>& simulationStrategies)
{
	typedef std::pair< std::string, std::string > pair_ss;
	static_assert(std::is_constructible< pair_ss, ValueType1 >::value,
				  "ERROR: type mismatch. ModelSuite::process_batch() takes "
				  "two containers, the first with strings pairs describing the "
				  "importance functions and strategies to use during simulations.");
	static_assert(std::is_constructible< std::string, ValueType2 >::value,
				  "ERROR: type mismatch. ModelSuite::process_batch() takes "
				  "two containers, the second with strings describing the "
				  "simulation strategies to use during simulations.");
	if (!sealed())
		throw_FigException("model hasn't been sealed yet");

	// For each property ...
	for (const auto prop: properties) {

		// ... each importance specification ...
		for (const pair_ss& impFunSpec: importanceSpecifications) {
			std::string impFunName, impFunStrategy;
			std::tie(impFunName, impFunStrategy) = impFunSpec;
			if (!exists_importance_function(impFunName)) {
				/// @todo TODO log the inexistence of this importance function
				continue;
			} else if (!exists_importance_strategy(impFunStrategy)) {
				/// @todo TODO log the inexistence of this importance assessment strategy
				continue;
			}
			if (impFunStrategy.empty() || "flat" == impFunStrategy)
				build_importance_function_flat(impFunName, *prop);
			else if ("auto" == impFunStrategy)
				build_importance_function_auto(impFunName, *prop);
			else
				throw_FigException("only automatically constructible importance "
								   "function strategies can be passed here, "
								   "i.e. \"flat\" or \"auto\".");
			build_thresholds("ams", impFunName);  // only implemented technique so far

			// ... and each simulation strategy ...
			for (const std::string simStrat: simulationStrategies) {
				if (!exists_simulator(simStrat)) {
					/// @todo TODO log the inexistence of this engine
					continue;
				}
				std::shared_ptr< const SimulationEngine > engine_ptr;
				try {
					engine_ptr = prepare_simulation_engine(simStrat, impFunName);
				} catch (FigException& e) {
					/// @todo TODO log the skipping of this combination
					///       This importance function is incompatible with
					///       the current simulation engine
					continue;
				}
				// ... estimate the property's value for all stopping conditions
				estimate(*prop, *engine_ptr, simulationBounds);
			}
			release_resources(impFunName);
		}
	}
}

} // namespace fig

#endif // MODELSUITE_H
