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
#include <tuple>
#include <deque>
#include <vector>
#include <forward_list>
#include <unordered_set>
#include <type_traits>  // std::is_convertible<>
#include <algorithm>    // std::find();
#include <iterator>     // std::begin(), std::end(), std::distance()
#include <string>
// C
#include <csignal>   // signal()
#include <unistd.h>  // alarm()
#include <omp.h>     // omp_get_wtime()
// FIG
#include <ModelSuite.h>
#include <FigException.h>
#include <Property.h>
#include <StoppingConditions.h>
#include <SimulationEngine.h>
#include <SimulationEngineNosplit.h>
#include <ImportanceFunctionConcreteSplit.h>
#include <ImportanceFunctionConcreteCoupled.h>
#include <ConfidenceInterval.h>
#include <ConfidenceIntervalMean.h>
#include <ConfidenceIntervalProportion.h>
#include <ConfidenceIntervalWilson.h>

// ADL
using std::find;
using std::begin;
using std::end;


namespace
{

/// Global pointer used for signal handling (http://stackoverflow.com/q/10816107)
std::unique_ptr< fig::ConfidenceInterval > g_ci_ptr(nullptr);

/**
 * @brief Build a ConfidenceInterval of the required type
 *
 *        Each PropertyType must be estimated using a special kind of
 *        ConfidenceInterval. This helper function returns a new (i.e. without
 *        estimation data) interval of the correct kind for the property,
 *        and also with the specified confidence criterion.
 *        If such confidence criteria isn't specified then a "time simulation"
 *        is assumed and the interval is built with the tightest constraints.
 *
 * @param property         Property whose value is being estimated
 * @param confidenceCo     Interval's confidence coefficient ∈ (0.0, 1.0)
 * @param precision        Interval's desired full width > 0.0
 * @param dynamicPrecision Is the precision a percentage of the estimate?
 * @param hint             Suggestion of which kind of ConfidenceInterval to build
 *
 * @return Fresh ConfidenceInterval tailored for the given property
 *
 * @throw FigException unrecognized property type or hint
 */
std::unique_ptr< fig::ConfidenceInterval >
build_empty_confidence_interval(
    const fig::Property& property,
	const double& confidenceCo = 0.99999,
	const double& precision = 0.00001,
	const bool& dynamicPrecision = true,
    const std::string& hint = "")
{
    switch (property.type) {
    case fig::PropertyType::TRANSIENT:
        if (hint.empty())  // default to most precise
            return std::unique_ptr< fig::ConfidenceInterval >(
					new fig::ConfidenceIntervalWilson(confidenceCo,
													  precision,
													  dynamicPrecision));
        else if ("wilson" == hint)
            return std::unique_ptr< fig::ConfidenceInterval >(
					new fig::ConfidenceIntervalWilson(confidenceCo,
													  precision,
													  dynamicPrecision));
        else if ("proportion" == hint)
            return std::unique_ptr< fig::ConfidenceInterval >(
					new fig::ConfidenceIntervalProportion(confidenceCo,
														  precision,
														  dynamicPrecision));
        else
			throw_FigException(std::string("unrecognized hint \"").
							   append(hint).append("\""));
		break;

    case fig::PropertyType::THROUGHPUT:
    case fig::PropertyType::RATE:
    case fig::PropertyType::PROPORTION:
    case fig::PropertyType::BOUNDED_REACHABILITY:
        throw_FigException("property type isn't supported yet");
        break;

    default:
        throw_FigException("unrecognized property type");
        break;
    }
    // Following only to avoid warnings :(
	return std::unique_ptr< fig::ConfidenceInterval >(
			   new fig::ConfidenceIntervalMean(confidenceCo,
											   precision,
											   dynamicPrecision));
}


/**
 * @brief Tell minimum number of simulation runs requested to estimate
 *        the value of a property, for the specified fig::SimulationEngine
 *        and fig::ImportanceFunction pair
 *
 * @param engineName Valid engine name, i.e. one from fig::SimulationEngine::names
 * @param ifunName   Valid importance function name, i.e. one from
 *                   fig::ImportanceFunction::names
 *
 * @return Minimum batch size for this engine and importance function pair,
 *         i.e. number of consecutive simulations to run during estimations
 *
 * @see increase_batch_size()
 */
size_t
min_batch_size(const std::string& engineName, const std::string& ifunName)
{
	// Build internal table once: rows follow engine names definition order
	//                            cols follow impFun names definition order
	static constexpr auto& engineNames(fig::SimulationEngine::names);
	static constexpr auto& ifunNames(fig::ImportanceFunction::names);
//	FIXME: following compiles with Clang but not with gcc -- keep checking
//	static const size_t batch_sizes[engineNames.size()][ifunNames.size()] = {
	static const size_t batch_sizes[1][1] = {
		{ 1u<<10 /*, 1u<<8 */ }  /* ,  ==> nosplit x {concrete_coupled, concrete_split}
		{ 1u<<7, 1u<<5 }          *    ==> restart x {concrete_coupled, concrete_split}
								  */
	};
	const auto engineIt = find(begin(engineNames), end(engineNames), engineName);
	const auto ifunIt = find(begin(ifunNames), end(ifunNames), ifunName);
	// Check given engine and importance function names are valid
	if (engineIt == end(engineNames))
		throw_FigException(std::string("invalid engine name \"")
						   .append(engineName).append("\""));
	if (ifunIt == end(ifunNames))
		throw_FigException(std::string("invalid importance function name \"")
						   .append(ifunName).append("\""));
	// Return corresponding entry from table
	return batch_sizes[std::distance(begin(engineNames), engineIt)]
					  [std::distance(begin(ifunNames), ifunIt)];
}


/**
 * @brief Increment the number of consecutive simulations to run
 *        in order to get an estimate of the property's value
 *
 * @param numRuns    Current number of consecutive simulations, to be increased
 * @param engineName Valid engine name, i.e. one from fig::SimulationEngine::names
 * @param ifunName   Valid importance function name, i.e. one from
 *                   fig::ImportanceFunction::names
 *
 * @see min_batch_size()
 */
void
increase_batch_size(size_t& numRuns,
					const std::string& engineName,
					const std::string& ifunName)
{
	// Build internal table once: rows follow engine names definition order
	//                            cols follow impFun names definition order
	static constexpr auto& engineNames(fig::SimulationEngine::names);
	static constexpr auto& ifunNames(fig::ImportanceFunction::names);
//	FIXME: following compiles with Clang but not with gcc -- keep checking
//	static const size_t batch_sizes[engineNames.size()][ifunNames.size()] = {
	static const size_t batch_sizes[1][1] = {
		{ 4u /*, 3u */ }  /* ,  ==> nosplit x {concrete_coupled, concrete_split}
		{ 2u, 2u }         *    ==> restart x {concrete_coupled, concrete_split}
						   */
	};
	const auto engineIt = find(begin(engineNames), end(engineNames), engineName);
	const auto ifunIt = find(begin(ifunNames), end(ifunNames), ifunName);
	// Check given engine and importance function names are valid
	if (engineIt == end(engineNames))
		throw_FigException(std::string("invalid engine name \"")
						   .append(engineName).append("\""));
	if (ifunIt == end(ifunNames))
		throw_FigException(std::string("invalid importance function name \"")
						   .append(ifunName).append("\""));
	// Update numRuns with corresponding entry from table
	numRuns *= batch_sizes[std::distance(begin(engineNames), engineIt)]
						  [std::distance(begin(ifunNames), ifunIt)];
}

} // namespace


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
	impFuns["concrete_coupled"] = std::make_shared< ImportanceFunctionConcreteCoupled >();
