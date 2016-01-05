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
// C
#include <csignal>   // signal()
#include <unistd.h>  // alarm()
#include <omp.h>     // omp_get_wtime()
// FIG
#include <ModelSuite.h>


namespace fig
{

// Static variables initialization

std::unique_ptr< ModuleNetwork > ModelSuite::model(std::make_shared<ModuleNetwork>());

std::vector< Reference< Property > > ModelSuite::properties;

StoppingConditions ModelSuite::simulationBounds;

std::unordered_map< std::string, std::shared_ptr< ImportanceFunction > >
	ModelSuite::impFuns;

std::unordered_map< std::string, Reference< SimulationEngine > >
	ModelSuite::simulators;

std::unique_ptr< ModelSuite > ModelSuite::instance_ = nullptr;

std::once_flag ModelSuite::singleInstance_;


// ModelSuite class member functions

ModelSuite::~ModelSuite() { /* not much to do around here... */ }


void ModelSuite::add_module(std::shared_ptr< ModuleInstance >& module)
{
	model->add_module(module);
}


void ModelSuite::add_property(Property& property)
{
	properties.push_back(property);
}


void ModelSuite::add_property(Property&& property)
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
		throw FigException("the ModelSuite has already been sealed");
#else
		return;
#endif

	// Notify the internal structures
	model->seal(initialClocksNames);
	for (Property& prop: properties)
		prop.pin_up_vars(model->global_state());

	// Build the simulation engines
	/// @todo TODO automatic mapping from names to simulation engines
	///       The idea is to have the correspondence name <--> engine
	///       in a single file (either here or better in core_typedefs.h)
	///       See http://stackoverflow.com/a/582456
}

// ModuleSuite::seal() can only be invoked with the following containers
template void ModelSuite::seal(const std::set<std::string>&);
template void ModelSuite::seal(const std::list<std::string>&);
template void ModelSuite::seal(const std::deque<std::string>&);
template void ModelSuite::seal(const std::vector<std::string>&);
template void ModelSuite::seal(const std::forward_list<std::string>&);
template void ModelSuite::seal(const std::unordered_set<std::string>&);


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
