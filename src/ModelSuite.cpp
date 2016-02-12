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
#include <array>
#include <vector>
#include <forward_list>
#include <unordered_set>
#include <type_traits>  // std::is_convertible<>
#include <algorithm>    // std::find();
#include <iterator>     // std::begin(), std::end(), std::distance()
#include <string>
// C
#include <unistd.h>  // alarm(), exit()
#include <cmath>     // std::pow()
#include <omp.h>     // omp_get_wtime()
// FIG
#include <ModelSuite.h>
#include <FigException.h>
#include <SignalSetter.h>
#include <Property.h>
#include <StoppingConditions.h>
#include <SimulationEngine.h>
#include <SimulationEngineNosplit.h>
#include <SimulationEngineRestart.h>
#include <ImportanceFunction.h>
#include <ImportanceFunctionAlgebraic.h>
#include <ImportanceFunctionConcreteSplit.h>
#include <ImportanceFunctionConcreteCoupled.h>
#include <ThresholdsBuilder.h>
#include <ThresholdsBuilderAMS.h>
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

/**
 * @brief Build a ConfidenceInterval of the required type
 *
 *        Each PropertyType must be estimated using a special kind of
 *        ConfidenceInterval. The nature of the \ref fig::ImportanceFunction
 *        "importance function" also affects internal scalings.
 *        This helper function returns a new (i.e. without estimation data)
 *        interval of the correct kind for the property, and also with the
 *        proper internal adjustments and specified confidence criterion.
 *
 * @param propertyType     Type of the property whose value is being estimated
 * @param splitsPerThreshold @copydoc fig::SimulationEngine::splits_per_threshold()
 * @param impFun           fig::ImportanceFunction to use for estimations
 * @param confidenceCo     Interval's confidence coefficient ∈ (0.0, 1.0)
 * @param precision        Interval's desired full width > 0.0
 * @param dynamicPrecision Is the precision a percentage of the estimate?
 * @param hint             Suggestion of which kind of ConfidenceInterval to build
 *
 * @return Fresh ConfidenceInterval tailored for the given property
 *
 * @note If no confidence criteria is passed then a "time simulation"
 *       is assumed and the interval is built with the tightest constraints.
 *
 * @throw FigException if property type or hint isn't valid
 */
