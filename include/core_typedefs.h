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
#include <tuple>
#include <array>
#include <vector>
#include <forward_list>
#include <unordered_map>
#include <string>
#include <bitset>
#include <functional>  // std::reference_wrapper<>
// External code
#include <uint128_t.h>


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

/// Time resolution (floating point precision choice)
///
#ifndef DOUBLE_TIME_PRECISION
  typedef  float                                         CLOCK_INTERNAL_TYPE;
#else
  typedef  double                                        CLOCK_INTERNAL_TYPE;
#endif

/// Fixed-size array of distribution parameters, needed to sample Distributions
typedef std::array< CLOCK_INTERNAL_TYPE , NUM_DISTRIBUTION_PARAMS >
///
													  DistributionParameters;

/// Arbitrary stochastic distribution mapping to the real line
/// @note To sample a Distribution the only argument needed is a single
///       \ref DistributionParameters "distribution parameter array",
///       regardless of the actual number of parameters the mathematical
///       entity may need.
typedef CLOCK_INTERNAL_TYPE(*Distribution)(const DistributionParameters&);

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
// State
//

/// StateInstances internal storage type
typedef short STATE_INTERNAL_TYPE;

/// Assignment of values to Variables (a logical <i>valuation</i>)
/// following the order given in some State. A StateInstance can be
/// compared to the State it comes from to check consistency.
typedef  std::vector< STATE_INTERNAL_TYPE >                    StateInstance;

/// Adjacency list for concrete states transitions graph
typedef  std::vector< std::forward_list< unsigned > >          AdjacencyList;

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// Transitions
//

#define  MAX_NUM_CLOCKS  (1ul<<10ul)  // The model can have up to 1024 clocks

/// Bit flag to identify resetting clocks
/// @note This bounds the max number of clocks the user can define in his model.
///       To extend this limit simply redefine "MAX_NUM_CLOCKS" above
///
typedef  std::bitset<MAX_NUM_CLOCKS>                                 Bitflag;

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// Importance functions
//

/// Primitive type used to assess the importance of a single *concrete* state
/// @warning This bounds the number of representable importance levels
///
typedef  size_t                                              ImportanceValue;

/// Vector of \ref ImportanceValue "importance values"
typedef  std::vector< ImportanceValue >                        ImportanceVec;

/// Post-process applied to the \ref ImportanceValue "importance values" assessed
struct PostProcessing
{
	/// Type of post-processing
	enum {
		/// Don't modify importance values
		NONE = 0,
		/// Increase/decrease importance values by a constant
		SHIFT,
		/// Exponentiate importance values
		EXP,
		/// Invalid post-processing
		INVALID
	} type;

	/// Human-readable name associated to the type
	std::string name;

	/// Post-processing argument value (e.g. base of the exponentiation for EXP)
	float value;

	PostProcessing() : type(NONE), name(""), value(0.0f) {}
	PostProcessing(decltype(type) theType, const std::string& theName, float theValue) :
		type(theType), name(theName), value(theValue) {}
};

/// ImportanceFunction specification: this struct should be filled
/// during the command line parsing, with the data provided by the user
struct ImpFunSpec
{
	/// ImportanceFunction name @see ImportanceFunction::names()
	const std::string name;
	/// ImportanceFunction assessment strategy @see ImportanceFunction::strategies()
	const std::string strategy;
	/// User-defined ad hoc expression needed by some ImportanceFunction s
	const std::string algebraicFormula;
	/// <i>Optional</i>: post-processing to perform to the importance values computed
	const PostProcessing postProcessing;
	/// <i>Optional</i>: min value the user-defined ad hoc function can take
	const ImportanceValue minValue;
	/// <i>Optional</i>: max value the user-defined ad hoc function can take
	const ImportanceValue maxValue;
	/// <i>Optional</i>: neutral element fot the user-defined composition function
	const ImportanceValue neutralElement;
	/// Data ctor needs at least a name and a strategy
	ImpFunSpec(const std::string& theName,
			   const std::string& theStrategy,
			   const std::string& theAlgebraicFormula = "",
			   const PostProcessing& thePostProcessing = PostProcessing(),
			   const ImportanceValue& theMinValue = static_cast<ImportanceValue>(0u),
			   const ImportanceValue& theMaxValue = static_cast<ImportanceValue>(0u),
			   const ImportanceValue& theNeutralElement = static_cast<ImportanceValue>(0u)) :
		name(theName),
		strategy(theStrategy),
		algebraicFormula(theAlgebraicFormula),
		postProcessing(thePostProcessing),
		minValue(theMinValue),
		maxValue(theMaxValue),
		neutralElement(theNeutralElement) {}
};

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// Simulation events
//

