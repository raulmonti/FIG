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
#include <StoppingConditions.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

// ADL
using std::begin;
using std::end;


namespace fig
{

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

	/// User's system model
	static std::unique_ptr< ModuleNetwork > model;
	
	/// Properties to estimate
	static std::vector< Reference< Property > > properties;
	
	/// Confidence criteria or time budgets bounding simulations
	static StoppingConditions simulationBounds;
	
	/// Importance functions available
	static std::unordered_map<
		std::string,
		std::shared_ptr< ImportanceFunction > > impFuns;
	
	/// Simulation engines available
	static std::unordered_map<
		std::string,
		Reference< SimulationEngine > > simulators;

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
	void add_module(std::shared_ptr< ModuleInstance >&);

	/// @todo TODO revise implementation and document
	void add_property(Property& property);

public:  // Modifyers

	/// @copydoc ModuleNetwork::seal()
	/// @todo TODO implement and document properly
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	void seal(const Container<ValueType, OtherContainerArgs...>& initialClocksNames)
	{
		model->seal(initialClocksNames);
		for (Property& prop: properties)
			prop.pin_up_vars(model->global_state());
	}

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

public:  // Delete the following when moving on from basic_TADs branch
	/// @todo TODO delete this function
	inline void simulation_step(Traial& traial, const SimulationEngine* engine) const
		{ model->simulation_step(traial, engine); }

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
	void process_batch(const Container1<ValueType1, OtherArgs1...>& importanceStrategies,
					   const Container2<ValueType2, OtherArgs2...>& simulationStrategies);

	/// @todo TODO design and implement
	void process_interactive();

	/**
	 * @brief Estimate the value of a property.
	 *
	 *        The estimation is performed using a single simulation strategy.
	 *        The importance function to use and the property to estimate
	 *        must have been loaded beforehand into the SimulationEngine.
	 *        Estimations are performed for all the \ref StoppingConditions
	 *        "simulation bounds" requested for experimentation, and logged
	 *        as they are produced.
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
				  "ERROR: type mismatch. ModelSuite::process_batch() takes "
				  "two containers with strings, the first describing the "
				  "importance strategies to use during simulations.");
	static_assert(std::is_constructible< std::string, ValueType2 >::value,
				  "ERROR: type mismatch. ModelSuite::process_batch() takes "
				  "two containers with strings, the second describing the "
				  "simulation strategies to use during simulations.");
	// For each property ...
	for (const Property& prop: properties) {
		// ... for each importance strategy (null, auto, ad hoc, etc) ...
		for (const std::string impStrat: importanceStrategies) {
			if (end(impFuns) == impFuns.find(impStrat)) {
				/// @todo TODO log the inexistence of this importance function
				continue;
			}
			auto impFun = impFuns[impStrat];
			impFun->assess_importance(model.get(), &prop);
			assert(impFun->ready());
			// ... and each simulation strategy (no split, restart, etc) ...
			for (const std::string simStrat: simulationStrategies) {
				if (end(simulators) == simulators.find(simStrat)) {
					/// @todo TODO log the inexistence of this engine
					continue;
				}
				SimulationEngine& engine = simulators[simStrat];
				try {
					engine.load(prop, impFun);
				} catch (FigException& e) {
					/// @todo TODO log the skipping of this combination
					///       Either the property or the importance function are
					///       incompatible with the current simulation engine
					continue;
				}
				// ... estimate the property value for all stopping conditions
				estimate(engine, simulationBounds);
				engine.unload();
			}
			impFun->clear();
		}
	}
}

} // namespace fig

#endif // MODELSUITE_H
