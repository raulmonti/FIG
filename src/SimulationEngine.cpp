//==============================================================================
//
//  SimulationEngine.cpp
//
//  Copyleft 2016-
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
#include <cmath>
#include <ctime>      // clock()
#include <omp.h>      // omp_get_wtime()
// C++
#include <memory>     // std::dynamic_pointer_cast<>
#include <sstream>
#include <iterator>   // std::begin, std::end
#include <algorithm>  // std::find()
// FIG
#include <SimulationEngine.h>
#include <ImportanceFunction.h>
#include <ImportanceFunctionConcrete.h>
#include <Property.h>
#include <PropertyRate.h>
#include <PropertyTransient.h>
#include <ConfidenceInterval.h>
#include <ConfidenceIntervalRate.h>
#include <ConfidenceIntervalTransient.h>
#include <ModuleNetwork.h>
#include <TraialPool.h>
#include <FigException.h>
#include <FigLog.h>

// ADL
using std::begin;
using std::end;


namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

/// Choose minimum batch size (i.e. requested number of consecutive simulations
/// to run) in order to estimate the value of transient-like properties.
/// Fine tune for the specified SimulationEngine and ImportanceFunction pair
size_t
min_batch_size(const std::string& engineName, const std::string& ifunName)
{
	// Build internal table once: rows follow engine names definition order
	//                            cols follow impFun names definition order
	constexpr size_t NUM_ENGINES(fig::SimulationEngine::NUM_NAMES);
	constexpr size_t NUM_IMPFUNS(fig::ImportanceFunction::NUM_NAMES);
	static const auto& engineNames(fig::SimulationEngine::names());
	static const auto& ifunNames(fig::ImportanceFunction::names());
	// NOTE: optimal batch size == 2^8 was chosen via experimentation with
	//       the tandem queue and the queue with breaks models, using the
	//       "ConfidenceIntervalTransient" class for interval construction.
	static const size_t batch_sizes[NUM_ENGINES][NUM_IMPFUNS] = {
		{ 1ul<<4, 1ul<<3, 1ul<<8 },  // nosplit x {concrete_coupled, concrete_split, algebraic
		{ 1ul<<8, 1ul<<8, 1ul<<8 },  // restart x {concrete_coupled, concrete_split, algebraic}
		{ 1ul<<8, 1ul<<9, 1ul<<8 }   //     sfe x {concrete_coupled, concrete_split, algebraic}
//		{ 1ul<<8, 1ul<<9, 1ul<<8 }   //     bfe x {concrete_coupled, concrete_split, algebraic}
	};
	const auto engineIt = find(begin(engineNames), end(engineNames), engineName);
	const auto ifunIt = find(begin(ifunNames), end(ifunNames), ifunName);
	// Check given engine and importance function names are valid
	if (engineIt == end(engineNames))
		throw_FigException("invalid engine name \""+engineName+"\"");
	if (ifunIt == end(ifunNames))
		throw_FigException("invalid importance function name \""+ifunName+"\"");
	// Return corresponding entry from table
	return batch_sizes[std::distance(begin(engineNames), engineIt)]
					  [std::distance(begin(ifunNames), ifunIt)];
}


