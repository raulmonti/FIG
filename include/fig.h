//==============================================================================
//
//
//  Main header for the FIG project.
//
//
//     Include this file to have the full functionality.
//     FIG defines the "fig" namespace and compiles in debug mode by default.
//     You must compile with the following flags to get the release version:
//
//       NDEBUG
//       NRANGECHK
//
//     Also by default FIG works with C's standard RNG, i.e. a medium quality
//     linear-congruential generator defined as std::minstd_rand.
//     To get the clock times sampled from a high quality Mersenne-Twister
//     with 64-bytes internal state compile with the following flag:
//
//       HQ_RNG
//
//     To reduce the size of your build consider also including only the
//     specific FIG headers your project requires. It is however highly likely
//     that most of the code will be implicitly included anyways,
//     due to FIG's highly coupled design. A little price to pay for
//     (attempted) efficiency :p
//
//
//  Copyleft 2016-
//  Authors:
//  - Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
//  - Raul E. Monti <raulmonti88@gmail.com> (Universidad Nacional de Córdoba)
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


#ifndef FIG_H
#define FIG_H

// Base
#include "core_typedefs.h"
#include "exceptions.h"
#include "FigException.h"
// Parser
#include "ast.h"
#include "config.h"
#include "parser.h"
#include "smtsolver.h"
#include "iosacompliance.h"
// Basic ADTs
#include "Clock.h"
#include "Label.h"
#include "ILabel.h"
#include "OLabel.h"
#include "Variable.h"
#include "VariableSet.h"
#include "VariableInterval.h"
#include "State.h"
#include "MathExpression.h"
#include "Precondition.h"
#include "Postcondition.h"
#include "Property.h"
#include "PropertyTransient.h"
#include "Traial.h"
// Importance functions
#include "ImportanceFunction.h"
#include "ImportanceFunctionConcrete.h"
#include "ImportanceFunctionConcreteSplit.h"
#include "ImportanceFunctionConcreteCoupled.h"
// Simulation engines
#include "SimulationEngine.h"
#include "SimulationEngineNosplit.h"
// Composite ADTs
#include "Transition.h"
#include "ModuleInstance.h"
#include "ModuleNetwork.h"
#include "ModelSuite.h"
#include "TraialPool.h"

#endif // FIG_H

