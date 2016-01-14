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
class StoppingConditions;

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
	
	/// Simulation engines available
	static std::unordered_map<
		std::string,
		std::shared_ptr< SimulationEngine > > simulators;

//	/// Log
//	static WTF? log_;

	/// Single existent instance of the class (singleton design pattern)
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

	/// @todo TODO erase as soon as we move on from simulations tests
	inline std::shared_ptr< const ModuleNetwork > modules_network() const { return model; }

public:  // Utils

	/// Names of available simulation engines,
	/// as they should be requested by the user.
	const std::vector< std::string >& available_simulators();

	/// Names of available importance function strategies,
	/// as they should be requested by the user.
	const std::vector< std::string >& available_importance_functions();

public:  // Simulation utils

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

	/// @todo TODO design and implement
	void process_interactive();

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
			if (end(impFuns) == impFuns.find(impFunName)) {
				/// @todo TODO log the inexistence of this importance function
				continue;
			}
			auto impFun = impFuns[impFunName];
			impFun->assess_importance(*model, *prop, impFunStrategy);
			assert(impFun->ready());

			// ... and each simulation strategy ...
			for (const std::string simStrat: simulationStrategies) {
				if (end(simulators) == simulators.find(simStrat)) {
					/// @todo TODO log the inexistence of this engine
					continue;
				}
				SimulationEngine& engine = *simulators[simStrat];
				try {
					engine.bind(impFun);
				} catch (FigException& e) {
					/// @todo TODO log the skipping of this combination
					///       Either the property or the importance function are
					///       incompatible with the current simulation engine
					continue;
				}
				// ... estimate the property's value for all stopping conditions
				estimate(*prop, engine, simulationBounds);
				engine.unbind();
			}
			impFun->clear();
		}
	}
}

} // namespace fig

#endif // MODELSUITE_H
