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
#include <ModuleNetwork.h>
#include <FigException.h>
#include <TraialPool.h>

// ADL
using std::begin;
using std::end;


namespace fig
{

// Static variables initialization

/// @note Arbitrary af
const unsigned SimulationEngine::MIN_COUNT_RARE_EVENTS = 5u;

/// @note Arbitrary af
const double SimulationEngine::MIN_ACC_RARE_TIME = M_PI_4l/2.0;

/// @note Small enough to distinguish variations of 0.01 simulation time units
///       when using fp single precision: mantissa 1, exponent 12, resulting in
///       1*2^12 == 4096. The corresponding C99 literal is 0x1p12
/// @see <a href="http://www.cprogramming.com/tutorial/floating_point/understanding_floating_point_representation.html">
///      Floating point arithmetic</a> and the <a href="http://stackoverflow.com/a/4825867">
///      C99 fp literals</a>.
const CLOCK_INTERNAL_TYPE SimulationEngine::SIM_TIME_CHUNK = 4096.f;



// SimulationEngine class member functions

SimulationEngine::SimulationEngine(
	const std::string& name,
    std::shared_ptr< const ModuleNetwork> network) :
		name_(name),
		locked_(false),
		network_(network),
		impFun_(nullptr),
		cImpFun_(nullptr),
        interrupted(false)
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
    if (!network->sealed())
        throw_FigException("ModuleNetwork hasn't been sealed yet");
}


SimulationEngine::~SimulationEngine()
{
	unlock();
    unbind();
}


void
SimulationEngine::bind(std::shared_ptr< const ImportanceFunction > ifun)
{
	assert(nullptr != ifun);
	if (!ifun->ready())
		throw_FigException("ImportanceFunction isn't ready for simulations");
    if (locked())
        throw_FigException("engine \"" + name() + "\" is currently locked "
                           "in \"simulation mode\"");
    impFun_ = ifun;
	if (ifun->concrete())
		cImpFun_ = std::dynamic_pointer_cast<const ImportanceFunctionConcrete>(ifun);
}


void
SimulationEngine::unbind() noexcept
{
    if (locked())
        throw_FigException("engine \"" + name() + "\" is currently locked "
                           "in \"simulation mode\"");
    impFun_  = nullptr;
	cImpFun_ = nullptr;
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
		"restart"
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


bool
SimulationEngine::simulate(const Property &property,
						   const size_t& effort,
						   ConfidenceInterval& interval,
						   bool reinit) const
{
	bool increaseEffort(false);
	assert(0ul < effort);
	if (!bound())
		throw_FigException("engine isn't bound to any importance function");

	switch (property.type) {

	case PropertyType::TRANSIENT: {
		assert(interval.name == "proportion_std" ||
			   interval.name == "proportion_wilson");
		double raresCount =
			transient_simulations(dynamic_cast<const PropertyTransient&>(property),
								  effort);
		// numExperiments == numRuns * splitsPerThreshold ^ numThresholds
		interval.update(std::abs(raresCount),
						std::log(effort) + log_experiments_per_sim());
		increaseEffort = 0.0 >= raresCount;
		} break;

	case PropertyType::RATE: {
		assert(interval.name == "mean_std");
		double rate = rate_simulation(dynamic_cast<const PropertyRate&>(property),
									  effort,
									  reinit);
		interval.update(std::abs(rate));
		increaseEffort = 0.0 >= rate;
		} break;

	case PropertyType::THROUGHPUT:
	case PropertyType::RATIO:
	case PropertyType::BOUNDED_REACHABILITY:
		throw_FigException(std::string("property type isn't supported by ")
						   .append(name_).append(" simulation yet"));
		break;

	default:
		throw_FigException("invalid property type");
		break;
	}

	return increaseEffort;
}


void
SimulationEngine::simulate(
	const Property& property,
	size_t effort,
	ConfidenceInterval& interval,
	std::ostream& techLog,
	void (*effort_inc)(const PropertyType&,
					   const std::string&,
					   const std::string&,
					   size_t &)) const
{
	assert(0ul < effort);
	if (!bound())
		throw_FigException("engine isn't bound to any importance function");

	switch (property.type) {

	case PropertyType::TRANSIENT:
		assert(interval.name == "proportion_std" ||
			   interval.name == "proportion_wilson");
		assert (!interrupted);
		while (!interrupted) {
			double raresCount =
				transient_simulations(dynamic_cast<const PropertyTransient&>(property),
									  effort);
			if (!interrupted) {
				// numExperiments == batchSize * splitsPerThreshold ^ numThresholds
				interval.update(std::abs(raresCount),
								std::log(effort) + log_experiments_per_sim());
				if (0.0 >= raresCount) {
					techLog << "-";
					if (nullptr != effort_inc)
						effort_inc(property.type, name_, impFun_->name(), effort);
					else
						effort *= 2;  // you left us with no other option
				} else {
					techLog << "+";
				}
			}
		}
		break;

	case PropertyType::RATE: {
		bool reinit(true);  // start from system's initial state
		assert(interval.name == "mean_std");
		assert (!interrupted);
		while (!interrupted) {
			double rate = rate_simulation(dynamic_cast<const PropertyRate&>(property),
										  effort,
										  reinit);
			if (!interrupted) {
				interval.update(std::abs(rate));
				if (0.0 >= rate) {
					techLog << "-";
					if (nullptr != effort_inc)
						effort_inc(property.type, name_, impFun_->name(), effort);
					else
						effort *= 2;  // you left us with no other option
				} else {
					techLog << "+";
				}
			}
			reinit = false;  // use batch means
		}
		} break;

	case PropertyType::THROUGHPUT:
	case PropertyType::RATIO:
	case PropertyType::BOUNDED_REACHABILITY:
		throw_FigException(std::string("property type isn't supported by ")
						   .append(name_).append(" simulation yet"));
		break;

	default:
		throw_FigException("invalid property type");
		break;
	}
}

} // namespace fig