/// Bit flag to identify the recognized events during simulation
/// @note Same as ImportanceValue to store this info in the ImportanceFunction
///
typedef  ImportanceValue                                               Event;

/// Simulation event types
///
enum EventType : Event
{
	NONE       = 0,

	/// Property's target, e.g. "goal" event for transient simulations
	RARE       = 1ul<<(8*sizeof(Event)-1),

	/// Simulation finished, e.g. "stop" event for transient simulations
	STOP       = 1ul<<(8*sizeof(Event)-2),

	/// Time elapsed, e.g. "reference" event for proportion simulations
	REFERENCE  = 1ul<<(8*sizeof(Event)-3),

	/// When a Traial jumps to a higher threshold level
	THR_UP     = 1ul<<(8*sizeof(Event)-4),

	/// When a Traial jumps to a lower threshold level
	THR_DOWN   = 1ul<<(8*sizeof(Event)-5)
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
inline bool IS_SOME_EVENT (const Event& e) {
	return e & (EventType::RARE      |
	            EventType::STOP      |
	            EventType::REFERENCE |
	            EventType::THR_UP    |
	            EventType::THR_DOWN);
}

inline void SET_RARE_EVENT     (Event& e) { e |= EventType::RARE;      }
inline void SET_STOP_EVENT     (Event& e) { e |= EventType::STOP;      }
inline void SET_REFERENCE_EVENT(Event& e) { e |= EventType::REFERENCE; }
inline void SET_THR_UP_EVENT   (Event& e) { e |= EventType::THR_UP;    }
inline void SET_THR_DOWN_EVENT (Event& e) { e |= EventType::THR_DOWN;  }
inline void SET_ALL_EVENTS (Event& e) {
	e |= EventType::RARE
	    | EventType::STOP
	    | EventType::REFERENCE
	    | EventType::THR_UP
	    | EventType::THR_DOWN;
}

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
	/// P( expr1 U expr2 )
	TRANSIENT = 0,

	/// S( label / total_time )
	THROUGHPUT,

	/// S( expr / total_time )
	RATE,

	/// S( expr1 / expr2 )
	RATIO,

	/// P( F[<=time] goal )
	BOUNDED_REACHABILITY
};

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //
//
// Miscellanea
//

/// For <a href="http://stackoverflow.com/a/23488449">containers of references</a>
template< typename T_ > using Reference = std::reference_wrapper<T_>;

/// Mapping of names (of clocks or variables or whatever) to their
/// respective positions in some global or local array
typedef  std::unordered_map< std::string, size_t >  PositionsMap;

/// 128-bit integer for concrete state size representation (they're that big)
/// @note Might use boost multiprecision library which defines integers
///       of up to 1024 bits, but the standalone headers weight 13 MB!
typedef uint128::uint128_t uint128_t;

/// When FIG has to interact with the <a href="http://jani-spec.org/">JANI
/// Specification format</a>, this struct defines the tasks to carry out.
/// e.g. parse a model file described in the JANI-model format,
///      or translate a IOSA-syntax model into a JANI-model format
struct JaniTranny
{
	/// Whether there's some interaction with JANI at all
	bool janiInteraction;
	/// Only translate from/to JANI to/from IOSA, viz. don't estimate
	bool translateOnly;
	/// In which direction is the translation
	enum {
		UNDEFINED = 0,
		FROM_JANI = 1,
		TO_JANI   = 2
	} translateDirection;
	/// IOSA model file name
	std::string modelFileIOSA;
	/// IOSA properties file name, if any
	std::string propsFileIOSA;
	/// JANI-spec model file name
	std::string modelFileJANI;
	/// Empty ctor
	JaniTranny() : janiInteraction(false),
				   translateOnly(false),
				   translateDirection(UNDEFINED),
				   modelFileIOSA(""),
				   propsFileIOSA(""),
				   modelFileJANI("") {}
};

//
//
// // // // // // // // // // // // // // // // // // // // // // // // // //

} // namespace fig

#endif // CORE_TYPEDEFS_H

