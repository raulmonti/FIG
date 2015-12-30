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


#include <ModelSuite.h>


namespace fig
{

// Static variables initialization

std::unique_ptr< ModuleNetwork > ModelSuite::model(new ModuleNetwork);

std::vector< Reference< Property > > ModelSuite::properties;

class StoppingConditions {}; /// @todo TODO implement class and erase this dummy
StoppingConditions ModelSuite::simulationBounds;

std::unordered_map< std::string, std::shared_ptr< ImportanceFunction > >
	ModelSuite::impFuns;

std::unordered_map< std::string, Reference< SimulationEngine > >
	ModelSuite::simulators;

std::unique_ptr< ModelSuite > ModelSuite::instance_ = nullptr;

std::once_flag ModelSuite::singleInstance_;


// ModelSuite class member functions

ModelSuite::~ModelSuite() { /* not much to do around here... */ }

} // namespace fig
