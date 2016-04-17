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
#include <ios>          // std::scientific, std::fixed
#include <iomanip>      // std::setprecision()
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
#include <ThresholdsBuilderSMC.h>
#include <ThresholdsBuilderFixed.h>
#include <ThresholdsBuilderHybrid.h>
#include <ConfidenceInterval.h>
#include <ConfidenceIntervalMean.h>
#include <ConfidenceIntervalProportion.h>
#include <ConfidenceIntervalWilson.h>

using std::to_string;
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

	switch (propertyType)
	{
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
		//  · from below by splitsPerThreshold ^ minRareValue,
		//  · from above by splitsPerThreshold ^ numThresholds.
		double minStatOversamp = std::pow(splitsPerThreshold,
										  impFun.min_rare_value());
		double maxStatOversamp = std::pow(splitsPerThreshold,
										  impFun.num_thresholds());
		ci_ptr->set_statistical_oversampling(maxStatOversamp);
		ci_ptr->set_variance_correction(minStatOversamp/maxStatOversamp);
		} break;

    case fig::PropertyType::RATE:
		// Ignore hints, there's a single option
		ci_ptr.reset(new fig::ConfidenceIntervalMean(confidenceCo,
													 precision,
													 dynamicPrecision));
		break;

	case fig::PropertyType::THROUGHPUT:
	case fig::PropertyType::RATIO:
    case fig::PropertyType::BOUNDED_REACHABILITY:
        throw_FigException("property type isn't supported yet");
        break;

    default:
        throw_FigException("unrecognized property type");
        break;
    }

	return ci_ptr;
}


