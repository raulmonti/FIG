//==============================================================================
//
//  SimulationEngineFixedEffort.cpp
//
//  Copyleft 2017-
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
#include <algorithm>  // std::fill(), std::move()
// FIG
#include <SimulationEngineFixedEffort.h>
#include <PropertyTransient.h>
#include <TraialPool.h>
#include <ModelSuite.h>

// ADL
using std::begin;
using std::end;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

SimulationEngineFixedEffort::SimulationEngineFixedEffort(
	const std::string& simEngineName,
    std::shared_ptr<const ModuleNetwork> network,
    unsigned effortPerLevel) :
		SimulationEngine(simEngineName, network),
        effortPerLevel_(effortPerLevel)
{ /* Not much to do around here */ }


SimulationEngineFixedEffort::~SimulationEngineFixedEffort()
{
	//TraialPool::get_instance().return_traials(traials_);
	// ^^^ pointless, and besides the TraialPool might be dead already,
	//     so this would trigger a re-creation of the pool
}


unsigned
SimulationEngineFixedEffort::global_effort() const noexcept
{
	return effortPerLevel_;
}


void
SimulationEngineFixedEffort::bind(std::shared_ptr< const ImportanceFunction > ifun_ptr)
{
	if (ifun_ptr->strategy() == "")
		throw_FigException("ImportanceFunction doesn't seem to have "
		                   "internal importance information");
	else if (ifun_ptr->strategy() == "flat")
		throw_FigException("RESTART simulation engine requires an importance "
		                   "building strategy other than \"flat\"");
	SimulationEngine::bind(ifun_ptr);
}


void
SimulationEngineFixedEffort::set_global_effort(unsigned epl)
{
	if (locked())
		throw_FigException("engine \"" + name() + "\" is currently locked "
		                   "in \"simulation mode\"");
	if (epl < 2u)
		throw_FigException("bad global effort per level \"" + std::to_string(epl) + "\". "
		                   "At least 2 simulations must be launched per level "
		                   "to guarantee some basic statistical properties");
	effortPerLevel_ = epl;
}


} // namespace fig  // // // // // // // // // // // // // // // // // // // //
