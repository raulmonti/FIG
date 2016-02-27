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

const std::array< std::string, 2 > SimulationEngine::names =
{{
	// Standard Monte Carlo simulations, without splitting
	"nosplit",

	// RESTART-like importance splitting, from the Villén-Altamirano brothers
	"restart"
}};


// SimulationEngine class member functions

SimulationEngine::SimulationEngine(
	const std::string& name,
    std::shared_ptr< const ModuleNetwork> network) :
		name_(name),
		network_(network),
		impFun_(nullptr),
		cImpFun_(nullptr),
        interrupted(false)
{
	if (std::find(begin(names), end(names), name) == end(names)) {
		std::stringstream errMsg;
		errMsg << "invalid engine name \"" << name << "\". ";
		errMsg << "Available engines are";
		for (const auto& name: names)
			errMsg << " \"" << name << "\"";
		errMsg << "\n";
		throw_FigException(errMsg.str());
	}
    if (!network->sealed())
        throw_FigException("ModuleNetwork hasn't been sealed yet");
}


SimulationEngine::~SimulationEngine()
{
    unbind();
}


bool
SimulationEngine::bound() const noexcept
{
	return nullptr != impFun_;
}


void
SimulationEngine::bind(std::shared_ptr< const ImportanceFunction > ifun)
{
	assert(nullptr != ifun);
	if (!ifun->ready())
		throw_FigException("ImportanceFunction isn't ready for simulations");
    impFun_ = ifun;
	if (ifun->concrete())
		cImpFun_ = std::dynamic_pointer_cast<const ImportanceFunctionConcrete>(ifun);
}


void
SimulationEngine::unbind() noexcept
{
    impFun_  = nullptr;
	cImpFun_ = nullptr;
}


const std::string&
SimulationEngine::name() const noexcept
{
	return name_;
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
						   const size_t& numRuns,
						   ConfidenceInterval& interval) const
{
	assert(0ul < numRuns);
	if (!bound())
		throw_FigException("engine isn't bound to any importance function");

	switch (property.type) {

	case PropertyType::TRANSIENT: {
		double raresCount =
			transient_simulations(dynamic_cast<const PropertyTransient&>(property),
								  numRuns);
		// numExperiments = numRuns * splitsPerThreshold ^ numThresholds
		interval.update(std::abs(raresCount),
						std::log(numRuns) + log_experiments_per_sim());
        if (0.0 < raresCount)
			return false;
        else
			return true;  // you'd better increase 'numRuns'
		}

	case PropertyType::THROUGHPUT:
	case PropertyType::RATE:
	case PropertyType::RATIO:
	case PropertyType::BOUNDED_REACHABILITY:
		throw_FigException(std::string("property type isn't supported by ")
						   .append(name_).append(" simulation yet"));
		return false;

	default:
		throw_FigException("invalid property type");
		return false;
	}
}


void
SimulationEngine::simulate(const Property& property,
						   size_t batchSize,
						   ConfidenceInterval& interval,
						   void (*batch_inc)(size_t&, ConstStr&, ConstStr&)) const
{
	assert(0ul < batchSize);
	if (!bound())
		throw_FigException("engine isn't bound to any importance function");

	switch (property.type) {

	case PropertyType::TRANSIENT:
		assert (!interrupted);
		while (!interrupted) {
			double raresCount =
				transient_simulations(dynamic_cast<const PropertyTransient&>(property),
                                      batchSize);
			if (!interrupted) {
				// numExperiments = batchSize * splitsPerThreshold ^ numThresholds
				interval.update(std::abs(raresCount),
								std::log(batchSize) + log_experiments_per_sim());
				if (0.0 >= raresCount) {
					std::cerr << "-";
					if (nullptr != batch_inc)
						batch_inc(batchSize, name_, impFun_->name());
					else
						batchSize *= 2;  // you left us with no other option
				} else {
					std::cerr << "+";
				}
			}
		}
		break;

	case PropertyType::THROUGHPUT:
	case PropertyType::RATE:
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
