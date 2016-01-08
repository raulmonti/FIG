//==============================================================================
//
//  ModelSuite.cpp
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


// C++
#include <set>
#include <list>
#include <deque>
#include <vector>
#include <forward_list>
#include <unordered_set>
#include <type_traits>  // std::is_convertible<>
#include <iterator>     // std::begin(), std::end()
// C
#include <csignal>   // signal()
#include <unistd.h>  // alarm()
#include <omp.h>     // omp_get_wtime()
// FIG
#include <ModelSuite.h>
#include <SimulationEngine.h>
#include <SimulationEngineNosplit.h>
#include <ImportanceFunctionConcreteSplit.h>
#include <ImportanceFunctionConcreteCoupled.h>

// ADL
using std::begin;
using std::end;


namespace fig
{

// Static variables initialization

std::shared_ptr< ModuleNetwork > ModelSuite::model(std::make_shared<ModuleNetwork>());

std::vector< std::shared_ptr< Property > > ModelSuite::properties;

StoppingConditions ModelSuite::simulationBounds;

std::unordered_map< std::string, std::shared_ptr< ImportanceFunction > >
	ModelSuite::impFuns;

std::unordered_map< std::string, std::shared_ptr< SimulationEngine > >
	ModelSuite::simulators;

std::unique_ptr< ModelSuite > ModelSuite::instance_ = nullptr;

std::once_flag ModelSuite::singleInstance_;


// ModelSuite class member functions

ModelSuite::~ModelSuite() { /* not much to do around here... */ }


void ModelSuite::add_module(std::shared_ptr< ModuleInstance >& module)
{
	model->add_module(module);
}


void ModelSuite::add_property(std::shared_ptr<Property> property)
{
	properties.push_back(property);
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
void
ModelSuite::seal(const Container<ValueType, OtherContainerArgs...>& initialClocksNames)
{
	static_assert(std::is_convertible< std::string, ValueType >::value,
				  "ERROR: type mismatch. ModelSuite::seal() needs "
				  "a container with the initial clock names as strings");
	if (model->sealed())
#ifndef NDEBUG
		throw_FigException("the ModelSuite has already been sealed");
#else
		return;
#endif

	// Notify the internal structures
	model->seal(initialClocksNames);
	for (auto prop: properties)
		prop->pin_up_vars(model->global_state());

	// Build the simulation engines and the importance functions
	simulators["nosplit"] = std::make_shared< SimulationEngineNosplit >(model);
//	impFuns["concrete_split"]   = std::make_shared< ImportanceFunctionConcreteSplit >();
//	impFuns["concrete_coupled"] = std::make_shared< ImportanceFunctionConcreteCoupled >();

#ifndef NDEBUG
	// Check all offered engines and functions were actually instantiated
	for (const auto& engineName: SimulationEngine::names)
		if (end(simulators) == simulators.find(engineName))
			throw_FigException(std::string("hey..., hey you ...  HEY, DEVELOPER!")
							   .append(" You forgot to create the '")
							   .append(engineName).append("'' engine"));
	/// @todo TODO same lookup shit for importance functions
#endif
}

// ModuleSuite::seal() can only be invoked with the following containers
template void ModelSuite::seal(const std::set<std::string>&);
template void ModelSuite::seal(const std::list<std::string>&);
template void ModelSuite::seal(const std::deque<std::string>&);
template void ModelSuite::seal(const std::vector<std::string>&);
template void ModelSuite::seal(const std::forward_list<std::string>&);
template void ModelSuite::seal(const std::unordered_set<std::string>&);


const std::vector< std::string >&
ModelSuite::available_simulators()
{
	static std::vector< std::string > simulatorsNames;
	if (simulatorsNames.empty() && !simulators.empty()) {
		simulatorsNames.reserve(simulators.size());
		for (const auto& pair: simulators)
			simulatorsNames.push_back(pair.first);
	} else if (simulators.empty()) {
		std::cerr << "ModelSuite hasn't been sealed, "
				  << "no simulation engines are available yet."
				  << std::endl;
	}
	return simulatorsNames;
}


const std::vector< std::string >&
ModelSuite::available_importance_functions()
{
	static std::vector< std::string > ifunsNames;
	if (ifunsNames.empty() && !impFuns.empty()) {
		ifunsNames.reserve(impFuns.size());
		for (const auto& pair: impFuns)
			ifunsNames.push_back(pair.first);
	} else if (impFuns.empty()) {
		std::cerr << "ModelSuite hasn't been sealed, "
				  << "no importance functions are available yet."
				  << std::endl;
	}
	return ifunsNames;
}


void
ModelSuite::estimate(const SimulationEngine& engine,
					 const StoppingConditions& bounds)
{
	if (bounds.is_time()) {

		// Simulation bounds are wall clock time limits
//		ConfidenceInterval ci;
//		log_.set_for_times();
//		for (const unsigned long& wallTimeInSeconds: bounds.time_budgets()) {
//			auto timeout = [&]() { log_(ci,
//										wallTimeInSeconds,
//										engine.name(),
//										engine.current_ifun()); };
//			signal(SIGALRM, &timeout);
//			alarm(wallTimeInSeconds);
//			engine.simulate(ci);
//		}

	} else {

		// Simulation bounds are confidence criteria
//		log.set_for_values();
//		for (const auto& criterion: bounds.confidence_criteria()) {
//			ConfidenceInterval ci(criterion);
//			size_t numRuns = min_batch_size(engine.name(), engine.current_ifun());
			double startTime = omp_get_wtime();
//			do {
//				double estimation = engine.simulate(numRuns);
//				if (estimation >= 0.0)
//					ci.update(estimation);
//				else
//					increase_batch_size(numRuns, engine.name(), engine.current_ifun());
//			} while (!ci.satisfied_criterion());
//			log_(ci,
//				 omp_get_wtime() - startTime,
//				 engine.name(),
//				 engine.current_ifun());
//		}
	}
}

} // namespace fig
