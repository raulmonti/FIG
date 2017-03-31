//==============================================================================
//
//  fig_cli.h
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

#ifndef FIG_CLI_H
#define FIG_CLI_H

// C++
#include <set>
#include <list>
#include <string>
#include <chrono>
// FIG
#include <core_typedefs.h>
#include <StoppingConditions.h>


/// This namespace contains the routines and objects used by the FIG tool
/// to parse the parameters fed through its Command Line Interface
namespace fig_cli
{

/**
 * @brief CLI arguments parsing routine.
 *
 *        The objects defined in this namespace are left in a valid state
 *        after a successfull call to this routine.<br>
 *        Any inconsistency encountered during parsing is reported to stderr
 *        (std::cerr) along with the full usage message. An exception
 *        is thrown upon finding any unexpected inconsistency.
 *
 * @param argc C++ main's argument count
 * @param argv C++ main's argument vector
 * @param fatalError Whether to exit(EXIT_FAILURE) if an error is found
 *
 * @return Whether the parsing was successfull. Notice this can only be false
 *         if 'fatalError' is explicitly set to false as well.
 *
 * @throw FigException if command line parsing fails unexpectedly
 */
bool
parse_arguments(const int& argc, const char** argv, bool fatalError = true);

/// Work with <a href="http://jani-spec.org/">JANI-specified</a> files
extern fig::JaniTranny janiSpec;

/// Abosulte path to the user's SA model file
extern std::string modelFile;

/// Abosulte path to the user's properties file
extern std::string propertiesFile;

/// Name of the FIG's \ref fig::SimulationEngine "simulation engine"
/// to use for estimations
extern std::string engineName;

/// Details of the FIG's \ref fig::ImportanceFunction "importance function"
/// to use for estimations
extern fig::ImpFunSpec impFunSpec;

/// Name of the FIG's \ref fig::ThresholdsBuilder "thresholds building
/// technique" to use for estimations
extern std::string thrTechnique;

/// Splitting values to test during estimations,
/// valid only for RESTART-like \ref fig::SimulationEngine "simulation engines"
extern std::set< unsigned > splittings;

/// Estimation bounds specified by the user. These can be either temporal
/// (e.g. run for 30 m) or value-driven (e.g. stop after building a 90%
/// confidence interval with 2.3x10^-5 precision)
extern std::list< fig::StoppingConditions > estBounds;

/// Simulations timeout. It causes a soft-interrupt in any simulation launched,
/// after 'simsTimeout' seconds of uninterrupted wall-clock execution
extern std::chrono::seconds simsTimeout;

/// CLocks' RNG
extern std::string rngType;

/// Seed for the RNG
extern size_t rngSeed;

/// Attempt to estimate/translate in spite of any warning from the parser
/// of the model not being IOSA-compliant
extern bool forceOperation;

/// Run algorithm to check confluence of committed actions
extern bool confluenceCheck;

}

#endif // FIG_CLI_H
