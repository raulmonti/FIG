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
 */
class ModuleNetwork : public Module
{
	friend class Traial;
	friend class ImportanceFunctionConcreteSplit;
	friend class ImportanceFunctionConcreteCoupled;

	/// Single existent instance of the class (singleton design pattern)
	static std::unique_ptr< ModuleNetwork > instance_;

	/// Private ctor (singleton design pattern)
	/// @todo Implement!
	ModuleNetwork();

	/// Proclaim to the four winds the uniqueness of the single instance
	ModuleNetwork(const ModuleNetwork& that)            = delete;
	ModuleNetwork(ModuleNetwork&& that)                 = delete;
	ModuleNetwork& operator=(const ModuleNetwork& that) = delete;

protected:  // Attributes shared with the ImportanceFunction visitors

	/// Unified, memory-contiguous global vector of \ref Variable "variables"
	State< STATE_INTERNAL_TYPE > gState;

	/// The modules network per se
	std::vector< std::shared_ptr< ModuleInstance > > modules;

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
	 *
	 * @todo TODO implement! Remember to invoke mark_added() in the module
	 */
	void add_module(ModuleInstance** module);

	/// @copydoc add_module()
	/// @note The argument should have been allocated with std::make_shared()
	void add_module(std::shared_ptr< ModuleInstance > module);

	/// @todo remove_module() facility? Seems pointless and troublesome to me

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

public:

	virtual inline void accept(ImportanceFunction& ifun)
		{ ifun.assess_importance(this); }
};

} // namespace fig

#endif // MODULENETWORK_H

