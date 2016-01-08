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


#include <SimulationEngine.h>


namespace fig
{

const std::array< std::string, 1 > SimulationEngine::names =
{
	{"nosplit"}
};


SimulationEngine::SimulationEngine(
	const std::string& name,
	std::shared_ptr< const ModuleNetwork>& network) :
		name_(name),
		network_(network),
		property(nullptr),
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
}


SimulationEngine::~SimulationEngine()
{
	unload();
}


bool
SimulationEngine::loaded() const noexcept
{
	return nullptr != property && nullptr != impFun;
}


void
SimulationEngine::load(const Property& prop,
					   std::shared_ptr< const ImportanceFunction > ifun) noexcept
{
	property = &prop;
	impFun = ifun;
}


void
SimulationEngine::unload() noexcept
{
	property = nullptr;
	impFun = nullptr;
}

} // namespace fig