/// Choose minimum simulation run length (in simulation-time units)
/// in order to estimate the value of steady-state-like properties.
/// Fine tune for the specified SimulationEngine and ImportanceFunction pair
/// @see increase_run_length
size_t
min_run_length(const std::string& engineName, const std::string& ifunName)
{
	// Build internal table once: rows follow engine names definition order
	//                            cols follow impFun names definition order
	constexpr size_t NUM_ENGINES(fig::SimulationEngine::NUM_NAMES);
	constexpr size_t NUM_IMPFUNS(fig::ImportanceFunction::NUM_NAMES);
	static const auto& engineNames(fig::SimulationEngine::names());
	static const auto& ifunNames(fig::ImportanceFunction::names());
	static const size_t run_lengths[NUM_ENGINES][NUM_IMPFUNS] = {
		{ 1ul<<15, 1ul<<16, 1ul<<16 },  // nosplit x {concrete_coupled, concrete_split, algebraic}
		{ 1ul<<14, 1ul<<14, 1ul<<14 },  // restart x {concrete_coupled, concrete_split, algebraic}
		{ 1ul<<12, 1ul<<12, 1ul<<12 }   //     sfe x {concrete_coupled, concrete_split, algebraic}
//		{ 1ul<<12, 1ul<<12, 1ul<<12 }   //     bfe x {concrete_coupled, concrete_split, algebraic}
	};
	const auto engineIt = find(begin(engineNames), end(engineNames), engineName);
	const auto ifunIt = find(begin(ifunNames), end(ifunNames), ifunName);
	// Check given engine and importance function names are valid
	if (engineIt == end(engineNames))
		throw_FigException("invalid engine name \""+engineName+"\"");
	if (ifunIt == end(ifunNames))
		throw_FigException("invalid importance function name \""+ifunName+"\"");
	// Return corresponding entry from table
	return run_lengths[std::distance(begin(engineNames), engineIt)]
					  [std::distance(begin(ifunNames), ifunIt)];
}


/// Increase given simulation run length (in simulation-time units)
/// in order to estimate the value of steady-state-like properties.
/// Fine tune for the specified SimulationEngine and ImportanceFunction pair
/// @see min_run_length
void
increase_run_length(const std::string& engineName,
					const std::string& ifunName,
					size_t& runLength)
{
	// Build internal table once: rows follow engine names definition order
	//                            cols follow impFun names definition order
	constexpr size_t NUM_ENGINES(fig::SimulationEngine::NUM_NAMES);
	constexpr size_t NUM_IMPFUNS(fig::ImportanceFunction::NUM_NAMES);
	static const auto& engineNames(fig::SimulationEngine::names());
	static const auto& ifunNames(fig::ImportanceFunction::names());
	static const float inc_length[NUM_ENGINES][NUM_IMPFUNS] = {
		{ 1.7f, 1.7f, 1.4f },  // nosplit x {concrete_coupled, concrete_split, algebraic}
		{ 1.4f, 1.4f, 1.4f },  // restart x {concrete_coupled, concrete_split, algebraic}
		{ 1.4f, 1.4f, 1.4f }   //     sfe x {concrete_coupled, concrete_split, algebraic}
//		{ 1.4f, 1.4f, 1.4f }   //     bfe x {concrete_coupled, concrete_split, algebraic}
	};
	const auto engineIt = find(begin(engineNames), end(engineNames), engineName);
	const auto ifunIt = find(begin(ifunNames), end(ifunNames), ifunName);
	// Check given engine and importance function names are valid
	if (engineIt == end(engineNames))
		throw_FigException("invalid engine name \""+engineName+"\"");
	if (ifunIt == end(ifunNames))
		throw_FigException("invalid importance function name \""+ifunName+"\"");
	// Update runLength with corresponding entry from table, rely on type promotion
	runLength *= inc_length[std::distance(begin(engineNames), engineIt)]
						   [std::distance(begin(ifunNames), ifunIt)];
}

//
//	/// Print the latest estimate from the simulations
//	CI_print(fig::ConfidenceInterval ci,
//	         std::ostream out,
//	         int precisionPrint = 2,
//	         bool scientificPrint = true)
//	{
//		if (scientificPrint)
//			out << std::setprecision(2) << std::scientific;
//		else
//			out << std::setprecision(precisionPrint);
//		out << "\nEstimate: " << ci.point_estimate()
//		    << " (var="  << ci.estimation_variance()
//		    << ",prec=" << ci.precision(ci.confidence) << ")";
//		out << std::defaultfloat;
//		//out << std::setprecision(6) << std::fixed;
//	}

} // namespace   // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

SimulationEngine::SimulationEngine(
    const std::string& name,
    std::shared_ptr< const ModuleNetwork> model,
    const bool thresholds) :
		name_(name),
		locked_(false),
        model_(model),
		impFun_(nullptr),
		cImpFun_(nullptr),
        interrupted(false),
        toBuildThresholds_(thresholds),
        reachCount_()
{
	if (std::find(begin(names()), end(names()), name) == end(names())) {
		std::stringstream errMsg;
		errMsg << "invalid engine name \"" << name << "\". ";
		errMsg << "Available engines are";
		for (const auto& name: names())
			errMsg << " \"" << name << "\"";
		errMsg << "\n";
		throw_FigException(errMsg.str());
	}
	if (!model->sealed())
        throw_FigException("ModuleNetwork hasn't been sealed yet");
}