//	impFuns["concrete_split"]   = std::make_shared< ImportanceFunctionConcreteSplit >();

#ifndef NDEBUG
	// Check all offered engines and functions were actually instantiated
	for (const auto& engineName: SimulationEngine::names)
		if (end(simulators) == simulators.find(engineName))
			throw_FigException(std::string("hey..., hey you ...  HEY, DEVELOPER!")
							   .append(" You forgot to create the '")
							   .append(engineName).append("' engine"));
	for (const auto& ifunName: ImportanceFunction::names)
		if(end(impFuns) == impFuns.find(ifunName))
			throw_FigException(std::string("hey..., hey you ...  HEY, DEVELOPER!")
							   .append(" You forgot to create the '")
							   .append(ifunName).append("' importance function"));
#endif
}

// ModelSuite::seal() can only be invoked with the following containers
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
ModelSuite::estimate(const Property& property,
                     const SimulationEngine& engine,
                     const StoppingConditions& bounds) const
{
	/// @todo TODO: implement proper log and discard following shell print
	std::cerr << "Estimating " << property.expression << ",\n";
	std::cerr << " using simulation engine \"" << engine.name() << "\"\n";
	std::cerr << " and importance function \"" << engine.current_imp_fun() << "\"\n";
	std::cerr << " built using strategy    \"" << engine.current_imp_strat() << "\"\n";

	if (bounds.is_time()) {

		// Simulation bounds are wall clock time limits
//		log_.time_estimation(engine);
		auto ci_ptr = build_empty_confidence_interval(property);
		g_ci_ptr.swap(ci_ptr);  // :'(
        for (const unsigned long& wallTimeInSeconds: bounds.time_budgets()) {
			/// @todo TODO: implement proper log and discard following shell print
			std::cerr << "   Estimation time: " << wallTimeInSeconds << " s\n";
			auto timeout = [/*&*/](int sig) {
				assert(SIGALRM == sig);
				/// @todo TODO: implement proper log and discard following shell print
				std::cerr << "   · Computed estimate: "
						  << g_ci_ptr->point_estimate() << std::endl;
				std::cerr << "   · 90% confidence interval: [ "
						  << g_ci_ptr->lower_limit(0.90) << " , "
						  << g_ci_ptr->upper_limit(0.90) << " ] " << std::endl;
				std::cerr << "   · 95% confidence interval: [ "
						  << g_ci_ptr->lower_limit(0.95) << " , "
						  << g_ci_ptr->upper_limit(0.95) << " ] " << std::endl;
				std::cerr << "   · 99% confidence interval: [ "
						  << g_ci_ptr->lower_limit(0.99) << " , "
						  << g_ci_ptr->upper_limit(0.99) << " ] " << std::endl;

				engine.interrupted = true;  /// FIXME: lambda can't capture!
				/// @todo TODO allow this lambda sighandler to capture variables
				///       http://stackoverflow.com/q/10816107

//				log_(*ci_ptr,
//				wallTimeInSeconds,
//				engine.name(),
//				engine.current_ifun());
//			    ci_ptr->reset();
			};
			engine.interrupted = false;
			signal(SIGALRM, timeout);
            alarm(wallTimeInSeconds);
			engine.simulate(property,
							min_batch_size(engine.name(), engine.current_imp_fun()),
							*g_ci_ptr);
			engine.interrupted = false;
		}

	} else {

		// Simulation bounds are confidence criteria
//		log.value_estimation(engine);
        for (const auto& criterion: bounds.confidence_criteria()) {
			auto ci_ptr = build_empty_confidence_interval(property,
														  std::get<0>(criterion),
														  std::get<1>(criterion),
														  std::get<2>(criterion));
			/// @todo TODO: implement proper log and discard following shell print
			std::cerr << "   Requested precision  "
					  << ((ci_ptr->percent ? 200  : 2) * ci_ptr->errorMargin)
					  << (ci_ptr->percent ? "%\n" : "\n");
			std::cerr << "   and confidence level " << 100*ci_ptr->confidence
					  << "%" << std::endl;
			size_t numRuns = min_batch_size(engine.name(), engine.current_imp_fun());
			double startTime = omp_get_wtime();
            do {
				double estimate = engine.simulate(property, numRuns);
				if (0.0 > estimate)
					throw_FigException("invalid simulation result");
				else
					ci_ptr->update(estimate);
				if (0.0 == estimate) {
					increase_batch_size(numRuns, engine.name(), engine.current_imp_fun());
					std::cerr << "-";
				} else {
					std::cerr << "+";
				}
			} while (!ci_ptr->is_valid());
			/// @todo TODO: implement proper log and discard following shell print
			std::cerr << std::endl;
			std::cerr << "   · Computed estimate: " << ci_ptr->point_estimate() << std::endl;
			std::cerr << "   · Confidence interval: [ "
					  << ci_ptr->lower_limit() << " , "
					  << ci_ptr->upper_limit() << " ] " << std::endl;
			std::cerr << "   · Estimation time: " << (omp_get_wtime()-startTime)
					  << " seconds" << std::endl;
//			log_(*ci_ptr,
//				 omp_get_wtime() - startTime,
//				 engine.name(),
//				 engine.current_ifun());
        }
	}
}

} // namespace fig
