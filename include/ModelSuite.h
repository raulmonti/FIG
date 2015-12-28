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

class ModelSuite
{
	/// User's system model
	static ModuleNetwork model;
	
	/// Properties to estimate
	static std::vector<Property&> properties;
	
//	/// Confidence criteria or time budgets bounding simulations
//	static StoppingCondition goal;
	
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

public:  // Current simulation configuration

//
//	Are these needed?
//
//	/**
//	 * @brief Register importance function to use in the following estimations
//	 * @details Grants access to importance info needed by some classes
//	 *          during simulations.
//	 * @param ifun Currently built importance function (null, auto, ad hoc...)
//	 * @see ModuleNetwork::inspect()
//	 */
//	void set_current_ifun(const ImportanceFunction& ifun);
//
//	/**
//	 * @brief Register simulation engine to use in the following estimations
//	 * @param engine Simulation engine (nosplit, restart...)
//	 * @note  This grants access to information for those classes
//	 *        which need it during simulations
//	 */
//	void set_current_engine(const SimulationEngine& engine);
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
	// For each property
	for (const Property& prop: properties) {
		// For each importance strategy (null, auto, ad hoc...)
		for (const std::string impStrat: importanceStrategies) {
			auto impFun = impFuns[impStrat]->assess_importance(model, prop);
			assert(impFun.ready());
			// For each simulation strategy (no split, restart...)
			for (const std::string simStrat: simulationStrategies) {
				SimulationEngine& engine = simulators[simStrat];
				engine.setup(prop, impFun);  // Throws if incompatible

			/// @todo TODO complete with pseudocode from ModelSuite_sketch.cpp
			///            located in the base dir of the git repo

			}
		}
	}
}

} // namespace fig

#endif // MODELSUITE_H
