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
#include <mutex>  // std::call_once(), std::once_flag
#include <vector>
#include <memory>     // std::unique_ptr<>
#include <functional>
#include <unordered_map>
// FIG
#include <core_typedefs.h>
#include <Clock.h>
#include <Variable.h>
#include <State.h>
#include <Module.h>
#include <ModuleInstance.h>
#include <TraialPool.h>


namespace fig
{

class SimulationEngine;

/**
 * @brief Network of \ref ModuleInstance "module instances" synchronized
 *        through input/output \ref Label "labels".
 *
 *        This is the user's system model.
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
 *        singleton design pattern</a>. It was implemented using C++11
 *        facilities to make it
 *        <a href="http://silviuardelean.ro/2012/06/05/few-singleton-approaches/">
 *        thread safe</a>.
 *
 * @todo remove_module() facility? Seems pointless and troublesome to me
 */
class ModuleNetwork : public Module
{
	friend class Traial;
	friend class ImportanceFunctionConcreteSplit;
	friend class ImportanceFunctionConcreteCoupled;

protected:  // Attributes shared with the ImportanceFunction visitors

	/// Unified, memory-contiguous global vector of \ref Variable "variables"
	static State< STATE_INTERNAL_TYPE > gState;

	/// Global position and distribution of the \ref Clock "initial clocks"
	static std::unordered_map< size_t, const Clock& > initialClocks;

	/// The modules network per se
	static std::vector< std::shared_ptr< ModuleInstance > > modules;

private:

	/// Total number of clocks, considering all modules in the network
	static size_t numClocks_;

	/// Global position of the last clock from the last added module,
	/// useful only during network construction phase, i.e. before seal()
	static size_t lastClockIndex_;

	/// Whether the system model has already been sealed for simulations
	static bool sealed_;

	/// Single existent instance of the class (singleton design pattern)
	static std::unique_ptr< ModuleNetwork > instance_;

	/// Single instance thread safety
	static std::once_flag singleInstance_;

	/// Private ctors (singleton design pattern)
	ModuleNetwork() {}
	ModuleNetwork(ModuleNetwork&& that)                 = delete;
	ModuleNetwork& operator=(const ModuleNetwork& that) = delete;

public:  // Access to the ModuleNetwork instance

	/// Global access point to the unique instance of this pool
	static inline ModuleNetwork& get_instance()
		{
			std::call_once(singleInstance_,
						   [] () { instance_.reset(new ModuleNetwork); });
			return *instance_;
		}

	/// Allow syntax "auto net = fig::ModuleNetwork::get_instance();"
	inline ModuleNetwork(const ModuleNetwork& that) {}
		// { instance_.swap(that.instance_); }

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

	/// @copydoc sealed_
	inline bool sealed() const noexcept { return sealed_; }

	/// @copydoc numClocks_
	inline size_t num_clocks() const noexcept { return numClocks_; }

	/// Symbolic global state size, i.e. number of variables in the system model
	inline size_t state_size() const noexcept { return gState.size(); }

	/// Concrete global state size, i.e. cross product of the ranges
	/// of all the variables in the system model
	inline size_t concrete_state_size() const noexcept { return gState.concrete_size(); }

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
	 *        which could be needed for some transitions precondition.
	 *
	 * @param initialClocksNames Container with the names of the clocks which
	 *                           need to be reset on system initialization
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
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	void seal(const Container<ValueType, OtherContainerArgs...>& initialClocksNames);

	/**
	 * @brief Advance a traial until some stopping criterion is met
	 *
	 *        Starting from the state stored in traial, this routine
	 *        performs synchronized jumps in the \ref ModuleInstance
	 *        "modules composing the system", until some event relevant
	 *        for the current simulation strategy is triggered.
	 *        Information regarding the simulation run is kept in traial.
	 *
	 * @param traial Traial instance keeping track of the simulation
	 * @param engine Semantics of the current simulation strategy
	 */
	void simulation_step(Traial& traial, SimulationEngine* const engine) const;
};

} // namespace fig

#endif // MODULENETWORK_H

