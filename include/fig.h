//==============================================================================
//
//  fig.h
//
//  Copyleft 2016-
//  Authors:
//  - Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
//  - Raul E. Monti <raulmonti88@gmail.com> (Universidad Nacional de Córdoba)
//
//------------------------------------------------------------------------------
//
//
//     Main header of the FIG project.
//
//
//       Include this file to have complete functionality. In order to reduce
//     the size of your build consider also including only the specific FIG
//     headers your project requires. It is however highly likely that most
//     of the code will be implicitly included anyways, due to FIG's highly
//     coupled design. A little price to pay for (attempted) efficiency :p
//
//       FIG defines the "fig" and "fig_cli" namespaces and compiles in debug
//     mode by default. You must define the following preprocessor macros to
//     compile the release version:
//
//           NDEBUG
//           NRANGECHK
//
//       FIG uses a pseudo-random number generator (RNG) to sample the clock
//     values each time these are reset. By default C++ STL's Mersenne-Twister
//     with 64-bytes internal state is used, because of its very long period
//     and good time performance. The user may also opt for an alternative,
//     also high-quality RNG from the PCG family (http://www.pcg-random.org/)
//     by defining the preprocessor macro:
//
//           PCG_RNG
//
//       The floating point precision of the time tracking is single by default,
//     i.e. 4-bytes size fp numbers (C++ 'float') are used to store the clocks
//     timeouts and the simulations lives lengths. If double fp precision is
//     desired (C++ 'double') the user must define the preprocessor macro:
//
//           DOUBLE_TIME_PRECISION
//
//       Any bugs, issues, comments or life-threats can (and ought to!) be sent
//     to the above specified e-mail addresses of the authors. This is by all
//     means "work in progress" so expect many updates in the near future and
//     remember: we do give a FIG!
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
#include "config.h"
#include "core_typedefs.h"
#include "string_utils.h"
#include "Util.h"
#include "Exceptions.h"
#include "FigException.h"
#include "FigConfig.h"
#include "FigLog.h"
#include "ErrorMessage.h"
// #include "SignalSetter.h"  // leave out to avoid double include
// CLI
#include "fig_cli.h"
#include "MultiDoubleArg.h"
#include "NumericConstraint.h"
#include "TimeConstraint.h"
#include "FigVersionVisitor.h"
// Basic ADTs
#include "ArrayFunctions.h"
#include "Clock.h"
#include "Label.h"
#include "Variable.h"
#include "VariableSet.h"
#include "VariableInterval.h"
#include "State.h"
#include "MathExpression.h"
#include "Precondition.h"
#include "Postcondition.h"
#include "Traial.h"
#include "StoppingConditions.h"
// Property queries
#include "Property.h"
#include "PropertyRate.h"
#include "PropertyTransient.h"
#include "PropertyProjection.h"
// Confidence intervals
#include "ConfidenceInterval.h"
#include "ConfidenceIntervalResult.h"
#include "ConfidenceIntervalMean.h"
#include "ConfidenceIntervalProportion.h"
#include "ConfidenceIntervalRate.h"
#include "ConfidenceIntervalTransient.h"
#include "ConfidenceIntervalWilson.h"
// Importance functions
#include "ImportanceFunction.h"
#include "ImportanceFunctionAlgebraic.h"
#include "ImportanceFunctionConcrete.h"
#include "ImportanceFunctionConcreteSplit.h"
#include "ImportanceFunctionConcreteCoupled.h"
// Thresholds builders
#include "ThresholdsBuilder.h"
#include "ThresholdsBuilderFixed.h"
#include "ThresholdsBuilderAdaptive.h"
#include "ThresholdsBuilderAdaptiveSimple.h"
#include "ThresholdsBuilderAMS.h"
#include "ThresholdsBuilderSMC.h"
#include "ThresholdsBuilderES.h"
#include "ThresholdsBuilderHybrid.h"
// Simulation engines
#include "SimulationEngine.h"
#include "SimulationEngineNosplit.h"
#include "SimulationEngineRestart.h"
#include "SimulationEngineFixedEffort.h"
// High level ADTs
#include "Transition.h"
#include "ModuleInstance.h"
#include "ModuleNetwork.h"
#include "ModelSuite.h"
#include "TraialPool.h"
// Lexer & Parser
#include "Type.h"
#include "Graph.h"
#include "Operators.h"
#include "ExprDNFBuilder.h"
#include "ExpEvaluator.h"
#include "ExpReductor.h"
#include "ExpState.h"
#include "ExpStateUpdater.h"
#include "ExpStateEvaluator.h"
#include "ModuleScope.h"
#include "ModelTC.h"
#include "ModelAST.h"
#include "ModelParser.hpp"
#include "ModelBuilder.h"
#include "ModelReductor.h"
#include "ModelVerifier.h"
#include "ModelPrinter.h"
// Language validation
#include "ConfluenceChecker.h"
#include "DNFChecker.h"
#include "IOSAState.h"
#include "IOSAModule.h"
#include "IOSAExpEval.h"
// Translators to other languages
#include "JANI_translator.h"

#endif // FIG_H

