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
#include <cstdio>     // std::sprintf()
#include <unistd.h>   // alarm(), exit()
#include <pthread.h>  // pthread_cancel()
#include <cmath>      // std::pow()
#include <omp.h>      // omp_get_wtime()
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
#include <functional>   // std::ref()
#include <algorithm>    // std::find();
#include <iterator>     // std::begin(), std::end(), std::distance()
#include <string>
#include <ios>          // std::scientific, std::fixed
#include <iomanip>      // std::setprecision()
#include <thread>
// FIG
#include <ModelSuite.h>
#include <FigException.h>
#include <FigLog.h>
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
#include <ConfidenceIntervalResult.h>
#include <ConfidenceIntervalRate.h>
#include <ConfidenceIntervalTransient.h>

using std::to_string;
// ADL
using std::find;
using std::begin;
using std::end;


namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

using fig::ConfidenceInterval;
using std::chrono::seconds;


/**
 * @brief Build a ConfidenceInterval of the required type
 *
 *        Each PropertyType must be estimated using a special kind of
 *        ConfidenceInterval. This helper function returns a new (i.e. without
 *        estimation data) interval of the correct kind for the property.
 *
 * @param propertyType     Type of the property whose value is being estimated
 * @param confidenceCo     Interval's confidence coefficient ∈ (0.0, 1.0)
 * @param precision        Interval's desired full width > 0.0
 * @param dynamicPrecision Is the precision a percentage of the estimate?
 *
 * @return Fresh ConfidenceInterval tailored for the given property
 *
 * @note If no confidence criteria is passed then a "time simulation"
 *       is assumed and the interval is built with the tightest constraints.
 *
 * @throw FigException if property type or hint isn't valid
 */
