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
#include <string>
#include <unordered_map>
#include <functional>     // std::function<>, std::reference_wrapper<>
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
///
typedef  float                                           CLOCK_INTERNAL_TYPE;

/// Fixed-size array of distribution parameters, needed to sample Distributions
typedef std::array< CLOCK_INTERNAL_TYPE , NUM_DISTRIBUTION_PARAMS >
///
													  DistributionParameters;

/// Arbitrary stochastic distribution mapping to the real line
/// @note To sample a Distribution the only argument needed is a single
///       \ref DistributionParameters "distribution parameter array",
///       regardless of the actual number of parameters the mathematical
///       entity may need.
typedef std::function< CLOCK_INTERNAL_TYPE (const DistributionParameters&) >
///
																Distribution;

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// Variables
//

/// Variable declaration: name, min, max
template< typename T_ > using                             VariableDeclaration
	= std::tuple< std::string, T_, T_ >;


/// Variable definition: name, min, max, initial value
template< typename T_ > using                              VariableDefinition
	= std::tuple< std::string, T_, T_, T_ >;

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// State and StateInstance
//

/// StateInstances internal storage type
/// @warning Must match, or be compatible with, that of MuParser library
///
typedef  MUP_BASETYPE                                    STATE_INTERNAL_TYPE;

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
///
typedef  uintmax_t                                                   Bitflag;

/// Mapping of clock and variable (names) to their respective positions
/// in the global arrays handled by the Traials. This is generally known
/// only after all modules have been added to the ModuleNetwork.
///
typedef  std::unordered_map< std::string, size_t >              PositionsMap;

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// Importance and simulation events
//

/// Primitive type used to assess the importance of a single *concrete* state
/// @warning This bounds the number of representable importance levels
///
typedef  unsigned short                                      ImportanceValue;

/// Bit flag to identify the recognized events during simulation
/// @note Same as ImportanceValue to store this info in the ImportanceFunction
///
typedef  ImportanceValue                                               Event;

/// Simulation event types
///
enum EventType
{
    NONE       = 0,
    RARE       = 1u<<(8*sizeof(Event)-1),
    STOP       = 1u<<(8*sizeof(Event)-2),
    REFERENCE  = 1u<<(8*sizeof(Event)-3),
    THR_UP     = 1u<<(8*sizeof(Event)-4),
    THR_DOWN   = 1u<<(8*sizeof(Event)-5)
};

inline Event MASK(const ImportanceValue& val) noexcept
{ return static_cast<Event>(val) & (RARE|STOP|REFERENCE|THR_UP|THR_DOWN); }

inline ImportanceValue UNMASK(const ImportanceValue& val) noexcept
{ return val & ~(static_cast<ImportanceValue>(RARE|STOP|REFERENCE|THR_UP|THR_DOWN)); }

inline bool IS_RARE_EVENT     (const Event& e) { return e & EventType::RARE;      }
inline bool IS_STOP_EVENT     (const Event& e) { return e & EventType::STOP;      }
inline bool IS_REFERENCE_EVENT(const Event& e) { return e & EventType::REFERENCE; }
inline bool IS_THR_UP_EVENT   (const Event& e) { return e & EventType::THR_UP;    }
inline bool IS_THR_DOWN_EVENT (const Event& e) { return e & EventType::THR_DOWN;  }

inline void SET_RARE_EVENT     (Event& e) { e |= EventType::RARE;      }
inline void SET_STOP_EVENT     (Event& e) { e |= EventType::STOP;      }
inline void SET_REFERENCE_EVENT(Event& e) { e |= EventType::REFERENCE; }
inline void SET_THR_UP_EVENT   (Event& e) { e |= EventType::THR_UP;    }
inline void SET_THR_DOWN_EVENT (Event& e) { e |= EventType::THR_DOWN;  }

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// Properties
//

/// Supported logical property types
///
enum PropertyType
{
	/// P( !stop U goal )
	TRANSIENT = 0,

	/// S( label / total_time )
	THROUGHPUT,

	/// S( rare_event / total_time )
	RATE,

	/// S( rare_event / reference )
	PROPORTION,  // or RATIO?

	/// P( F[<=time] goal )
	BOUNDED_REACHABILITY
};

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// Miscellanea
//

/// Allow containers with references (http://stackoverflow.com/a/23488449)
template< typename T_ > using Reference = std::reference_wrapper<T_>;

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //

} // namespace fig

#endif // CORE_TYPEDEFS_H