SimulationEngine::~SimulationEngine() noexcept
{
	try {
		unlock();
		unbind();
	} catch(std::exception&) {}
}


void
SimulationEngine::bind(std::shared_ptr< const ImportanceFunction > ifun)
{
	assert(nullptr != ifun);
    if (locked())
        throw_FigException("engine \"" + name() + "\" is currently locked "
                           "in \"simulation mode\"");
	// NOTE: bind/unbind are now exclusive for ModelSuite use,
	//       thus we omit the "ready()" check
	//       which would cause a circular dependency with build_thresholds
	// if (!ifun->ready() && !toBuildThresholds_)
	//        throw_FigException("ImportanceFunction isn't ready for simulations");
	if (bound())
		unbind();
	impFun_ = ifun;
	impFun_->bind_sim_engine(name());
	if (impFun_->concrete())
		cImpFun_ = std::dynamic_pointer_cast<const ImportanceFunctionConcrete>(ifun);
}


void
SimulationEngine::unbind()
{
    if (locked())
        throw_FigException("engine \"" + name() + "\" is currently locked "
                           "in \"simulation mode\"");
	if (nullptr != impFun_)
		impFun_->unbind_sim_engine();
	impFun_.reset();
    cImpFun_.reset();
}


void
SimulationEngine::lock() const
{
    if (locked_)
        throw_FigException("engine \"" + name() + "\" is already locked");
    else
        locked_ = true;
}


void
SimulationEngine::unlock() const noexcept
{
    locked_ = false;
}


const std::array<std::string, SimulationEngine::NUM_NAMES>&
SimulationEngine::names() noexcept
{
	static const std::array< std::string, NUM_NAMES > names =
	{{
		// Standard Monte Carlo simulations, without splitting
		// See SimualtionEngineNosplit class
		"nosplit",

	    // RESTART-like importance splitting, from the Villén-Altamirano brothers
	    // See SimualtionEngineRestart class
	    "restart",

	    // (Standard) Fixed Effort importance splitting, from Garvels' PhD thesis
		// See SimualtionEngineSFE class
		"sfe"
	}};
	return names;
}


const std::string&
SimulationEngine::name() const noexcept
{
	return name_;
}


bool
SimulationEngine::bound() const noexcept
{
    return nullptr != impFun_;
}


bool
SimulationEngine::locked() const noexcept
{
    return locked_;
}


const std::string
SimulationEngine::current_imp_fun() const noexcept
{
	if (nullptr != impFun_)
		return impFun_->name();
	else
		return "";
}


const std::string
SimulationEngine::current_imp_strat() const noexcept
{
	if (nullptr != impFun_)
		return impFun_->strategy();
	else
		return "";
}


void
SimulationEngine::simulate(const Property& property, ConfidenceInterval& ci) const
{
	if (!bound())
		throw_FigException("engine isn't bound to any importance function");
	if (interrupted)
		throw_FigException("called with an interrupted simulation");

	switch (property.type)
	{
	case PropertyType::TRANSIENT: {
		const auto& pTransient(dynamic_cast<const PropertyTransient&>(property));
		auto& ciTransient(dynamic_cast<ConfidenceIntervalTransient&>(ci));
		size_t batchSize = min_batch_size(name(), impFun_->name());
		while ( ! (interrupted || ci.is_valid()) ) {
			auto counts = transient_simulations(pTransient, batchSize);
			transient_update(ciTransient, counts);
		}
		} break;

	case PropertyType::RATE: {
		const auto& pRate(dynamic_cast<const PropertyRate&>(property));
		auto& ciRate(dynamic_cast<ConfidenceIntervalRate&>(ci));
		size_t runLength = min_run_length(name(), impFun_->name());
		bool firstRun(true);
		do {
			std::clock_t t0 = std::clock();
			auto value = rate_simulation(pRate, runLength, firstRun);  // use batch-means
			rate_update(ciRate, value, runLength, (std::clock()-t0)/CLOCKS_PER_SEC);
			firstRun = false;
		} while ( ! (interrupted || ci.is_valid()) );
		} break;

	case PropertyType::THROUGHPUT:
	case PropertyType::RATIO:
	case PropertyType::BOUNDED_REACHABILITY:
		throw_FigException("property type isn't supported by \"" + name_ +
						   "\" simulation engine yet");
		break;

	default:
		throw_FigException("invalid property type");
		break;
	}
}


