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


// C
#include <csignal>   // signal()
#include <unistd.h>  // alarm()
//#include <omp.h>     // omp_get_wtime()
// FIG
#include <ModelSuite.h>


namespace fig
{

// Static variables initialization

std::unique_ptr< ModuleNetwork > ModelSuite::model(new ModuleNetwork);

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
//			double startTime = omp_get_wtime();
//			do {
//				double estimation = engine.simulate(numRuns);
//				if (estimation >= 0.0)
//					ci.update(estimation);
//				else
//					increase_batch_size(numRuns, engine.name(), engine.current_ifun());
//			} while (!ci.satisfied_criterion());
//			log_(ci,
//				 omp_get_wtime() - startTime,
//				 engine.name,
//				 engine.current_ifun());
//		}
	}
}

} // namespace fig
