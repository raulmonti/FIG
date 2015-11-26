//==============================================================================
//
//  core_typedefs.h
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


#ifndef CORE_TYPEDEFS_H
#define CORE_TYPEDEFS_H

// C++
#include <array>
#include <tuple>
#include <functional>
// External code
#include <muParserDef.h>  // MUP_BASETYPE


/**
 * @brief "fig" is the core namespace of the FIG project.
 *
 *         All classes needed for the description of the user's system model
 *         and later simulations on it, e.g. Clock, State, Transition,
 *         ModuleInstance and so on, exist within this namespace.
 */
namespace fig
{

// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// Clock stochastic distributions
//

#define  NUM_DISTRIBUTION_PARAMS  4u  // Arglist length for any distribution

/// Time resolution (choice of floating point precision)
typedef  float  CLOCK_INTERNAL_TYPE;

/// Fixed-size array of distribution parameters,
/// needed to sample any Distribution.
typedef std::array< const CLOCK_INTERNAL_TYPE , NUM_DISTRIBUTION_PARAMS >
    DistributionParameters;

/// Arbitrary stochastic distribution mapping to the real line
/// @note To sample a Distribution the only argument needed is a single
///       \ref DistributionParameters "distribution parameter array",
///       regardless of the actual number of parameters the mathematical
///       entity may need.
typedef std::function< CLOCK_INTERNAL_TYPE (const DistributionParameters&) >
    Distribution;

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// Variables
//

/// Variable declaration: name, min, max
template< typename T_ > using VariableDeclaration =
    std::tuple< std::string, T_, T_ >;


/// Variable definition: name, min, max, initial value
template< typename T_ > using VariableDefinition =
    std::tuple< std::string, T_, T_, T_ >;

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// State and StateInstance
//

/// StateInstances internal storage type
/// @warning Must match, or be compatible with, that of MuParser library
typedef  MUP_BASETYPE  STATE_INTERNAL_TYPE;

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// Transitions
//

/// Bit flag to identify resetting clocks
/// @warning This bounds the max # of clocks in the model
/// @note In <a href="http://goo.gl/hXmnBQ">Boost's multiprecision library</a>
///       there are ints of up to 1024 bits.
typedef  uintmax_t  Bitflag;

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// Importance
//

/// Type used to assess the importance of a single concrete state
/// @warning This bounds the max # of clocks in the model
typedef short ImportanceValue;

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //

} // namespace fig

#endif // CORE_TYPEDEFS_H