void
SimulationEngine::transient_update(ConfidenceIntervalTransient& ci,
								   const std::vector<double>& weighedNREs) const
{
	if (interrupted)
		return;  // don't update interrupted simulations

	ci.update(weighedNREs);

	// Print updated CI, providing enough time elapsed since last print
	static constexpr double TIMEOUT_PRINT(M_PI);  // in seconds
	const bool newCI(ci.num_samples() <= 1ul);
	const double thisCallTime(omp_get_wtime());
	static double lastCallTime(newCI ? thisCallTime : lastCallTime);
	static unsigned cnt(newCI ? 0u : cnt);
	if (thisCallTime-lastCallTime > TIMEOUT_PRINT) {
		figTechLog << "\n[" << cnt++ << "] ";
		ci.print(figTechLog);
		lastCallTime = thisCallTime;
	}
//		CI_print(ci, figTechLog);
//		figTechLog << std::setprecision(2) << std::scientific
//				   << "\nEstimate: " << ci.point_estimate()
//				   << " (var="  << ci.estimation_variance()
//				   << ",prec=" << ci.precision(ci.confidence) << ")";
//		figTechLog << std::defaultfloat;
//		//figTechLog << std::setprecision(6) << std::fixed;
//		lastCallTime = thisCallTime;
//	}
}


void
SimulationEngine::rate_update(ConfidenceIntervalRate& ci,
							  const double& rareTime,
							  size_t& simTime,
							  const long& CPUtime) const
{
	if (interrupted)
		return;  // don't update interrupted simulations

	// Try to determine whether we are observing steady-state behaviour
	static constexpr size_t NHITS_REQUIRED = 3u;  // #{"successes"} to acnowledge steady-state behaviour
	static size_t NHITS(0ul);  // #{"successes"} observed in last simulations
	const bool RESET(ci.num_samples() <= 0ul && NHITS >= NHITS_REQUIRED);
	static unsigned cnt(RESET ? 0u : cnt);
	NHITS = RESET ? 0ul : NHITS;
	NHITS += rareTime > MIN_ACC_RARE_TIME ? 1ul : 0ul;
	double thisCallTime(omp_get_wtime());  // time since last update
	static double lastCallTime(omp_get_wtime());
	lastCallTime = RESET || lastCallTime<thisCallTime+.001 ? thisCallTime
	                                                       : lastCallTime;
	const bool BATCH_TAKES_TOO_LONG(thisCallTime-lastCallTime > MAX_CPU_TIME);
	const bool isSteadyState = ci.num_samples() > 0l  ||  // yes, this was decided before
	                           BATCH_TAKES_TOO_LONG   ||  // yes, time constraints force us
							   NHITS >= NHITS_REQUIRED;   // yes, we succeeded enough times
	if (isSteadyState) {
		// Reduce fp precision loss (is this any good?)
		const double thisRate(std::exp(std::log(rareTime)-std::log(simTime)));
		ci.update(thisRate);
		figTechLog << "\n[" << cnt++ << "] ";
		ci.print(figTechLog);
//		CI_print(ci, figTechLog);
//		// Print updated CI (detailed progress report)
//		figTechLog << std::setprecision(2) << std::scientific
//		           << "\nEstimate: " << ci.point_estimate()
//				   << " (var=" << ci.estimation_variance()
//		           << ",prec=" << ci.precision(ci.confidence) << ")";
//		figTechLog << std::defaultfloat;
//		//figTechLog << std::setprecision(6) << std::fixed;
	} else {
		increase_run_length(name_, impFun_->name(), simTime);
		figTechLog << "*";  // report "discarded"
	}
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