std::unique_ptr< fig::ConfidenceInterval >
build_empty_confidence_interval(
	const fig::PropertyType& propertyType,
	const unsigned& splitsPerThreshold,
	const fig::ImportanceFunction& impFun,
	const double& confidenceCo = 0.99999,
	const double& precision = 0.00001,
	const bool& dynamicPrecision = true,
    const std::string& hint = "")
{
	std::unique_ptr< fig::ConfidenceInterval > ci_ptr(nullptr);

	switch (propertyType) {
	case fig::PropertyType::TRANSIENT: {
		if (hint.empty()  // default to most precise
			|| "wilson" == hint)
			ci_ptr.reset(new fig::ConfidenceIntervalWilson(confidenceCo,
														   precision,
														   dynamicPrecision));
		else if ("proportion" == hint)
			ci_ptr.reset(new fig::ConfidenceIntervalProportion(confidenceCo,
															   precision,
															   dynamicPrecision));
        else
			throw_FigException(std::string("invalid CI hint \"").append(hint)
							   .append("\" for transient property"));
		// The statistical oversampling incurred here is bounded:
		//  · from below by splitsPerThreshold ^ minRareImportance,
		//  · from above by splitsPerThreshold ^ numThresholds.
		double minStatOversamp = std::pow(splitsPerThreshold,
										  impFun.min_rare_importance());
		double maxStatOversamp = std::pow(splitsPerThreshold,
										  impFun.num_thresholds());
		ci_ptr->set_statistical_oversampling(maxStatOversamp);
		ci_ptr->set_variance_correction(minStatOversamp/maxStatOversamp);
		} break;

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

	return ci_ptr;
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
	static const size_t batch_sizes[2][1] = {
		{ 1u<<11 /*, 1u<<9 */ },  // nosplit x {concrete_coupled, concrete_split}
		{ 1u<<9  /*, 1u<<8 */ }   // restart x {concrete_coupled, concrete_split}
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
	static const size_t batch_sizes[2][1] = {
		{ 3u /*, 3u */ },  // nosplit x {concrete_coupled, concrete_split}
		{ 2u /*, 2u */ }   // restart x {concrete_coupled, concrete_split}
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


/**
 * @brief Print/Log confidence intervals around an estimate
 *        for the given confidence criteria
 * @param ci ConfidenceInterval with the current estimate to show
 * @param confidenceCoefficients Confidence criteria to build the intervals
 * @note This should be implemented as a reentrant function,
 *       as it may be called from within signal handlers.
 */
void
interrupt_print(const fig::ConfidenceInterval& ci,
                const std::vector<float>& confidenceCoefficients)
{
    /// @todo TODO: implement proper reentrant logging and discard following shell print
    std::cerr << std::endl << "   · Computed estimate: "
              << ci.point_estimate() << std::endl;
    for (const float& confCo: confidenceCoefficients) {
        std::cerr << "   · " << confCo*100 << "% precision: "
                  << ci.precision(confCo) << std::endl;
        std::cerr << "     " << confCo*100 << "% confidence interval: [ "
                  << ci.lower_limit(confCo) << " , "
                  << ci.upper_limit(confCo) << " ] " << std::endl;
    }
    std::cerr << std::endl;
}

} // namespace



namespace fig
{

/// To catch interruptions (timeout, ^C, etc)
thread_local std::array< SignalHandlerType, MAX_SIGNUM_HANDLED > SignalHandlers;


// Static variables initialization

std::shared_ptr< ModuleNetwork > ModelSuite::model(std::make_shared<ModuleNetwork>());

std::vector< std::shared_ptr< Property > > ModelSuite::properties;

StoppingConditions ModelSuite::simulationBounds;

std::unordered_map< std::string, std::shared_ptr< ImportanceFunction > >
	ModelSuite::impFuns;

std::unordered_map< std::string, std::shared_ptr< ThresholdsBuilder > >
	ModelSuite::thrBuilders;

std::unordered_map< std::string, std::shared_ptr< SimulationEngine > >
	ModelSuite::simulators;

const ConfidenceInterval* ModelSuite::interruptCI_ = nullptr;

const std::vector< float > ModelSuite::confCoToShow_ = {0.8, 0.9, 0.95, 0.99};

SignalSetter ModelSuite::SIGINThandler_(SIGINT, [] (const int signal) {
        assert(SIGINT == signal);
        /// @todo TODO: implement proper reentrant logging and discard following shell print
        std::cerr << "\nCaught SIGINT, stopping estimations.\n";
        if (nullptr != ModelSuite::interruptCI_)
            interrupt_print(*ModelSuite::interruptCI_, ModelSuite::confCoToShow_);
        exit((1<<7)+SIGINT);
    }
);

SignalSetter ModelSuite::SIGTERMhandler_(SIGTERM, [] (const int signal) {
        assert(SIGTERM == signal);
        /// @todo TODO: implement proper reentrant logging and discard following shell print
        std::cerr << "\nCaught SIGTERM, stopping estimations.\n";
        if (nullptr != ModelSuite::interruptCI_)
            interrupt_print(*ModelSuite::interruptCI_, ModelSuite::confCoToShow_);
        exit((1<<7)+SIGTERM);
    }
);

std::unique_ptr< ModelSuite > ModelSuite::instance_ = nullptr;

std::once_flag ModelSuite::singleInstance_;

const unsigned ModelSuite::MIN_COUNT_RARE_EVENTS = (1u<<3);


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

	// Build offered importance functions
	impFuns["concrete_coupled"] = std::make_shared< ImportanceFunctionConcreteCoupled >();

	// Build offered thresholds builders
	thrBuilders["ams"] = std::make_shared< ThresholdsBuilderAMS >();

	// Build offered simulation engines
	simulators["nosplit"] = std::make_shared< SimulationEngineNosplit >(model);
	simulators["restart"] = std::make_shared< SimulationEngineRestart >(model);

#ifndef NDEBUG
	// Check all offered importance functions, thresholds builders and
	// simulation engines were actually instantiated
	for (const auto& ifunName: ImportanceFunction::names)
		if(end(impFuns) == impFuns.find(ifunName))
			throw_FigException(std::string("hey..., hey you ...  HEY, DEVELOPER!")
							   .append(" You forgot to create the '")
							   .append(ifunName).append("' importance function"));
	for (const auto& thrBuildName: ThresholdsBuilder::names)
		if(end(thrBuilders) == thrBuilders.find(thrBuildName))
			throw_FigException(std::string("hey..., hey you ...  HEY, DEVELOPER!")
							   .append(" You forgot to create the '")
							   .append(thrBuildName).append("' thresholds builder"));
	for (const auto& engineName: SimulationEngine::names)
		if (end(simulators) == simulators.find(engineName))
			throw_FigException(std::string("hey..., hey you ...  HEY, DEVELOPER!")
							   .append(" You forgot to create the '")
							   .append(engineName).append("' engine"));
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
ModelSuite::available_simulators() const
{
	static std::vector< std::string > simulatorsNames;
	if (simulatorsNames.empty() && !simulators.empty()) {
		simulatorsNames.reserve(simulators.size());
		for (const auto& pair: simulators)
			simulatorsNames.push_back(pair.first);
	} else if (simulators.empty()) {
		throw_FigException("ModelSuite hasn't been sealed, "
						   "no simulation engine is available yet.");
	}
	return simulatorsNames;
}


const std::vector< std::string >&
ModelSuite::available_importance_functions() const
{
	static std::vector< std::string > ifunsNames;
	if (ifunsNames.empty() && !impFuns.empty()) {
		ifunsNames.reserve(impFuns.size());
		for (const auto& pair: impFuns)
			ifunsNames.push_back(pair.first);
	} else if (impFuns.empty()) {
		throw_FigException("ModelSuite hasn't been sealed, "
						   "no importance function is available yet.");
	}
	return ifunsNames;
}


const std::vector< std::string >&
ModelSuite::available_importance_strategies() const
{
	static std::vector< std::string > importanceAssessmentStrategies;
	if (importanceAssessmentStrategies.empty()) {
		importanceAssessmentStrategies.reserve(ImportanceFunction::strategies.size());
		for (const auto& strategy: ImportanceFunction::strategies)
			importanceAssessmentStrategies.push_back(strategy);
	}
	return importanceAssessmentStrategies;
}


const std::vector< std::string >&
ModelSuite::available_threshold_techniques() const
{
	static std::vector< std::string > thresholdsBuildersTechniques;
	if (thresholdsBuildersTechniques.empty() && !thrBuilders.empty()) {
		thresholdsBuildersTechniques.reserve(thrBuilders.size());
		for (const auto& pair: thrBuilders)
			thresholdsBuildersTechniques.push_back(pair.first);
	} else if (thrBuilders.empty()) {
		throw_FigException("ModelSuite hasn't been sealed, "
						   "no thresholds builder is available yet.");
	}
	return thresholdsBuildersTechniques;
}


bool
ModelSuite::exists_simulator(const std::string& engineName) const noexcept
{
	try {
		const auto& simulators = available_simulators();
		if (find(begin(simulators), end(simulators), engineName) != end(simulators))
			return true;
	} catch (FigException) { /* Model isn't sealed, nothing exists yet */ }
	return false;
}


bool
ModelSuite::exists_importance_function(const std::string& ifunName) const noexcept
{
	try {
		const auto& impFuns = available_importance_functions();
		if (find(begin(impFuns), end(impFuns), ifunName) != end(impFuns))
			return true;
	} catch (FigException) { /* Model isn't sealed, nothing exists yet */ }
	return false;
}


bool
ModelSuite::exists_importance_strategy(const std::string& impStrategy) const noexcept
{
	try {
		const auto& impStrats = available_importance_strategies();
		if (find(begin(impStrats), end(impStrats), impStrategy) != end(impStrats))
			return true;
	} catch (FigException) { /* Model isn't sealed, nothing exists yet */ }
	return false;
}


bool
ModelSuite::exists_threshold_technique(const std::string& thrTechnique) const noexcept
{
	try {
		const auto& thrTechs = available_threshold_techniques();
		if (find(begin(thrTechs), end(thrTechs), thrTechnique) != end(thrTechs))
			return true;
	} catch (FigException) { /* Model isn't sealed, nothing exists yet */ }
	return false;
}


template<>  // For concrete importance functions
void        // which are built from the Porperty to be estimated.
ModelSuite::build_importance_function(const std::string& name,
									  const std::string& strategy,
									  const Property& property,
									  bool force)
{
	if (!exists_importance_function(name))
		throw_FigException(std::string("inexistent importance function \"")
						   .append(name).append("\". Call \"available_")
						   .append("importance_functions()\" for a list of ")
						   .append("available options."));
	if (!exists_importance_strategy(strategy))
		throw_FigException(std::string("inexistent importance assessment ")
						   .append("strategy \"").append(strategy).append("\". ")
						   .append("Call \"available_importance_strategies()\" ")
						   .append("for a list of available options."));

	auto cImpFun = std::dynamic_pointer_cast< ImportanceFunctionConcrete >(
					   impFuns[name]);
	if (nullptr == cImpFun)
		throw_FigException(std::string("build_importance_function() was called ")
						   .append("with a Property, which works only for ")
						   .append("concrete importance functions. \"")
						   .append(name).append("\" is no such function."));

	if (force
		|| strategy != cImpFun->strategy()
		|| !cImpFun->has_importance_info())
	{
		cImpFun->clear();
		cImpFun->assess_importance(*model, property, strategy);
	}

	assert(strategy == cImpFun->strategy());
	assert(cImpFun->has_importance_info());
}


template<>  // For algebraic importance functions
void        // which are built from some algebraic formula
ModelSuite::build_importance_function(const std::string& name,
									  const std::string& strategy,
									  std::pair< const std::string&,
												 const std::vector< std::string >&
									  > exprPair,
									  bool force)
{
	if (!exists_importance_function(name))
		throw_FigException(std::string("inexistent importance function \"")
						   .append(name).append("\". Call \"available_")
						   .append("importance_functions()\" for a list of ")
						   .append("available options."));
	if (!exists_importance_strategy(strategy))
		throw_FigException(std::string("inexistent importance assessment ")
						   .append("strategy \"").append(strategy).append("\". ")
						   .append("Call \"available_importance_strategies()\" ")
						   .append("for a list of available options."));

	auto aImpFun = std::dynamic_pointer_cast< ImportanceFunctionAlgebraic >(
					   impFuns[name]);
	if (nullptr == aImpFun)
		throw_FigException(std::string("build_importance_function() was called ")
						   .append("with a pair (expression_string, ")
						   .append("vector_of_varnames), which works only for ")
						   .append("algebraic importance functions. \"")
						   .append(name).append("\" is no such function."));

	if (force
		|| strategy != aImpFun->strategy()
		|| !aImpFun->has_importance_info())
	{
		aImpFun->clear();
		aImpFun->set_formula(exprPair.first, exprPair.second, strategy);
	}

	assert(strategy == aImpFun->strategy());
	assert(aImpFun->has_importance_info());
}


void
ModelSuite::build_thresholds(const std::string& technique,
							 const std::string& ifunName,
							 bool force)
{
	if (!exists_threshold_technique(technique))
		throw_FigException(std::string("inexistent threshold building ")
						   .append("technique \"").append(technique)
						   .append("\". Call \"available_threshold_techniques()")
						   .append("\" for a list of available options."));
	if (!exists_importance_function(ifunName))
		throw_FigException(std::string("inexistent importance function \"")
						   .append(ifunName).append("\". Call \"available_")
						   .append("importance_functions()\" for a list of ")
						   .append("available options."));

	ThresholdsBuilder& thrBuilder = *thrBuilders[technique];
	ImportanceFunction& ifun = *impFuns[ifunName];

	if (!ifun.has_importance_info())
		throw_FigException(std::string("importance function \"").append(ifunName)
						   .append("\" doesn't have importance information ")
						   .append("yet. Call \"build_importance_function()\" ")
						   .append("beforehand"));

	if (force || ifun.thresholds_technique() != technique)
		ifun.build_thresholds(thrBuilder, 2u/*splitsPerThreshold?*/);

	assert(technique == ifun.thresholds_technique());
	assert(ifun.ready());
}


std::shared_ptr< const SimulationEngine >
ModelSuite::prepare_simulation_engine(const std::string& engineName,
									  const std::string& ifunName)
{
	if (!exists_simulator(engineName))
		throw_FigException(std::string("inexistent simulation engine \"")
						   .append(engineName).append("\". Call \"available_")
						   .append("simulators()\" for a list of ")
						   .append("available options."));
	if (!exists_importance_function(ifunName))
		throw_FigException(std::string("inexistent importance function \"")
						   .append(ifunName).append("\". Call \"available_")
						   .append("importance_functions()\" for a list of ")
						   .append("available options."));

	auto engine_ptr = simulators[engineName];
	auto ifun_ptr = impFuns[ifunName];

	if (!ifun_ptr->has_importance_info())
		throw_FigException(std::string("importance function \"").append(ifunName)
						   .append("\" isn't yet ready for simulations. Call ")
						   .append("\"build_importance_function()\" and ")
						   .append("\"build_thresholds()\" beforehand"));

	if (engine_ptr->bound())
		engine_ptr->unbind();
	engine_ptr->bind(ifun_ptr);
	assert(engine_ptr->bound());
	assert(ifunName == engine_ptr->current_imp_fun());

	return engine_ptr;
}


void
ModelSuite::release_resources(const std::string& ifunName,
							  const std::string& engineName) noexcept
{
	if (exists_importance_function(ifunName)) {
		impFuns[ifunName]->clear();
		assert(!impFuns[ifunName]->has_importance_info());
	}
	if (exists_simulator(engineName)) {
		simulators[engineName]->unbind();
		assert(!simulators[engineName]->bound());
	}
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
//		log_.setfor_time_estimation();
		bool& timedout = engine.interrupted;
		auto ci_ptr = build_empty_confidence_interval(
						  property.type,
						  engine.splits_per_threshold(),
						  *impFuns[engine.current_imp_fun()]);
        interruptCI_ = ci_ptr.get();  // bad boy
        for (const unsigned long& wallTimeInSeconds: bounds.time_budgets()) {
			/// @todo TODO: implement proper log and discard following shell print
			std::cerr << "   Estimation time: " << wallTimeInSeconds << " s\n";
			SignalSetter handler(SIGALRM, [&ci_ptr, &timedout] (const int sig)
				{
                    assert(SIGALRM == sig);
                    interrupt_print(*ci_ptr, ModelSuite::confCoToShow_);
					ci_ptr->reset();
					timedout = true;
				}
			);
			timedout = false;
			alarm(wallTimeInSeconds);
			engine.simulate(property,
							min_batch_size(engine.name(), engine.current_imp_fun()),
							*ci_ptr,
							&increase_batch_size);
		}
        interruptCI_ = nullptr;

	} else {

		// Simulation bounds are confidence criteria
//		log.setfor_value_estimation();
        for (const auto& criterion: bounds.confidence_criteria()) {
			auto ci_ptr = build_empty_confidence_interval(
							  property.type,
							  engine.splits_per_threshold(),
							  *impFuns[engine.current_imp_fun()],
							  std::get<0>(criterion),
							  std::get<1>(criterion),
							  std::get<2>(criterion));
            interruptCI_ = ci_ptr.get();  // bad boy
			/// @todo TODO: implement proper log and discard following shell print
			std::cerr << "   Requested precision  "
					  << ((ci_ptr->percent ? 200  : 2) * ci_ptr->errorMargin)
					  << (ci_ptr->percent ? "%\n" : "\n");
			std::cerr << "   and confidence level " << 100*ci_ptr->confidence
					  << "%" << std::endl;
			size_t numRuns = min_batch_size(engine.name(), engine.current_imp_fun());
			double startTime = omp_get_wtime();

			do {
				bool increaseBatch = engine.simulate(property, numRuns, *ci_ptr);
				if (increaseBatch) {
					std::cerr << "-";
					increase_batch_size(numRuns, engine.name(), engine.current_imp_fun());
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
			std::cerr << "   · Precision: " << ci_ptr->precision() << std::endl;
			std::cerr << "   · Estimation time: " << (omp_get_wtime()-startTime)
					  << " seconds" << std::endl;
//			log_(*ci_ptr,
//				 omp_get_wtime() - startTime,
//				 engine.name(),
//				 engine.current_ifun());
        }
        interruptCI_ = nullptr;
    }
}

} // namespace fig
