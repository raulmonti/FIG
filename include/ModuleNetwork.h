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
class Property;

/**
 * @brief Network of \ref ModuleInstance "module instances" synchronized
 *        through input/output \ref Label "labels".
 *
 *        This is the user's system model.
 *        This class holds a memory-contiguous view of the \ref State "global
 *        state": a vector with <i>a copy of</i> the variables from all the
 *        constituent modules. In contrast, \ref Clock "clocks" are kept
 *        locally inside each \ref ModuleInstance "module instance".
 *
 * @todo remove_module() facility? Seems pointless and troublesome to me
 */
class ModuleNetwork : public Module
{
	friend class Traial;
	friend class ImportanceFunctionConcreteSplit;
	friend class ImportanceFunctionConcreteCoupled;

private:  // Attributes shared with our friends

	/// Unified, memory-contiguous global vector of \ref Variable "variables"
	State< STATE_INTERNAL_TYPE > gState;

	/// Global position and distribution of the \ref Clock "initial clocks"
	std::unordered_map< size_t, const Clock& > initialClocks;

	/// The modules network per se
	std::vector< std::shared_ptr< ModuleInstance > > modules;

private:

	/// Total number of clocks, considering all modules in the network
	size_t numClocks_;

	/// Whether the system model has already been sealed for simulations
	bool sealed_;

public:  // Ctors/Dtor

	/// No data ctor, only empty ctor provided
	ModuleNetwork();

	/// Copy ctor copies all internal modules, not their pointers
	ModuleNetwork(const ModuleNetwork& that);

	/// Default move ctor
	ModuleNetwork(ModuleNetwork&& that) = default;

	/// Can't copy assign since Transitions can't
	ModuleNetwork& operator=(const ModuleNetwork&) = delete;

	/// Can't move assign since Transitions can't
	ModuleNetwork& operator=(ModuleNetwork&&) = delete;

	~ModuleNetwork();

public:  // Populating facilities

	/**
	 * @brief Add a new \ref ModuleInstance "module" to the network
	 * @param module Pointer with the new module to add
	 * @note The argument should have been allocated with std::make_shared()
	 * @note The argument is reset to nullptr during call for safety reasons.
	 *       The module instance is thus effectively stolen.
	 * @warning Do not invoke after seal()
	 * \ifnot NDEBUG
	 *   @throw FigException if the network has already been sealed()
	 * \endif
	 */
	void add_module(std::shared_ptr< ModuleInstance >& module);

public:  // Accessors

	/// @copydoc numClocks_
	inline virtual size_t num_clocks() const noexcept { return numClocks_; }

	inline virtual size_t state_size() const noexcept { return gState.size(); }

	inline virtual size_t concrete_state_size() const noexcept { return gState.concrete_size(); }

	inline virtual bool sealed() const noexcept { return sealed_; }

	/// @copydoc gState
	inline const State<STATE_INTERNAL_TYPE>& global_state() const noexcept { return gState; }

public:  // Utils

	virtual StateInstance initial_state() const;

	virtual size_t initial_concrete_state() const;

	virtual void accept(ImportanceFunction& ifun,
						const Property& prop,
						const std::string& strategy) const;

	/**
	 * @brief Shut the network and fill in internal global data.
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
	 * @note This member function must be called after all \ref ModuleInstance
	 *       "module instances" have been added to the network.
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
	 *        for the current property and simulation strategy is triggered.
	 *        Information regarding the simulation run is kept in traial.
	 *
	 * @param traial   Traial instance keeping track of the simulation <b>(modified)</b>
	 * @param property Property whose value is currently being estimated
	 * @param engine   Semantics of the current simulation strategy,
	 *                 viz. object of any class derived from SimulationEngine
	 * @param watch_events Member function of 'engine' telling when
	 *                     is a simulation step considered finished
	 *
	 * @return Events observed/marked by the 'watch_event' member function
	 *         when a finishing event for this simulation step is triggered.
	 *
	 * @note Unrelated to simulation(), this routine was deviced for
	 *       estimating the value of a Property.
	 *
	 * @warning seal() must have been called beforehand
	 * \ifnot NDEBUG
	 *   @throw FigException if seal() hasn't been called yet
	 * \endif
	 */
	template< typename DerivedProperty,
			  class Simulator,
			  class TraialMonitor >
	Event simulation_step(Traial& traial,
						  const DerivedProperty& property,
						  const Simulator& engine,
						  TraialMonitor watch_events) const;

	/**
	 * @brief Advance a traial and keep track of maximum importance reached
	 *
	 *        Starting from the state stored in traial, this routine
	 *        performs synchronized jumps in the \ref ModuleInstance
	 *        "modules composing the system" as long as the given
	 *        Predicate remains true.<br>
	 *        At function exit the traial internals are left at the peak:
	 *        its importance is the maximum achieved and its state is
	 *        the variables valuation realizing that importance.
	 *
	 * @param traial Traial instance keeping track of the simulation <b>(modified)</b>
	 * @param update Traial update function applied on each jump iteration
	 * @param pred   Predicate telling when to stop jumping
	 *
	 * @return Maximum importance achieved during simulation
	 *
	 * @note Unrelated to simulation_step(), this routine was deviced for
	 *       \ref ThresholdsBuilder "thresholds builders" which require
	 *       exercising the ModuleNetwork dynamics.
	 *
	 * @warning seal() must have been called beforehand
	 * \ifnot NDEBUG
	 *   @throw FigException if seal() hasn't been called yet
	 * \endif
	 */
	template< class Predicate, class Update >
	ImportanceValue peak_simulation(Traial& traial,
									Update update,
									Predicate pred) const;
}; // class ModuleNetwork




/// @note Defined here to allow lambda functions with captures as template parameters
template< class Predicate, class Update >
ImportanceValue
ModuleNetwork::peak_simulation(Traial& traial,
							   Update update,
							   Predicate pred) const
{
	if (!sealed())
#ifndef NDEBUG
		throw_FigException("ModuleNetwork hasn't been sealed yet");
#else
		return;
#endif

	ImportanceValue maxImportance(traial.importance);
	StateInstance maxImportanceState(traial.state);

	while ( pred(traial) ) {
		auto timeout = traial.next_timeout();
		// Active jump in the module whose clock timed-out
		auto label = timeout.module->jump(timeout.name, timeout.value, traial);
		// Passive jumps in the modules listening to label
		for (auto module_ptr: modules)
			if (module_ptr->name != timeout.module->name)
				module_ptr->jump(label, timeout.value, traial);
		// Update traial internals
		traial.lifeTime += timeout.value;
		update(traial);
		if (UNMASK(traial.importance) > UNMASK(maxImportance)) {
			maxImportance = traial.importance;
			maxImportanceState = traial.state;
		}
	}
	traial.importance = maxImportance;
	traial.state = maxImportanceState;

	return UNMASK(maxImportance);
}

/// Update traial function specialization for "template<...> ModuleNetwork::peak_simulation()"
typedef void(*UpdateFun)(Traial&);

/// Predicate specialization for "template<...> ModuleNetwork::peak_simulation()"
typedef bool(*KeepRunning)(const Traial&);

template<> ImportanceValue ModuleNetwork::peak_simulation(Traial&,
														  UpdateFun,
														  KeepRunning) const;

} // namespace fig

#endif // MODULENETWORK_H