std::shared_ptr< ConfidenceInterval >
build_empty_ci(const fig::PropertyType& propertyType,
			   double confidenceCo = -1.0,
			   double precision = -1.0,
			   const bool& dynamicPrecision = true)
{
	std::shared_ptr< ConfidenceInterval > ci_ptr(nullptr);

	bool timeBoundSim(false);
	if (confidenceCo <= 0.0) {
		confidenceCo = 0.999999;
		precision    = 0.000001;
		timeBoundSim = true;
	}

	switch (propertyType)
	{
	case fig::PropertyType::TRANSIENT: {
		ci_ptr.reset(new fig::ConfidenceIntervalTransient(confidenceCo,
														  precision,
														  dynamicPrecision,
														  timeBoundSim));
//		// The statistical oversampling incurred here is bounded:
//		//  · from below by splitsPerThreshold ^ minRareValue,
//		//  · from above by splitsPerThreshold ^ numThresholds.
//		// NOTE: Deprecated - This was used by the binomial proportion CIs
//		double minStatOversamp = std::pow(splitsPerThreshold,
//										  impFun.min_rare_value());
//		double maxStatOversamp = std::pow(splitsPerThreshold,
//										  impFun.num_thresholds());
//		ci_ptr->set_statistical_oversampling(maxStatOversamp);
//		ci_ptr->set_variance_correction(minStatOversamp/maxStatOversamp);
		} break;

    case fig::PropertyType::RATE:
		ci_ptr.reset(new fig::ConfidenceIntervalRate(confidenceCo,
													 precision,
													 dynamicPrecision,
													 timeBoundSim));
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
interrupt_print(const ConfidenceInterval& ci,
				const std::vector<float>& confidenceCoefficients,
				std::ostream& out,
				const double& startTime = -1.0)
{
    /// @todo TODO: implement proper reentrant logging and discard use of streams
    out << std::endl;
	out << std::setprecision(2) << std::scientific;
	out << "   · Computed estimate: " << ci.point_estimate() << " ("
									  << ci.num_samples() << " samples)\n";
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
estimate_print(const ConfidenceInterval& ci,
               const double& time,
               std::ostream& out)
{
    out << std::endl;
	out << std::setprecision(2) << std::scientific;
	out << "   · Computed estimate: " << ci.point_estimate() << " ("
									  << ci.num_samples() << " samples)\n";
	out << std::setprecision(2) << std::scientific;
	out << "   · Computed precision: " << ci.precision(ci.confidence) << std::endl;
	out << "   · Precision: " << ci.precision() << std::endl;
	out << "   · Confidence interval: [ " << ci.lower_limit() << ", "
										  << ci.upper_limit() << " ] " << std::endl;
	out << std::setprecision(2) << std::fixed;
	out << "   · Estimation time: " << time << " s\n";
//	out << std::defaultfloat;
    out << std::setprecision(6) << std::fixed;
    out << std::endl;
}


/**
 * @brief Sleep during 'timeLimit'; then write in 'timeoutSignal' and show 'ci'
 * @details Sleep is done on this thread; the user should call this function
 *          via std::thread if a "background timeout" is desired, e.g.
 *          std::thread timer(start_timer, std::ref(arg1)...);<br>
 *          The thread can then be cancelled via pthread_cancel() e.g. if the
 *          simulation succeeds and these actions should be avoided.
 * @param ci            Confidence interval to print out and reset
 * @param timeoutSignal Shared variable signaling simulations to stop
 * @param timeLimit     Duration (in seconds) of the sleep
 * @param out           Stream where results will be printed out
 * @param startTime     Result of omp_get_wtime() at simulations start
 */
void
start_timer(ConfidenceInterval& ci,
			bool& timeoutSignal,
			const seconds& timeLimit,
			std::ostream& out,
            const double& startTime)
{
	std::this_thread::sleep_for(timeLimit);
	timeoutSignal = true;  // this should stop computations
	interrupt_print(ci, fig::ModelSuite::get_cc_to_show(), out, startTime);
// * @param reset         Reset ConfidenceInterval after printing?
//	if (reset)
//		ci.reset();
}


/// Format time given in seconds as a hh:mm:ss string
std::string
time_formatted_str(size_t timeInSeconds)
{
	const size_t hours(timeInSeconds/3600ul);
	const size_t minutes((timeInSeconds%3600ul)/60ul);
	const size_t seconds(timeInSeconds%60ul);
	char timeStr[9] = {'\0'};
	std::sprintf(timeStr, "%02zu:%02zu:%02zu", hours, minutes, seconds);
	return timeStr;
}

} // namespace  // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
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

std::ostream& ModelSuite::mainLog_(figMainLog);

std::ostream& ModelSuite::techLog_(figTechLog);

double ModelSuite::lastEstimationStartTime_;

seconds ModelSuite::timeout_(0l);

std::vector< ConfidenceIntervalResult > ModelSuite::lastEstimates_;

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


ModelSuite::~ModelSuite() { /* Just relax and let everything go */ }


void ModelSuite::add_module(std::shared_ptr< ModuleInstance >& module)
{
	model->add_module(module);
}


size_t ModelSuite::add_property(std::shared_ptr<Property> property)
{
	properties.push_back(property);
	return properties.size()-1ul;
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
        prop->prepare(model->global_state());

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
ModelSuite::set_splitting(const unsigned& spt, bool verbose)
{
    if (!sealed())
        throw_FigException("ModelSuite hasn't been sealed yet");
	if (1u < spt)  // i.e. if we actually use splitting
		dynamic_cast<SimulationEngineRestart&>(*simulators["restart"])
				.set_splits_per_threshold(spt);
	splitsPerThreshold = spt;
	if (verbose)
		// Show in tech log
		tech_log("\nSplitting set to " + std::to_string(spt) + "\n");
}


void
ModelSuite::set_timeout(const seconds& timeLimit)
{
	if (!sealed())
		throw_FigException("ModelSuite hasn't been sealed yet");
	timeout_ = timeLimit;
	// Show in tech log
	if (timeLimit.count() > 0l)
		tech_log("Timeout set to " + time_formatted_str(timeout_.count()) + "\n");
	else
		tech_log("Timeout was unset\n");
}


void
ModelSuite::set_rng(const std::string& rngType, const size_t& rngSeed)
{
	const auto& valid_RNGs(available_RNGs());
	if (find(begin(valid_RNGs), end(valid_RNGs), rngType) == end(valid_RNGs))
		throw_FigException("invalid RNG specified: " + rngType);

	Clock::change_rng_seed(rngSeed);  // change seed first! (seeding policies)
	Clock::change_rng(rngType);

	tech_log("Using RNG \"" + rngType + "\" with ");
	if (0ul == rngSeed)
		tech_log("randomized seeding.\n");
	else
		tech_log("seed " + to_string(rngSeed) + "\n");
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


const seconds&
ModelSuite::get_timeout() const noexcept
{
	return timeout_;
}


const std::vector< float >&
ModelSuite::get_cc_to_show() noexcept
{
	return confCoToShow_;
}

const std::vector< ConfidenceIntervalResult >&
ModelSuite::get_last_estimates() noexcept
{
	return lastEstimates_;
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
ModelSuite::available_importance_post_processings() noexcept
{
	static std::vector< std::string > importancePostProcessings;
	if (importancePostProcessings.empty()) {
		importancePostProcessings.reserve(num_importance_post_processings());
		for (const auto& pp: ImportanceFunctionConcrete::post_processings())
			importancePostProcessings.push_back(pp);
	}
	return importancePostProcessings;
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


const std::vector< std::string >&
ModelSuite::available_RNGs() noexcept
{
	static std::vector< std::string > RNGs;
	if (RNGs.empty()) {
		RNGs.reserve(num_RNGs());
		for (const auto& rng: Clock::RNGs())
			RNGs.push_back(rng);
	}
	return RNGs;
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
ModelSuite::exists_importance_post_processing(const std::string& postProc) noexcept
{
	const auto& impPostProc = available_importance_post_processings();
	return postProc.empty() ||
			end(impPostProc) != find(begin(impPostProc), end(impPostProc), postProc);
}


bool
ModelSuite::exists_threshold_technique(const std::string& thrTechnique) noexcept
{
	const auto& thrTechs = available_threshold_techniques();
	return find(begin(thrTechs), end(thrTechs), thrTechnique) != end(thrTechs);
}


bool
ModelSuite::exists_rng(const std::string& rng) noexcept
{
	const auto& RNGs = available_RNGs();
	return find(begin(RNGs), end(RNGs), rng) != end(RNGs);
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
        techLog_ << "Property: " << property.to_string() << std::endl;
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
ModelSuite::build_importance_function_adhoc(const ImpFunSpec& impFun,
											const Property& property,
											bool force)
{
	if (!exists_importance_function(impFun.name))
		throw_FigException("inexistent importance function \"" + impFun.name +
						   "\". Call \"available_importance_functions()\" "
						   "for a list of available options.");

	ImportanceFunction& ifun = *impFuns[impFun.name];
	// "adhoc" strategy is compatible with all ImportanceFunction derived types

	if (force || !ifun.has_importance_info() || "adhoc" != ifun.strategy()) {
		techLog_ << "\nBuilding importance function \"" << impFun.name
				 << "\" with \"adhoc\" assessment strategy (\""
				 << impFun.algebraicFormula << "\")\n";
        techLog_ << "Property: " << property.to_string() << std::endl;
		ifun.clear();
		auto allVarnames = model->global_state().varnames();
		const double startTime = omp_get_wtime();
		if (ifun.concrete()) {
			std::vector<std::string> varnamesVec(begin(allVarnames), end(allVarnames));
			static_cast<ImportanceFunctionConcrete&>(ifun)
				.assess_importance(property, impFun.algebraicFormula, varnamesVec);
		} else {
			static_cast<ImportanceFunctionAlgebraic&>(ifun)
				.set_formula("adhoc",
							 impFun.algebraicFormula,
							 allVarnames,
							 model->global_state(),
							 property,
							 impFun.minValue,
							 impFun.maxValue);
		}
		if (PostProcessing::NONE != impFun.postProcessing.type)
			techLog_ << "\n[WARNING] post-processing \"" << impFun.postProcessing.name
					 << "\" ignored; can't specify a post-processing for "
					 << "\"adhoc\" importance assessment (build that into "
					 << "the expression you provided!)\n";
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


void
ModelSuite::build_importance_function_adhoc(const ImpFunSpec& impFun,
											const size_t& propertyIndex,
											bool force)
{
	auto propertyPtr = get_property(propertyIndex);
	if (nullptr == propertyPtr)
		throw_FigException("no property at index " + to_string(propertyIndex));
	build_importance_function_adhoc(impFun, *propertyPtr, force);
}


void
ModelSuite::build_importance_function_auto(const ImpFunSpec& impFun,
										   const Property& property,
										   bool force)
{
	if (!exists_importance_function(impFun.name))
		throw_FigException("inexistent importance function \"" + impFun.name +
						   "\". Call \"available_importance_functions()\" "
						   "for a list of available options.");

	ImportanceFunction& ifun = *impFuns[impFun.name];
    if (!ifun.concrete())
		throw_FigException("requested to build a non-concrete importance "
						   "function (\"" + impFun.name + "\") using the "
						   "\"auto\" importance assessment strategy");

	if (property.is_rare(model->initial_state()))
		throw_FigException("automatic importance functions don't tolerate "
						   "a rare initial system state (property: " +
						   property.to_string() + " ; initial state: " +
						   model->initial_state().to_string() + ")");

    if (force || !ifun.has_importance_info() || "auto" != ifun.strategy()) {
		techLog_ << "\nBuilding importance function \"" << impFun.name
				 << "\" with \"auto\" assessment strategy.\n";
        techLog_ << "Property: " << property.to_string() << std::endl;
		ifun.clear();
		const double startTime = omp_get_wtime();
		// Compositional importance functions need a composition function
		if (impFun.name.find("split") != std::string::npos)
			static_cast<ImportanceFunctionConcreteSplit&>(ifun)
				.set_composition_fun(impFun.algebraicFormula,
									 impFun.neutralElement,
									 impFun.minValue,
									 impFun.maxValue);

		try {
			// Compute importance automatically -- here hides the magic!
            static_cast<ImportanceFunctionConcrete&>(ifun)
					.assess_importance(property, "auto", impFun.postProcessing);
		} catch (std::bad_alloc&) {
			throw_FigException("couldn't build importance function \""
							   + impFun.name + "\" automatically: not enough "
							   "system memory");
		} catch (FigException& e) {
			throw_FigException("couldn't build importance function \""
							   + impFun.name + "\" automatically: " + e.msg());
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
ModelSuite::build_importance_function_auto(const ImpFunSpec& impFun,
										   const size_t& propertyIndex,
										   bool force)
{
    auto propertyPtr = get_property(propertyIndex);
    if (nullptr == propertyPtr)
		throw_FigException("no property at index " + to_string(propertyIndex));
	build_importance_function_auto(impFun, *propertyPtr, force);
}


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
		techLog_ << "\nBuilding thresholds for importance function \"" << ifunName
				 << "\",\nusing technique \"" << technique << "\" with splitting "
				 << "== " << splitsPerThreshold << std::endl;
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
	techLog_ << " · releasing resources";
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
	techLog_ << "\nReleasing all system resources:\n";
	try {
		for (auto ifunName: available_importance_functions())
			release_resources(ifunName);
		for (auto engineName: available_simulators()) {
			simulators[engineName]->unlock();
			simulators[engineName]->unbind();
		}
	} catch (std::exception&) {}  // Meh... everything was going to hell anyway
}


void
ModelSuite::estimate(const Property& property,
                     const SimulationEngine& engine,
                     const StoppingConditions& bounds) const
{
	lastEstimates_.clear();
	lastEstimates_.reserve(bounds.size());

	if (!engine.ready())
		throw_FigException("SimulationEngine \"" + engine.name()
						  +"\" isn't ready for simulations");
	const ImportanceFunction& ifun(*impFuns[engine.current_imp_fun()]);
	const std::string adHocFun(ifun.adhoc_fun());
	const std::string postProcStr(ifun.post_processing().name.empty()
			? ("(null)") : (ifun.post_processing().name + " "
							+ to_string(ifun.post_processing().value)));

	mainLog_ << "RNG algorithm used: " << Clock::rng_type() << "\n";
    mainLog_ << "Estimating " << property.to_string() << ",\n";
	mainLog_ << " using simulation engine  \"" << engine.name() << "\"\n";
	mainLog_ << " with importance function \"" << engine.current_imp_fun() << "\"\n";
	mainLog_ << " built using strategy     \"" << engine.current_imp_strat() << "\"";
	mainLog_ << (adHocFun.empty() ? ("") : (" ("+adHocFun+")")) << std::endl;
	mainLog_ << " with post-processing     \"" << postProcStr << "\"\n";
	mainLog_ << " and thresholds technique \"" << ifun.thresholds_technique() << "\"\n";
	mainLog_ << " [ " << ifun.num_thresholds() << " thresholds";
	mainLog_ << " | splitting " << engine.splits_per_threshold() << " ]\n";

	if (bounds.is_time())
		// Simulation bounds are wall clock time limits
		estimate_for_times(property, engine, bounds);
	else
		// Simulation bounds are confidence criteria
		estimate_for_confs(property, engine, bounds);

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
		throw_FigException("no property at index " + to_string(propertyIndex));
	estimate(*propertyPtr, engine, bounds);
}


void
ModelSuite::estimate_for_times(const Property& property,
							   const SimulationEngine& engine,
							   const StoppingConditions& bounds) const
{
	assert(bounds.is_time_budgets());

	for (const unsigned long& wallTimeInSeconds: bounds.time_budgets()) {

		// Configure simulation
		auto ci_ptr = build_empty_ci(property.type);
		interruptCI_ = ci_ptr.get();  // bad boy
		engine.interrupted = false;
		lastEstimationStartTime_ = omp_get_wtime();
		Clock::seed_rng();  // restart RNG sequence for this estimation

		// Show simulation run info
		const seconds timeLimit(timeout_.count() > 0l
		        ? std::min<long>(wallTimeInSeconds, timeout_.count())
		        : wallTimeInSeconds);
		mainLog_ << std::setprecision(0) << std::fixed;
		mainLog_ << "   Estimation time bound: " << time_formatted_str(timeLimit.count()) << "\n";
		mainLog_ << "   RNG seed: " << Clock::rng_seed()
		         << (Clock::rng_seed_is_random() ? (" (randomized)\n") : ("\n"));

		// Start timer
		std::thread timer(start_timer, std::ref(*ci_ptr), std::ref(engine.interrupted),
		                               timeLimit, std::ref(mainLog_), lastEstimationStartTime_);
		// Simulate
		try {
			engine.lock();
			engine.simulate(property, *ci_ptr);
			engine.unlock();
			timer.join();  // must've timed-out already

		} catch (std::exception&) {
			engine.unlock();
			pthread_cancel(timer.native_handle());  // cancel pending timeout
			timer.detach();
			throw;
		}
		techLog_ << std::endl;
		interruptCI_ = nullptr;
		lastEstimates_.push_back(*ci_ptr);

		/// @todo TODO erase debug print
		std::cout << std::setprecision(8);
		std::cout << "   --> " << ci_ptr->precision(.9) << " <--" << std::endl;
		std::cout << "   ~~) " << lastEstimates_.back().precision(.9) << " (~~" << std::endl;

		// Results should've been shown on TO interruption
	}
}


void
ModelSuite::estimate_for_confs(const Property& property,
							   const SimulationEngine& engine,
							   const StoppingConditions& bounds) const
{
	assert(bounds.is_confidence_criteria());
	const seconds timeLimit = timeout_.count() > 0l ? timeout_
													: seconds(9999999l);
	for (const auto& criterion: bounds.confidence_criteria()) {

		const double confCo(std::get<0>(criterion)),   // confidence coefficient
					 precVal(std::get<1>(criterion));  // precision to achieve
		const bool precRel(std::get<2>(criterion));    // is precision relative?

		// Configure simulation
		auto ci_ptr = build_empty_ci(property.type, confCo, precVal, precRel);
		interruptCI_ = ci_ptr.get();  // bad boy
		engine.interrupted = false;
		lastEstimationStartTime_ = omp_get_wtime();
		Clock::seed_rng();  // restart RNG sequence for this estimation

		// Show simulation run info
		mainLog_ << "   Confidence level: "
		         << std::setprecision(0) << std::fixed << 100*(confCo) << "%\n";
		mainLog_ << "   Precision: ";
		if (precRel)
			mainLog_ << std::setprecision(0) << std::fixed << (100*precVal) << "%\n";
		else
			mainLog_ << std::setprecision(2) << std::scientific << (2*precVal) << "\n";
		if (timeout_.count() > 0l)
			mainLog_ << "   Timeout: " << time_formatted_str(timeout_.count()) << "\n";
		mainLog_ << "   RNG seed: " << Clock::rng_seed()
		         << (Clock::rng_seed_is_random() ? (" (randomized)\n") : ("\n"));

		// Start timer
		std::thread timer(start_timer, std::ref(*ci_ptr), std::ref(engine.interrupted),
		                               timeLimit, std::ref(mainLog_), lastEstimationStartTime_);
		// Simulate
		try {
			engine.lock();
			engine.simulate(property, *ci_ptr);
			engine.unlock();

		} catch (std::exception&) {
			engine.unlock();
			pthread_cancel(timer.native_handle());  // cancel pending TO
			timer.detach();
			throw;
		}

		// Show results
		if (!engine.interrupted) {
			// Simulations succeeded!
			pthread_cancel(timer.native_handle());  // cancel pending TO
			timer.detach();
			estimate_print(*ci_ptr, omp_get_wtime()-lastEstimationStartTime_, mainLog_);
			techLog_ << std::endl;
			ci_ptr->reset();
		} else {
			// Simulations timed-out: wait for interrupt_print() to finish
			timer.join();
		}
		interruptCI_ = nullptr;
		lastEstimates_.push_back(*ci_ptr);
	}
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
