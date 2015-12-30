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
#include <type_traits>  // std::is_constructible<>
#include <unordered_map>
// FIG
#include <ModuleNetwork.h>
#include <Property.h>
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

/// @todo TODO define ConfidenceInterval class and erase this dummy
class ConfidenceInterval;
/// @todo TODO define StoppingCondition class and erase this dummy
class StoppingConditions;


class ModelSuite
{
	/// User's system model
	static ModuleNetwork model;
	
	/// Properties to estimate
	static std::vector<Property&> properties;
	
	/// Confidence criteria or time budgets bounding simulations
	static StoppingConditions simulationBounds;
	
	/// Importance functions available
	static std::unordered_map<
		std::string,
		std::shared_ptr< ImportanceFunction > > impFuns;
	
	/// Simulation engines available
	static std::unordered_map<
		std::string,
		SimulationEngine& > simulators;

	/// Single existent instance of the class (singleton design pattern)
	static std::unique_ptr< ModelSuite > instance_;

	/// Single instance thread safety
	static std::once_flag singleInstance_;

	/// Private ctors (singleton design pattern)
	ModelSuite() {}
	ModelSuite(ModelSuite&& that)                 = delete;
	ModelSuite& operator=(const ModelSuite& that) = delete;

public:  // Access to the ModelSuite instance

	/// Global access point to the unique instance of this pool
	static inline ModelSuite& get_instance()
		{
			std::call_once(singleInstance_,
						   [] () { instance_.reset(new ModelSuite); });
			return *instance_;
		}

	/// Allow syntax "auto varname = fig::ModelSuite::get_instance();"
	inline ModelSuite(const ModelSuite& that) {}

	~ModelSuite();

public:  // Stubs for ModuleNetwork

	/// @copydoc ModuleNetwork::add_module(std::shared_ptr<ModuleInstance>&)
	void add_module(std::shared_ptr< ModuleInstance >& module);

	/// @todo TODO copy all relevant public functions from ModuleNetwork

public:  // Simulation utils

	/**
	 * @brief Estimate the value of the \ref Property "stored properties"
	 *        with all combinations of importance and simulation strategies.
	 *
	 *        Consider one Property at a time and, for each simulation strategy,
	 *        importance function and stopping condition requested, estimate
	 *        its value and log the results.
	 *
	 * @param importanceStrategies Names of the importance functions  to test
	 * @param simulationStrategies Names of the simulation strategies to test
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
	void process_batch(const Container1<ValueType1, OtherArgs2...>& importanceStrategies,
					   const Container2<ValueType2, OtherArgs2...>& simulationStrategies);

	/// @todo TODO design and implement
	void process_interactive();

	/**
	 * @brief Estimate the value of a property using a specific combination of
	 *        importance function and simulation strategy
	 *
	 *        Estimations are performed for all the \ref StoppingConditions
	 *        "simulation bounds" requested for experimentation
	 *
	 * @param engine SimulationEngine already loaded with a Property and an ImportanceFunction
	 * @param bounds List of stopping conditions to experiment with
	 *
	 * @throw FigException if engine wasn't \ref SimulationEngine::loaded()
	 *                     "ready" for simulations
	 */
	void estimate(const SimulationEngine& engine, const StoppingConditions& bounds);
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
	const Container1<ValueType1, OtherArgs1...>& importanceStrategies,
	const Container2<ValueType2, OtherArgs2...>& simulationStrategies)
{
	static_assert(std::is_constructible< std::string, ValueType1 >::value,
				  "ERROR: type missmatch. ModelSuite::process_batch() takes "
				  "two containers with strings, the first describing the "
				  "importance strategies to use during simulations.");
	static_assert(std::is_constructible< std::string, ValueType2 >::value,
				  "ERROR: type missmatch. ModelSuite::process_batch() takes "
				  "two containers with strings, the second describing the "
				  "simulation strategies to use during simulations.");
	// For each property ...
	for (const Property& prop: properties) {
		// ... for each importance strategy (null, auto, ad hoc, etc) ...
		for (const std::string impStrat: importanceStrategies) {
			auto impFun = impFuns[impStrat]->assess_importance(model, prop);
			assert(impFun.ready());
			// ... and each simulation strategy (no split, restart, etc) ...
			for (const std::string simStrat: simulationStrategies) {
				SimulationEngine& engine = simulators[simStrat];
				try {
					engine.setup(prop, impFun);
				} catch (FigException& e) {
					continue;
					/// @todo TODO log the skipping of this combination
					///       Either the property or the importance function are
					///       incompatible with the current simulation engine
				}
				// ... estimate the property value for every stopping condition
				estimate(engine, simulationBounds);
				engine.cleanup();
			}
		}
	}
}

} // namespace fig

#endif // MODELSUITE_H
