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
// FIG
#include <ImportanceFunction.h>
#include <Property.h>


namespace fig
{

class Transition;

/**
 * @brief Abstract base module class
 *
 *        The system model described by the user is implemented as a
 *        ModuleNetwork, composed of ModuleInstance objects.
 *
 * @note The accept member function implements the
 *       <a href="https://sourcemaking.com/design_patterns/visitor">
 *       visitor design pattern</a>, where the visitor is the
 *       ImportanceFunction and the visited elements are instances
 *       of the classes which derive from Module.
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
	inline const size_t num_transitions() const noexcept
		{ return transitions_.size(); }

	/// Symbolic global state size, i.e. number of variables in the Module
	virtual size_t state_size() const noexcept = 0;

	/// Concrete global state size, i.e. cross product of the ranges
	/// of all the variables in the system model
	virtual size_t concrete_state_size() const noexcept = 0;

	/// Whether this Module has already been sealed for simulations
	virtual bool sealed() const noexcept = 0;

public:  // Utils

	/// Have the importance of our states assessed by this ImportanceFunction,
	/// according to the given Property and strategy
	virtual void accept(ImportanceFunction& ifun,
						const Property& property,
						const std::string& startegy) = 0;
};

} // namespace fig

#endif // MODULE_H