/// Choose minimum batch size (i.e. requested number of consecutive simulations
/// to run) in order to estimate the value of transient-like properties.
/// Fine tune for the specified SimulationEngine and ImportanceFunction pair
size_t
min_batch_size(const std::string& engineName, const std::string& ifunName)
{
	// Build internal table once: rows follow engine names definition order
	//                            cols follow impFun names definition order
	constexpr size_t NUM_ENGINES(fig::ModelSuite::num_simulators());
	constexpr size_t NUM_IMPFUNS(fig::ModelSuite::num_importance_functions());
	static const auto& engineNames(fig::SimulationEngine::names());
	static const auto& ifunNames(fig::ImportanceFunction::names());
	static const size_t batch_sizes[NUM_ENGINES][NUM_IMPFUNS] = {
		{ 1ul<<11, 1ul<<12, 1ul<<12 },  // nosplit x {concrete_coupled, concrete_split, algebraic}
		{ 1ul<<10, 1ul<<10, 1ul<<10 }   // restart x {concrete_coupled, concrete_split, algebraic}
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


/// Choose minimum simulation run length (in simulated time units)
/// in order to estimate the value of steady-state-like properties.
/// Fine tune for the specified SimulationEngine and ImportanceFunction pair
size_t
min_run_length(const std::string& engineName, const std::string& ifunName)
{
	// Build internal table once: rows follow engine names definition order
	//                            cols follow impFun names definition order
	constexpr size_t NUM_ENGINES(fig::ModelSuite::num_simulators());
	constexpr size_t NUM_IMPFUNS(fig::ModelSuite::num_importance_functions());
	static const auto& engineNames(fig::SimulationEngine::names());
	static const auto& ifunNames(fig::ImportanceFunction::names());
	static const size_t run_lengths[NUM_ENGINES][NUM_IMPFUNS] = {
		{ 1ul<<15, 1ul<<16, 1ul<<16 },  // nosplit x {concrete_coupled, concrete_split, algebraic}
		{ 1ul<<14, 1ul<<14, 1ul<<14 }   // restart x {concrete_coupled, concrete_split, algebraic}
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
	return run_lengths[std::distance(begin(engineNames), engineIt)]
					  [std::distance(begin(ifunNames), ifunIt)];
}


/**
 * @brief Select minimum simulation effort depending on the Property to estimate,
 *        tuning for specified SimulationEngine and ImportanceFunction
 * @param engineName Valid engine name, i.e. one from fig::SimulationEngine::names
 * @param ifunName   Valid importance function name, i.e. one from
 *                   fig::ImportanceFunction::names
 * @see min_batch_size()
 * @see min_run_length()
 */
size_t
min_effort(const fig::PropertyType& propertyType,
		   const std::string& engineName,
		   const std::string& ifunName)
{
	switch (propertyType)
	{
	case fig::PropertyType::TRANSIENT:
		return min_batch_size(engineName, ifunName);
		break;

	case fig::PropertyType::RATE:
		return min_run_length(engineName, ifunName);
		break;

	case fig::PropertyType::THROUGHPUT:
	case fig::PropertyType::RATIO:
	case fig::PropertyType::BOUNDED_REACHABILITY:
		throw_FigException("property type isn't supported yet");
		break;

	default:
		throw_FigException("unrecognized property type");
		break;
	}
	return 0ul;
}


/// Increase given batch size (i.e. requested number of consecutive simulations
/// ran) in order to estimate the value of transient-like properties
/// Fine tune for the specified SimulationEngine and ImportanceFunction pair
void
increase_batch_size(const std::string& engineName,
					const std::string& ifunName,
					size_t& batchSize)
{
	// Build internal table once: rows follow engine names definition order
	//                            cols follow impFun names definition order
	constexpr size_t NUM_ENGINES(fig::ModelSuite::num_simulators());
	constexpr size_t NUM_IMPFUNS(fig::ModelSuite::num_importance_functions());
	static const auto& engineNames(fig::SimulationEngine::names());
	static const auto& ifunNames(fig::ImportanceFunction::names());
	static const size_t inc_batch[NUM_ENGINES][NUM_IMPFUNS] = {
		{ 3ul, 3ul, 2ul },  // nosplit x {concrete_coupled, concrete_split, algebraic}
		{ 2ul, 2ul, 2ul }   // restart x {concrete_coupled, concrete_split, algebraic}
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
	// Update batchSize with corresponding entry from table
	batchSize *= inc_batch[std::distance(begin(engineNames), engineIt)]
						  [std::distance(begin(ifunNames), ifunIt)];
}



/// Increase given simulation run length (in simulated time units)
/// in order to estimate the value of steady-state-like properties.
/// Fine tune for the specified SimulationEngine and ImportanceFunction pair
void
increase_run_length(const std::string& engineName,
					const std::string& ifunName,
					size_t& runLength)
{
	// Build internal table once: rows follow engine names definition order
	//                            cols follow impFun names definition order
	constexpr size_t NUM_ENGINES(fig::ModelSuite::num_simulators());
	constexpr size_t NUM_IMPFUNS(fig::ModelSuite::num_importance_functions());
	static const auto& engineNames(fig::SimulationEngine::names());
	static const auto& ifunNames(fig::ImportanceFunction::names());
	static const float inc_length[NUM_ENGINES][NUM_IMPFUNS] = {
		{ 1.7f, 1.7f, 1.4f },  // nosplit x {concrete_coupled, concrete_split, algebraic}
		{ 1.4f, 1.4f, 1.4f }   // restart x {concrete_coupled, concrete_split, algebraic}
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
	// Update runLength with corresponding entry from table, rely on type promotion
	runLength *= inc_length[std::distance(begin(engineNames), engineIt)]
						   [std::distance(begin(ifunNames), ifunIt)];
}


/**
 * @brief Increase given simulation effort depending on the Property to estimate,
 *        tuning for specified SimulationEngine and ImportanceFunction
 * @param engineName Valid engine name, i.e. one from fig::SimulationEngine::names
 * @param ifunName   Valid importance function name, i.e. one from
 *                   fig::ImportanceFunction::names
 * @param effort     Simulation effort currently on use, to be increased
 * @see increase_batch_size()
 * @see increase_run_length()
 */
void
increase_effort(const fig::PropertyType& propertyType,
				const std::string& engineName,
				const std::string& ifunName,
				size_t& effort)
{
	switch (propertyType)
	{
	case fig::PropertyType::TRANSIENT:
		increase_batch_size(engineName, ifunName, effort);
		break;

	case fig::PropertyType::RATE:
		increase_run_length(engineName, ifunName, effort);
		break;

	case fig::PropertyType::THROUGHPUT:
	case fig::PropertyType::RATIO:
	case fig::PropertyType::BOUNDED_REACHABILITY:
		throw_FigException("property type isn't supported yet");
		break;

	default:
		throw_FigException("unrecognized property type");
		break;
	}
}


/**
 * @brief Print/Log confidence intervals around an estimate
 *        for all given confidence criteria
 * @details Intended for printing interrupted estimations, including
 *          those whose stopping condition was the running time.
 * @param ci ConfidenceInterval with the current estimate to show
 * @param confidenceCoefficients Confidence criteria to build the intervals
 * @param startTime (<i>optional</i>) Starting time, as returned by
 *                  omp_get_wtime(), of the last estimation launched
 * @note This should be implemented as a reentrant function,
 *       as it may be called from within signal handlers.
 */
void
interrupt_print(const fig::ConfidenceInterval& ci,
				const std::vector<float>& confidenceCoefficients,
				std::ostream& out,
				const double& startTime = -1.0)
{
    /// @todo TODO: implement proper reentrant logging and discard use of streams
    out << std::endl;
    out << std::setprecision(3) << std::scientific;
    out << "   · Computed estimate: " << ci.point_estimate() << std::endl;
    for (const float& confCo: confidenceCoefficients) {
        out << "   · " << std::setprecision(0) << std::fixed
            << confCo*100 << "% confidence" << std::endl
            << std::setprecision(2) << std::scientific
            << "       - precision: "  << ci.precision(confCo) << std::endl
            << "       - interval: [ " << ci.lower_limit(confCo) << ", "
                                       << ci.upper_limit(confCo) << "]"
            << std::endl;
    }
	if (startTime > 0.0) {
		out << std::setprecision(2) << std::fixed;
		out << "   · Estimation time: " << omp_get_wtime()-startTime << " s\n";
	}
//	out << std::defaultfloat;
    out << std::setprecision(6) << std::fixed;
    out << std::endl;
}


/**
 * @brief Print/Log an estimate and its theoretical confidence information
 * @details Intended for printing estimations which met a confidence goal,
 *          e.g. *not* those which were interrupted during computations.
 * @param ci ConfidenceInterval with the current estimate to show
 * @param time Time in seconds the whole estimation process took
 */
void
estimate_print(const fig::ConfidenceInterval& ci,
               const double& time,
               std::ostream& out)
{
    out << std::endl;
    out << std::setprecision(3) << std::scientific;
    out << "   · Computed estimate: " << ci.point_estimate() << std::endl;
    out << std::setprecision(2) << std::scientific;
    out << "   · Precision: " << ci.precision() << std::endl;
    out << "   · Confidence interval: [ " << ci.lower_limit() << ", "
                                          << ci.upper_limit() << " ] "
        << std::endl;
	out << std::setprecision(2) << std::fixed;
	out << "   · Estimation time: " << time << " s\n";
//	out << std::defaultfloat;
    out << std::setprecision(6) << std::fixed;
    out << std::endl;
}

} // namespace



namespace fig
{

/// To catch interruptions (timeout, ^C, etc)
thread_local std::array< SignalHandlerType, MAX_SIGNUM_HANDLED > SignalHandlers;


// Static variables initialization

std::shared_ptr< ModuleNetwork > ModelSuite::model(std::make_shared<ModuleNetwork>());

std::vector< std::shared_ptr< Property > > ModelSuite::properties;

unsigned ModelSuite::splitsPerThreshold = 2u;

std::unordered_map< std::string, std::shared_ptr< ImportanceFunction > >
	ModelSuite::impFuns;

std::unordered_map< std::string, std::shared_ptr< ThresholdsBuilder > >
	ModelSuite::thrBuilders;

std::unordered_map< std::string, std::shared_ptr< SimulationEngine > >
	ModelSuite::simulators;

std::ostream& ModelSuite::mainLog_(std::cout);

std::ostream& ModelSuite::techLog_(std::cerr);

double ModelSuite::lastEstimationStartTime_;

const ConfidenceInterval* ModelSuite::interruptCI_ = nullptr;

const std::vector< float > ModelSuite::confCoToShow_ = {0.8, 0.9, 0.95, 0.99};

SignalSetter ModelSuite::SIGINThandler_(SIGINT, [] (const int signal) {
#ifndef NDEBUG
		assert(SIGINT == signal);
#else
		if (SIGINT != signal) {
			ModelSuite::log("\nCalled SIGINThandler for signal "
							+ to_string(signal) + ", aborting.\n");
			std::exit((1<<7)+SIGABRT);
		}
#endif
		/// @todo TODO: implement proper reentrant logging
		ModelSuite::log("\nCaught SIGINT, stopping computations.\n");
		if (nullptr != ModelSuite::interruptCI_)
			interrupt_print(*ModelSuite::interruptCI_,
							ModelSuite::confCoToShow_,
							ModelSuite::mainLog_,
							ModelSuite::lastEstimationStartTime_);
		std::exit((1<<7)+SIGINT);
    }
);

SignalSetter ModelSuite::SIGTERMhandler_(SIGTERM, [] (const int signal) {
#ifndef NDEBUG
		assert(SIGTERM == signal);
#else
		if (SIGTERM != signal) {
			ModelSuite::log("\nCalled SIGTERMhandler for signal "
							+ to_string(signal) + ", aborting.\n");
			std::exit((1<<7)+SIGABRT);
		}
#endif
		assert(SIGTERM == signal);
		/// @todo TODO: implement proper reentrant logging
		ModelSuite::log("\nCaught SIGTERM, stopping estimations.\n");
        if (nullptr != ModelSuite::interruptCI_)
			interrupt_print(*ModelSuite::interruptCI_,
							ModelSuite::confCoToShow_,
							ModelSuite::mainLog_,
							ModelSuite::lastEstimationStartTime_);
		std::exit((1<<7)+SIGTERM);
    }
);

std::unique_ptr< ModelSuite > ModelSuite::instance_ = nullptr;

std::once_flag ModelSuite::singleInstance_;



// ModelSuite class member functions

// Class utils defined first

template< template< typename... Args > class Container, typename... Args>
void
ModelSuite::process_adhocfun_varnames(Container<Args...>& varnames)
{
	if (varnames.empty()) {
		auto allVarnames = model->global_state().varnames();
		varnames.insert(begin(varnames), begin(allVarnames), end(allVarnames));
	}
}
// ModelSuite::process_adhocfun_varnames generic version can only be invoked
// with the following containers
template void ModelSuite::process_adhocfun_varnames(std::list<std::string>&);
template void ModelSuite::process_adhocfun_varnames(std::deque<std::string>&);
template void ModelSuite::process_adhocfun_varnames(std::vector<std::string>&);
// ModelSuite::process_adhocfun_varnames specialization for std::set<>
template<> void
ModelSuite::process_adhocfun_varnames(std::set<std::string>& varnames)
{
	if (varnames.empty())
		for (const auto& name: model->global_state().varnames())
			varnames.emplace(name);
}
// ModelSuite::process_adhocfun_varnames specialization for std::forward_list<>
template<> void
ModelSuite::process_adhocfun_varnames(std::forward_list<std::string>& varnames)
{
	if (varnames.empty())
		for (const auto& name: model->global_state().varnames())
			varnames.emplace_front(name);
}
// ModelSuite::process_adhocfun_varnames specialization for std::unordered_set<>
template<> void
ModelSuite::process_adhocfun_varnames(std::unordered_set<std::string>& varnames)
{
	if (varnames.empty()) {
		auto allVarnames = model->global_state().varnames();
		varnames.insert(begin(allVarnames), end(allVarnames));
	}
}


ModelSuite::~ModelSuite() { /* not much to do around here... */ }


void ModelSuite::add_module(std::shared_ptr< ModuleInstance >& module)
{
	model->add_module(module);
}


void ModelSuite::add_property(std::shared_ptr<Property> property)
{
	property->index_ = properties.size();
	properties.push_back(property);
}


template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
void
ModelSuite::seal(const Container<ValueType, OtherContainerArgs...>& initialClocksNames)
{
	static_assert(std::is_constructible< std::string, ValueType >::value,
				  "ERROR: type mismatch. ModelSuite::seal() needs "
				  "a container with the initial clock names as strings");
	if (model->sealed())
#ifndef NDEBUG
		throw_FigException("the ModelSuite has already been sealed");
#else
		return;
#endif

	// Notify the internal structures
	if (std::distance(begin(initialClocksNames), end(initialClocksNames)) == 0ul) {
		// No initial clocks specified => all are treated as initial
		std::vector< std::string > allClocksNames(model->num_clocks());
		size_t i(0ul);
        for (const Clock& clock: model->clocks())
            allClocksNames[i++] = clock.name();
		model->seal(allClocksNames);
	} else {
		model->seal(initialClocksNames);
	}
	for (auto prop: properties)
		prop->pin_up_vars(model->global_state());

	// Build offered importance functions
	impFuns["concrete_coupled"] =
			std::make_shared< ImportanceFunctionConcreteCoupled >(*model);
	impFuns["concrete_split"] =
			std::make_shared< ImportanceFunctionConcreteSplit >(*model);
    impFuns["algebraic"] =
            std::make_shared< ImportanceFunctionAlgebraic >();

	// Build offered thresholds builders
	thrBuilders["fix"] = std::make_shared< ThresholdsBuilderFixed >();
	thrBuilders["ams"] = std::make_shared< ThresholdsBuilderAMS >();
	thrBuilders["smc"] = std::make_shared< ThresholdsBuilderSMC >();
	thrBuilders["hyb"] = std::make_shared< ThresholdsBuilderHybrid >();

	// Build offered simulation engines
	simulators["nosplit"] = std::make_shared< SimulationEngineNosplit >(model);
	simulators["restart"] = std::make_shared< SimulationEngineRestart >(model);
    set_splitting(splitsPerThreshold);

#ifndef NDEBUG
	// Check all offered importance functions, thresholds builders and
	// simulation engines were actually instantiated
	for (const auto& ifunName: available_importance_functions())
		if(end(impFuns) == impFuns.find(ifunName))
            throw_FigException("Hey.  Hey you...  HEY, DEVELOPER! You forgot to "
                               "create the '"+ifunName+"' importance function");
	for (const auto& thrTechnique: available_threshold_techniques())
		if(end(thrBuilders) == thrBuilders.find(thrTechnique))
            throw_FigException("Hey.  Hey you...  HEY, DEVELOPER! You forgot to "
							   "create the '"+thrTechnique+"' thresholds builder");
	for (const auto& engineName: available_simulators())
		if (end(simulators) == simulators.find(engineName))
            throw_FigException("Hey.  Hey you...  HEY, DEVELOPER! You forgot to "
                               "create the '"+engineName+"' simulation engine");
#endif
}

// ModelSuite::seal() can only be invoked with the following containers
template void ModelSuite::seal(const std::set<std::string>&);
template void ModelSuite::seal(const std::list<std::string>&);
template void ModelSuite::seal(const std::deque<std::string>&);
template void ModelSuite::seal(const std::vector<std::string>&);
template void ModelSuite::seal(const std::forward_list<std::string>&);
template void ModelSuite::seal(const std::unordered_set<std::string>&);


void
ModelSuite::set_splitting(const unsigned& spt)
{
    if (!sealed())
        throw_FigException("ModelSuite hasn't been sealed yet");
    dynamic_cast<SimulationEngineRestart&>(*simulators["restart"])
            .set_splits_per_threshold(spt);
	splitsPerThreshold = spt;
}


std::shared_ptr< const Property >
ModelSuite::get_property(const size_t& i) const noexcept
{
	if (i >= num_properties())
		return nullptr;
	else
		return properties[i];
}


const unsigned&
ModelSuite::get_splitting() const noexcept
{
    return splitsPerThreshold;
}


const std::vector< std::string >&
ModelSuite::available_simulators() noexcept
{
	static std::vector< std::string > simulatorsNames;
	if (simulatorsNames.empty()) {
		simulatorsNames.reserve(num_simulators());
		for (const auto& name: SimulationEngine::names())
			simulatorsNames.push_back(name);
	}
	return simulatorsNames;
}


const std::vector< std::string >&
ModelSuite::available_importance_functions() noexcept
{
	static std::vector< std::string > ifunsNames;
	if (ifunsNames.empty()) {
		ifunsNames.reserve(num_importance_functions());
		for (const auto& name: ImportanceFunction::names())
			ifunsNames.push_back(name);
	}
	return ifunsNames;
}


const std::vector< std::string >&
ModelSuite::available_importance_strategies() noexcept
{
	static std::vector< std::string > importanceAssessmentStrategies;
	if (importanceAssessmentStrategies.empty()) {
		importanceAssessmentStrategies.reserve(num_importance_strategies());
		for (const auto& strategy: ImportanceFunction::strategies())
			importanceAssessmentStrategies.push_back(strategy);
	}
	return importanceAssessmentStrategies;
}


const std::vector< std::string >&
ModelSuite::available_threshold_techniques() noexcept
{
	static std::vector< std::string > thresholdsBuildersTechniques;
	if (thresholdsBuildersTechniques.empty()) {
		thresholdsBuildersTechniques.reserve(num_threshold_techniques());
		for (const auto& technique: ThresholdsBuilder::techniques())
			thresholdsBuildersTechniques.push_back(technique);
	}
	return thresholdsBuildersTechniques;
}


bool
ModelSuite::exists_simulator(const std::string& engineName) noexcept
{
	const auto& simulators = available_simulators();
	return find(begin(simulators), end(simulators), engineName) != end(simulators);
}


bool
ModelSuite::exists_importance_function(const std::string& ifunName) noexcept
{
	const auto& impFuns = available_importance_functions();
	return find(begin(impFuns), end(impFuns), ifunName) != end(impFuns);
}


bool
ModelSuite::exists_importance_strategy(const std::string& impStrategy) noexcept
{
	const auto& impStrats = available_importance_strategies();
	return find(begin(impStrats), end(impStrats), impStrategy) != end(impStrats);
}


bool
ModelSuite::exists_threshold_technique(const std::string& thrTechnique) noexcept
{
	const auto& thrTechs = available_threshold_techniques();
	return find(begin(thrTechs), end(thrTechs), thrTechnique) != end(thrTechs);
}


void
ModelSuite::main_log(const std::string& msg)
{
	mainLog_ << msg;
}


void
ModelSuite::tech_log(const std::string& msg)
{
	techLog_ << msg;
}


void
ModelSuite::log(const std::string& msg)
{
	mainLog_ << msg;
	techLog_ << msg;
}


void
ModelSuite::build_importance_function_flat(const std::string& ifunName,
                                           const Property& property,
                                           bool force)
{
    if (!exists_importance_function(ifunName))
		throw_FigException("inexistent importance function \"" + ifunName +
						   "\". Call \"available_importance_functions()\" "
						   "for a list of available options.");

    ImportanceFunction& ifun = *impFuns[ifunName];
    // "flat" strategy is compatible with all ImportanceFunction derived types

    if (force || !ifun.has_importance_info() || "flat" != ifun.strategy()) {
		techLog_ << "\nBuilding importance function \"" << ifunName
				 << "\" with \"flat\" assessment strategy.\n";
		techLog_ << "Property: " << property.expression << std::endl;
        ifun.clear();
		const double startTime = omp_get_wtime();
		if (ifun.concrete())
			static_cast<ImportanceFunctionConcrete&>(ifun)
                .assess_importance(property, "flat");
		else
            static_cast<ImportanceFunctionAlgebraic&>(ifun)
                .set_formula("flat",
                             "0",
                             std::vector<std::string>(),
                             model->global_state(),
							 property);
		techLog_ << "Importance function building time: "
				 << std::fixed << std::setprecision(2)
				 << omp_get_wtime()-startTime << " s\n"
//				 << std::defaultfloat;
				 << std::setprecision(6);
	}

#ifndef NDEBUG
	assert(ifun.has_importance_info());
	assert("flat" == ifun.strategy());
	if (ifun.min_value() != ifun.max_value())
		throw_FigException("bad function built (non-flat importance)");
#endif
}


void
ModelSuite::build_importance_function_flat(const std::string& ifunName,
										   const size_t& propertyIndex,
										   bool force)
{
	auto propertyPtr = get_property(propertyIndex);
	if (nullptr == propertyPtr)
		throw_FigException("no property at index " + to_string(propertyIndex));
	build_importance_function_flat(ifunName, *propertyPtr, force);
}


void
ModelSuite::build_importance_function_auto(const std::string& ifunName,
										   const Property& property,
										   const std::string& mergeFun,
										   bool force)
{
	if (!exists_importance_function(ifunName))
		throw_FigException("inexistent importance function \"" + ifunName +
						   "\". Call \"available_importance_functions()\" "
						   "for a list of available options.");

    ImportanceFunction& ifun = *impFuns[ifunName];
    if (!ifun.concrete())
		throw_FigException("requested to build a non-concrete importance "
						   "function (\"" + ifunName + "\") using the "
						   "\"auto\" importance assessment strategy");

    if (force || !ifun.has_importance_info() || "auto" != ifun.strategy()) {
		techLog_ << "\nBuilding importance function \"" << ifunName
				 << "\" with \"auto\" assessment strategy.\n";
		techLog_ << "Property: " << property.expression << std::endl;
		ifun.clear();
		const double startTime = omp_get_wtime();
		if (ifunName == "concrete_split")
			static_cast<ImportanceFunctionConcreteSplit&>(ifun).set_merge_fun(mergeFun);
		try {
			static_cast<ImportanceFunctionConcrete&>(ifun)
					.assess_importance(property, "auto");
		} catch (FigException& e) {
			throw_FigException("couldn't build importance function \""
							   + ifunName + "\" automatically: " + e.msg());
		}
		techLog_ << "Initial state importance: " << ifun.initial_value() << std::endl;
		techLog_ << "Max importance: " << ifun.max_value() << std::endl;
		techLog_ << "Importance function building time: "
				 << std::fixed << std::setprecision(2)
				 << omp_get_wtime()-startTime << " s\n"
//				 << std::defaultfloat;
				 << std::setprecision(6);
	}

#ifndef NDEBUG
    assert(ifun.has_importance_info());
    assert("auto" == ifun.strategy());
	if (ifun.min_value() == ifun.max_value())
		throw_FigException("bad function built (flat importance)");
#endif
}


void
ModelSuite::build_importance_function_auto(const std::string& ifunName,
                                           const size_t& propertyIndex,
                                           const std::string& mergeFun,
                                           bool force)
{
    auto propertyPtr = get_property(propertyIndex);
    if (nullptr == propertyPtr)
		throw_FigException("no property at index " + to_string(propertyIndex));
    build_importance_function_auto(ifunName, *propertyPtr, mergeFun, force);
}


template< template< typename... > class Container, typename... OtherArgs >
void
ModelSuite::build_importance_function_adhoc(
    const std::string& ifunName,
    const Property& property,
    const std::string& formulaExprStr,
    Container<std::string, OtherArgs...> varnames,
    bool force)
{
    if (!exists_importance_function(ifunName))
		throw_FigException("inexistent importance function \"" + ifunName +
						   "\". Call \"available_importance_functions()\" "
						   "for a list of available options.");

    ImportanceFunction& ifun = *impFuns[ifunName];
    // "adhoc" strategy is compatible with all ImportanceFunction derived types

    if (force || !ifun.has_importance_info() || "adhoc" != ifun.strategy()) {
		techLog_ << "\nBuilding importance function \"" << ifunName
				 << "\" with \"adhoc\" assessment strategy (\""
				 << formulaExprStr << "\")\n";
		techLog_ << "Property: " << property.expression << std::endl;
		ifun.clear();
		process_adhocfun_varnames(varnames);  // make sure we have some variable names
		const double startTime = omp_get_wtime();
		if (ifun.concrete()) {
            std::vector<std::string> varnamesVec(begin(varnames), end(varnames));
            static_cast<ImportanceFunctionConcrete&>(ifun)
                .assess_importance(property, formulaExprStr, varnamesVec);
		} else {
            static_cast<ImportanceFunctionAlgebraic&>(ifun)
                .set_formula("adhoc",
                             formulaExprStr,
                             varnames,
                             model->global_state(),
                             property);
        }
		techLog_ << "Initial state importance: " << ifun.initial_value() << std::endl;
		techLog_ << "Max importance: " << ifun.max_value() << std::endl;
		techLog_ << "Importance function building time: "
				 << std::fixed << std::setprecision(2)
				 << omp_get_wtime()-startTime << " s\n"
//				 << std::defaultfloat;
				 << std::setprecision(6);
    }

#ifndef NDEBUG
	assert(ifun.has_importance_info());
	assert("adhoc" == ifun.strategy());
	if (ifun.min_value() == ifun.max_value())
		throw_FigException("bad function built (flat importance)");
#endif
}

// ModelSuite::build_importance_function_adhoc() can only be invoked
// with the following containers
template void ModelSuite::build_importance_function_adhoc(
    const std::string&, const Property&, const std::string&,
    std::set<std::string>, bool);
template void ModelSuite::build_importance_function_adhoc(
    const std::string&, const Property&, const std::string&,
    std::list<std::string>, bool);
template void ModelSuite::build_importance_function_adhoc(
    const std::string&, const Property&, const std::string&,
    std::deque<std::string>, bool);
template void ModelSuite::build_importance_function_adhoc(
    const std::string&, const Property&, const std::string&,
    std::vector<std::string>, bool);
template void ModelSuite::build_importance_function_adhoc(
    const std::string&, const Property&, const std::string&,
    std::forward_list<std::string>, bool);
template void ModelSuite::build_importance_function_adhoc(
    const std::string&, const Property&, const std::string&,
    std::unordered_set<std::string>, bool);


template< template< typename... > class Container, typename... OtherArgs >
void
ModelSuite::build_importance_function_adhoc(
	const std::string& ifunName,
	const size_t& propertyIndex,
	const std::string& formulaExprStr,
	Container<std::string, OtherArgs...> varnames,
	bool force)
{
	auto propertyPtr = get_property(propertyIndex);
	if (nullptr == propertyPtr)
		throw_FigException("no property at index " + to_string(propertyIndex));
	build_importance_function_adhoc(ifunName,
									*propertyPtr,
									formulaExprStr,
									varnames,
									force);
}

// ModelSuite::build_importance_function_adhoc() can only be invoked
// with the following containers
template void ModelSuite::build_importance_function_adhoc(
	const std::string&, const size_t&, const std::string&,
	std::set<std::string>, bool);
template void ModelSuite::build_importance_function_adhoc(
	const std::string&, const size_t&, const std::string&,
	std::list<std::string>, bool);
template void ModelSuite::build_importance_function_adhoc(
	const std::string&, const size_t&, const std::string&,
	std::deque<std::string>, bool);
template void ModelSuite::build_importance_function_adhoc(
	const std::string&, const size_t&, const std::string&,
	std::vector<std::string>, bool);
template void ModelSuite::build_importance_function_adhoc(
	const std::string&, const size_t&, const std::string&,
	std::forward_list<std::string>, bool);
template void ModelSuite::build_importance_function_adhoc(
	const std::string&, const size_t&, const std::string&,
	std::unordered_set<std::string>, bool);


void
ModelSuite::build_thresholds(const std::string& technique,
							 const std::string& ifunName,
							 bool force,
							 const float& lvlUpProb,
							 const unsigned& simsPerIter)
{
	if (!exists_threshold_technique(technique))
		throw_FigException("inexistent threshold building technique \"" + technique
						   + "\". Call \"available_threshold_techniques()"
							 "\" for a list of available options.");
	if (!exists_importance_function(ifunName))
		throw_FigException("inexistent importance function \"" + ifunName +
						   "\". Call \"available_importance_functions()\" "
						   "for a list of available options.");

	ThresholdsBuilder& thrBuilder = *thrBuilders[technique];
	ImportanceFunction& ifun = *impFuns[ifunName];

	if (!ifun.has_importance_info())
		throw_FigException("importance function \"" + ifunName + "\" doesn't "
						   "have importance information yet. Call any of the "
						   "\"build_importance_function_xxx()\" routines with "
						   "\"" + ifunName + "\" beforehand");

	if (force || ifun.thresholds_technique() != technique) {
		techLog_ << "\nBuilding thresholds for importance function \""
				 << ifunName << "\",\nwith splitting = "
				 << std::to_string(splitsPerThreshold)
				 << " and using technique \"" << technique << "\"\n";
		const double startTime = omp_get_wtime();
		if (thrBuilder.adaptive() && lvlUpProb > 0.0)
			ifun.build_thresholds_adaptively(
					*std::dynamic_pointer_cast<ThresholdsBuilderAdaptive>(thrBuilders[technique]),
					splitsPerThreshold,
					lvlUpProb,
					simsPerIter);
		else
			ifun.build_thresholds(thrBuilder, splitsPerThreshold);
		techLog_ << "Thresholds building time: "
				 << std::fixed << std::setprecision(2)
				 << omp_get_wtime()-startTime << " s\n"
//				 << std::defaultfloat;
				 << std::setprecision(6);
	}

    assert(ifun.ready());
    assert(technique == ifun.thresholds_technique());
}


std::shared_ptr< SimulationEngine >
ModelSuite::prepare_simulation_engine(const std::string& engineName,
									  const std::string& ifunName)
{
	if (!exists_simulator(engineName))
        throw_FigException("inexistent simulation engine \"" + engineName +
                           "\". Call \"available_simulators()\" for a list "
                           "of available options.");
	if (!exists_importance_function(ifunName))
        throw_FigException("inexistent importance function \"" + ifunName +
                           "\". Call \"available_importance_functions()\" "
                           "for a list of available options.");

	auto engine_ptr = simulators[engineName];
	auto ifun_ptr = impFuns[ifunName];

	if (!ifun_ptr->has_importance_info())
        throw_FigException("importance function \"" + ifunName + "\" isn't yet "
                           "ready for simulations. Call \"build_importance_"
                           "function()\" and \"build_thresholds()\" beforehand");

	if (engine_ptr->bound())
		engine_ptr->unbind();
    techLog_ << "\nBinding simulation engine \"" << engineName << ""
             << "\" to importance function \"" << ifunName << "\"\n";
    engine_ptr->bind(ifun_ptr);
	assert(engine_ptr->bound());
	assert(ifunName == engine_ptr->current_imp_fun());

	return engine_ptr;
}


void
ModelSuite::release_resources(const std::string& ifunName,
							  const std::string& engineName) noexcept
{
    techLog_ << "\nReleasing resources";
    if (exists_importance_function(ifunName)) {
        techLog_ << " of importance function \"" << ifunName << "\"";
		impFuns[ifunName]->clear();
		assert(!impFuns[ifunName]->has_importance_info());
	}
    if (exists_simulator(engineName)) {
        if (exists_importance_function(ifunName))
            techLog_ << " and";
        else
            techLog_ << " of";
        techLog_ << " simulation engine \"" << engineName << "\"";
		simulators[engineName]->unbind();
		assert(!simulators[engineName]->bound());
    }
    techLog_ << std::endl;
}


void
ModelSuite::release_resources() noexcept
{
	if (!sealed())
        return;
    techLog_ << "\nReleasing all system resources\n";
	try {
		for (auto ifunName: available_importance_functions())
			release_resources(ifunName);
		for (auto engineName: available_simulators())
			simulators[engineName]->unbind();
	} catch (FigException&) {}
}


void
ModelSuite::estimate(const Property& property,
                     const SimulationEngine& engine,
                     const StoppingConditions& bounds) const
{
	if (!engine.ready())
		throw_FigException("SimulationEngine \"" + engine.name()
						  +"\" isn't ready for simulations");
	const ImportanceFunction& ifun(*impFuns[engine.current_imp_fun()]);
	const std::string adHocFun(ifun.adhoc_fun());

	mainLog_ << "Estimating " << property.expression << ",\n";
	mainLog_ << " using simulation engine  \"" << engine.name() << "\"\n";
	mainLog_ << " with importance function \"" << engine.current_imp_fun() << "\"\n";
	mainLog_ << " built using strategy     \"" << engine.current_imp_strat() << "\"";
	mainLog_ << (adHocFun.empty() ? ("") : (" ("+adHocFun+")")) << std::endl;
	mainLog_ << " and thresholds technique \"" << ifun.thresholds_technique() << "\"\n";
	mainLog_ << " [ " << ifun.num_thresholds() << " thresholds";
	mainLog_ << " | splitting " << engine.splits_per_threshold() << " ]\n";

	if (bounds.is_time()) {

		// Simulation bounds are wall clock time limits
		bool& timedout = engine.interrupted;
		for (const unsigned long& wallTimeInSeconds: bounds.time_budgets()) {
			auto ci_ptr = build_empty_confidence_interval(
							  property.type,
							  engine.splits_per_threshold(),
							  *impFuns[engine.current_imp_fun()]);
			interruptCI_ = ci_ptr.get();  // bad boy
			mainLog_ << std::setprecision(0) << std::fixed;
			mainLog_ << "   Estimation time: " << wallTimeInSeconds << " s\n";
			SignalSetter handler(SIGALRM, [&ci_ptr, &timedout] (const int sig){
#				ifndef NDEBUG
					assert(SIGALRM == sig);
#				else
					if (SIGALRM != sig) std::exit((1<<7)+SIGABRT);
#				endif
				interrupt_print(*ci_ptr, ModelSuite::confCoToShow_,
								mainLog_, lastEstimationStartTime_);
				ci_ptr->reset();
				timedout = true;
			});
			timedout = false;
			Clock::seed_rng();  // restart RNG sequence for this estimation
			alarm(wallTimeInSeconds);
			lastEstimationStartTime_ = omp_get_wtime();
			engine.lock();
			engine.simulate(property,
							min_effort(property.type,
									   engine.name(),
									   engine.current_imp_fun()),
							*ci_ptr,
							techLog_,
							&increase_effort);
            engine.unlock();
			techLog_ << std::endl;
		}
		interruptCI_ = nullptr;

	} else {

		// Simulation bounds are confidence criteria
		engine.interrupted = false;
        for (const auto& criterion: bounds.confidence_criteria()) {
			auto ci_ptr = build_empty_confidence_interval(
							  property.type,
							  engine.splits_per_threshold(),
							  *impFuns[engine.current_imp_fun()],
							  std::get<0>(criterion),
							  std::get<1>(criterion),
							  std::get<2>(criterion));
            interruptCI_ = ci_ptr.get();  // bad boy
			mainLog_ << "   Confidence level: "
					 << std::setprecision(0) << std::fixed
					 << 100*ci_ptr->confidence << "%" << std::endl;
			mainLog_ << "   Precision: ";
            if (ci_ptr->percent)
				mainLog_ << std::setprecision(0) << std::fixed
						 << (200*ci_ptr->errorMargin) << "%\n";
            else
				mainLog_ << std::setprecision(2) << std::scientific
						  << (2*ci_ptr->errorMargin) << "\n";
			size_t effort = min_effort(property.type,
									   engine.name(),
									   engine.current_imp_fun());
			double startTime = omp_get_wtime();

			bool reinit(true);  // start from system's initial state
			Clock::seed_rng();  // restart RNG sequence for this estimation
			lastEstimationStartTime_ = omp_get_wtime();
			engine.lock();
			do {
				bool increaseBatch = engine.simulate(property,
													 effort,
													 *ci_ptr,
													 reinit);
				if (increaseBatch) {
					techLog_ << "-";
					increase_effort(property.type,
									engine.name(),
									engine.current_imp_fun(),
									effort);
				} else {
					techLog_ << "+";
				}
				reinit = false;  // use batch means if possible
            } while (!ci_ptr->is_valid());
            engine.unlock();

			estimate_print(*ci_ptr, omp_get_wtime()-startTime, mainLog_);
			techLog_ << std::endl;
			ci_ptr->reset();
        }
        interruptCI_ = nullptr;
    }

//	mainLog_ << std::defaultfloat;
	mainLog_ << std::setprecision(6);
}


void
ModelSuite::estimate(const size_t& propertyIndex,
					 const SimulationEngine& engine,
					 const StoppingConditions& bounds) const
{
	auto propertyPtr = get_property(propertyIndex);
	if (nullptr == propertyPtr)
		throw_FigException(std::string("no property at index ")
						   .append(std::to_string(propertyIndex)));
	estimate(*propertyPtr, engine, bounds);
}

} // namespace fig
