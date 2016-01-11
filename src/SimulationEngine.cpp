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



// C++
#include <sstream>
#include <iterator>   // std::begin, std::end
#include <algorithm>  // std::find()
// FIG
#include <SimulationEngine.h>
#include <FigException.h>
#include <ModuleNetwork.h>

// ADL
using std::begin;
using std::end;
using std::find;


namespace fig
{

const std::array< std::string, 1 > SimulationEngine::names =
{
    {"nosplit"}  // Standard Monte Carlo simulations, without splitting
};


SimulationEngine::SimulationEngine(
	const std::string& name,
    std::shared_ptr< const ModuleNetwork> network) :
		name_(name),
		network_(network),
		impFun(nullptr)
{
	if (find(begin(names), end(names), name) == end(names)) {
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
    return nullptr != impFun;
}


void
SimulationEngine::bind(std::shared_ptr< const ImportanceFunction > ifun) noexcept
{
	impFun = ifun;
}


void
SimulationEngine::unbind() noexcept
{
	impFun = nullptr;
}

} // namespace fig
