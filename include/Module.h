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
// FIG
#include <core_typedefs.h>


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
	/// @note Needed to traverse the state space, e.g. when building auto ifun
	std::vector< std::shared_ptr< Transition > > transitions_;

public:  // Accessors

	/// Number of clocks defined in this Module
	virtual size_t num_clocks() const noexcept = 0;

	/// Number of (symbolic) transitions of this Module, i.e. the transitions
	/// defined syntactically by the user in the IOSA model description
	inline size_t num_transitions() const noexcept { return transitions_.size(); }

	/// Return this Module's symbolic transitions
	/// @note Safety is ensured as long as Transition offers no non-const
	///       public method. Otherwise Transitions could be modified by
	///       the user and there'd be no way for us to find out.
	inline const std::vector< std::shared_ptr< Transition > >& transitions() const noexcept
		{ return transitions_; }

	/// Symbolic global state size, i.e. number of variables in the Module
	virtual size_t state_size() const noexcept = 0;

	/// Concrete global state size, i.e. cross product of the ranges
	/// of all the variables in the system model
	virtual size_t concrete_state_size() const noexcept = 0;

	/// Whether this Module has already been sealed for simulations
	virtual bool sealed() const noexcept = 0;

public:  // Utils

	/// Get a copy of the initial state of the system
	/// @warning Module should be sealed()
	/// \ifnot NDEBUG
	///   @throw FigException if Module hasn't been sealed() yet
	/// \endif
	virtual StateInstance initial_state() const = 0;

	/// Initial concrete state of the system, i.e. a number between zero
	/// and concrete_state_size() enconding the initial_state()
	/// @warning Module should be sealed()
	/// \ifnot NDEBUG
	///   @throw FigException if Module hasn't been sealed() yet
	/// \endif
	virtual size_t initial_concrete_state() const = 0;
};

} // namespace fig

#endif // MODULE_H

