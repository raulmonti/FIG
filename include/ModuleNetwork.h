//==============================================================================
//
//  ModuleNetwork.h
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


#ifndef MODULENETWORK_H
#define MODULENETWORK_H

// C++
#include <vector>
#include <memory>
#include <functional>
// FIG
#include <core_typedefs.h>
#include <Module.h>
#include <ModuleInstance.h>
#include <State.h>
#include <Variable.h>
#include <Clock.h>


namespace fig
{


/**
 * @brief Network of \ref ModuleInstance "module instances" synchronized
 *        through input/output \ref Label "labels".
 *
 *        This class holds a memory-contiguous view of the global \ref State
 *        "state": a vector with <i>a copy of</i> the variables from all the
 *        constituent modules. In contrast, \ref Clock "clocks" are kept
 *        locally inside each \ref ModuleInstance "module instance".
 *
 * @note  There should be exactly one ModuleNetwork at all times,
 *        which starts out empty and is sequentially filled with
 *        ModuleInstance objects as these are created. For that reason
 *        this class follows the
 *        <a href="https://sourcemaking.com/design_patterns/singleton">
 *        singleton design pattern</a>.
 *
 * @todo remove_module() facility? Seems pointless and troublesome to me
 */
class ModuleNetwork : public Module
{
	friend class Traial;
	friend class ImportanceFunctionConcreteSplit;
	friend class ImportanceFunctionConcreteCoupled;

	/// Single existent instance of the class (singleton design pattern)
	static std::unique_ptr< ModuleNetwork > instance_;

	/// Private ctor (singleton design pattern)
	ModuleNetwork() : gState(), lastClockIndex_(0u), sealed(false) {}

	/// Proclaim to the four winds the uniqueness of the single instance
	ModuleNetwork(const ModuleNetwork& that)            = delete;
	ModuleNetwork(ModuleNetwork&& that)                 = delete;
	ModuleNetwork& operator=(const ModuleNetwork& that) = delete;

protected:  // Attributes shared with the ImportanceFunction visitors

	/// Unified, memory-contiguous global vector of \ref Variable "variables"
	State< STATE_INTERNAL_TYPE > gState;

	/// The modules network per se
	std::vector< std::shared_ptr< ModuleInstance > > modules;

private:

	/// Global position of the last clock from the last added module
	size_t lastClockIndex_;

	/// Whether the system module has already been sealed for simulations
	bool sealed;

public:  // Access to ModuleNetwork

	/// Global access point to the unique instance of this pool
	static ModuleNetwork& get_instance() {
		if (nullptr == instance_)
			instance_ = std::unique_ptr< ModuleNetwork >(new ModuleNetwork);
		return *instance_;
	}

	~ModuleNetwork();

public:  // Populating facilities

	/**
	 * @brief Add a new \ref ModuleInstance "module" to the network
	 * @param module Pointer with the new module to add
	 * @note The argument is reset to nullptr during call for safety reasons.
	 *       The module instance is thus effectively stolen.
	 * @warning Do not invoke after seal()
	 * @deprecated Use the \ref add_module(std::shared_ptr<ModuleInstance>&)
	 *             "shared_ptr version" instead
	 */
	void add_module(ModuleInstance** module);

	/**
	 * @brief Add a new \ref ModuleInstance "module" to the network
	 * @param module Pointer with the new module to add
	 * @note The argument should have been allocated with std::make_shared()
	 * @note The argument is reset to nullptr during call for safety reasons.
	 *       The module instance is thus effectively stolen.
	 * @warning Do not invoke after seal()
	 */
	void add_module(std::shared_ptr< ModuleInstance >& module);

public:  // Utils

	virtual inline void accept(ImportanceFunction& ifun)
		{ ifun.assess_importance(this); }

	/**
	 * @brief Shut the system model to begin with simulations
	 *
	 *        Information, about the global position of the \ref Clock "clocks"
	 *        and \ref Variable "variables" which belong to each individual
	 *        \ref ModuleInstance "module", has to be built and broadcasted
	 *        along the network. This is for instance required to allow
	 *        modules reading the state of some other module's variables,
	 *        which could be needed for some transition preconditions.
	 *
	 * @note This member function should be called by the user exactly once,
	 *       after all \ref ModuleInstance "module instances" have been added
	 *       to the network and right before the beginning of simulations.
	 *
	 * @warning No more modules can be added with add_module() after this invocation
	 * \ifnot NDEBUG
	 *   @throw FigException if called more than once
	 * \endif
	 */
	void seal();

protected:  // Utilities offered to Traials

	/**
	 * @brief Facility for Traial::Timeout construction
	 * @param pos Global position of the clock, as in any Traial's internal vector
	 * @return Pointer to the ModuleInstance which owns the clock located
	 *         at the (global) position 'pos', or nullptr on error
	 * \ifnot NRANGECHK
	 *   @throw std::out_of_range if 'pos' goes beyond scope
	 * \endif
	 */
	inline std::shared_ptr< ModuleInstance > module_of_clock_at(const size_t& pos)
		{
			/// @todo TODO implement!
			return nullptr;
		}
};

} // namespace fig

#endif // MODULENETWORK_H

