//==============================================================================
//
//  Module.h
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


#ifndef MODULE_H
#define MODULE_H

// C++
#include <vector>
#include <memory>
#include <string>
#include <forward_list>
// FIG
#include <core_typedefs.h>
#include <State.h>


namespace fig
{

class Property;
class Transition;
class ImportanceFunction;
class ThresholdsBuilder;

/**
 * @brief Abstract base module class
 *
 *        The system model described by the user is implemented as a
 *        ModuleNetwork, composed of ModuleInstance objects.
 */
class Module
{
protected:

	/// All the transitions of the Module, with no particular order.
	/// @note Needed in this base class to traverse the state space,
	///       e.g. when building auto ifun
	std::vector< Transition > transitions_;

	/// Whether all clocks distributions' are memoryless,
	/// e.g. exponential, hyper-exponential, Erlang
	bool markovian_;

public:  // Dtor

	virtual ~Module() {}

public:  // Accessors

	/// The ModuleNetwork returns "GlobalModel", a ModuleInstance returns its name
	virtual std::string id() const noexcept = 0;

	/// @copydoc markovian_
	inline bool is_markovian() const noexcept { return markovian_; }

	/// Number of clocks defined in this Module
	virtual size_t num_clocks() const noexcept = 0;

	/// Number of (symbolic) transitions of this Module, i.e. the transitions
	/// defined syntactically by the user in the IOSA model description
	inline size_t num_transitions() const noexcept { return transitions_.size(); }

	/// Symbolic global state size, i.e. number of variables in the Module
	virtual size_t state_size() const noexcept = 0;

	/// Concrete global state size, i.e. cross product of the ranges
	/// of all the variables in the system model
	virtual uint128_t concrete_state_size() const noexcept = 0;

	/// Whether this Module has already been sealed for simulations
	virtual bool sealed() const noexcept = 0;

public:  // Utils

	/// Get a copy of the initial state of the system
	/// @warning Module should be sealed()
	/// \ifnot NDEBUG
	///   @throw FigException if Module hasn't been sealed() yet
	/// \endif
	virtual State<STATE_INTERNAL_TYPE> initial_state() const = 0;

	/// Initial concrete state of the system, i.e. a number between zero
	/// and concrete_state_size() enconding the initial_state()
	/// @warning Module should be sealed()
	/// \ifnot NDEBUG
	///   @throw FigException if Module hasn't been sealed() yet
	/// \endif
	virtual size_t initial_concrete_state() const = 0;

	/// Copy initial state valuation into given StateInstance
	/// @warning Module should be sealed()
	/// @throw FigException if 's' doesn't match the Module state's size
	/// \ifnot NDEBUG
	///   @throw FigException if Module hasn't been sealed() yet
	/// \endif
	virtual void instantiate_initial_state(StateInstance& s) const = 0;

	/// Get all (concrete) states that can be reached in a single step from 's'
	virtual std::forward_list<size_t> adjacent_states(const size_t& s) const = 0;
};

} // namespace fig

#endif // MODULE_H

