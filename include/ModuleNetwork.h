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
#include <FigLog.h>


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
	friend class ImportanceFunctionConcreteSplit;  // grant access to the modules

private:  // Attributes shared with our friends

	/// Unified, memory-contiguous global vector of \ref Variable "variables"
	State< STATE_INTERNAL_TYPE > gState;

	/// Global position and distribution of the \ref Clock "initial clocks"
	std::unordered_map< size_t, const Clock& > initialClocks;

	/// The modules network per se
	std::vector< std::shared_ptr< ModuleInstance > > modules;

    /// Whether or not this module network has committed actions
    bool has_committed_ = false;

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

	~ModuleNetwork() override;

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
	 * @throw FigException if the model's clocks quota is exceeded
	 */
	void add_module(std::shared_ptr< ModuleInstance >& module);

public:  // Accessors

	inline std::string id() const noexcept override { return "GlobalModel"; }

	/// How many \ref ModuleInstance "modules" have been added to the network
	inline size_t num_modules() const noexcept { return modules.size(); }

	/// @copydoc numClocks_
	inline size_t num_clocks() const noexcept override { return numClocks_; }

	inline size_t state_size() const noexcept override { return gState.size(); }

	inline uint128_t concrete_state_size() const noexcept override { return gState.concrete_size(); }

	inline bool sealed() const noexcept override { return sealed_; }

	/// @copydoc gState
	inline const State<STATE_INTERNAL_TYPE>& global_state() const noexcept { return gState; }

	/// Get vector of const references to all the system clocks
	std::vector< Reference< const Clock > > clocks() const;

public:  // Utils

	State<STATE_INTERNAL_TYPE> initial_state() const override;

	size_t initial_concrete_state() const override;

	void instantiate_initial_state(StateInstance& s) const override;

	/**
	 * @copydoc Module::adjacent_states()
	 * @param s Concrete state from the system's global state space
	 * @note <b>Complexity:</b> <i>O((m*t*v)<sup>2</sup>)</i>, where
	 *       <ul>
	 *       <li> <i>m</i> is the number  of   modules   of the system,</li>
	 *       <li> <i>t</i> is the max num of transitions of any module,</li>
	 *       <li> <i>v</i> is the max num of  variables  of any module.</li>
	 *       </ul>
	 */
	std::forward_list<size_t> adjacent_states(const size_t& s) const override;

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
	 * @param watch_events Function telling when does a simulation step finish
	 *
	 * @return Events observed/marked by the 'watch_event' member function
	 *         when a finishing event for this simulation step is triggered.
	 *
	 * @note Unrelated to simulation(), this routine was deviced for
	 *       estimating the value of a Property.
	 *
	 * @warning seal() must have been called beforehand
	 */
	template< typename DerivedProperty,
	          class TraialMonitor >
	Event simulation_step(Traial& traial,
	                      const DerivedProperty& property,
						  const TraialMonitor& watch_events) const;

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
	 */
	template< class Predicate, class Update >
	ImportanceValue peak_simulation(Traial& traial,
									Update update,
									Predicate pred) const;

private:  // Committed actions processing

    /**
     * @brief Find (if any) an enabled output-committed
     * transition and broadcast it to all the modules.
     * @return true if such a transition exists, false otherwise.
     * @note This will choose *the first* enabled transition.
     * Assuming the model is confluent this choice is safe.
     * @param Traial that receives the changes in the state.
     */
	bool process_committed_once(Traial &traial) const;

    /**
     * @brief Process all the committed actions repeatedly until
     * no committed transition is enabled.
     * @note The execution of the postcondition of a committed transition could
     * enable another committed transition that should be executed inmmediately,
     * and that is why this method is necessary.
     * @param Traial that receives the changes in the state.
     */
	void process_committed(Traial &Traial) const;

public:  // Debug

	void print_info(std::ostream &out) const;

}; // class ModuleNetwork




/// @note Defined here to allow lambda functions with captures as template parameters
template< class Predicate, class Update >
ImportanceValue
ModuleNetwork::peak_simulation(Traial& traial,
							   Update update,
							   Predicate pred) const
{
	assert(sealed());
	ImportanceValue maxImportance(UNMASK(traial.level));
	StateInstance maxImportanceState(traial.state);
	std::vector<Traial::Timeout> maxImportanceClocks(TraialPool::get_timeouts(traial));

	try {
		// Start up processing the initial commited actions
		process_committed(traial);
		// (that could've reset clocks and changed next timeout)
		while ( pred(traial) ) {
			// Process timed actions
			const Traial::Timeout& to = traial.next_timeout();
			const float elapsedTime(to.value);
			assert(0.0f <= elapsedTime);
			// Active jump in the module whose clock timed-out:
			const Label& label = to.module->jump(to, traial);
			// Passive jumps in the modules listening to label:
			for (auto module_ptr: modules)
				if (module_ptr->name != to.module->name)
					module_ptr->jump(label, elapsedTime, traial);
			// Update traial internals
			traial.lifeTime += elapsedTime;
			update(traial);
			if (UNMASK(traial.level) > maxImportance) {
				maxImportance = UNMASK(traial.level);
				maxImportanceState = traial.state;
				maxImportanceClocks = TraialPool::get_timeouts(traial);
			}
			// Process any newly activated committed action
			process_committed(traial);
		}
	} catch (const FigException& e) {
		// Ignore: this is one of many, hopefully it won't matter
#ifndef NDEBUG
		figTechLog << "\n[WARNING] Exception during peak_simulation: "
		           << e.msg();
#endif
	} catch (const std::exception& e) {
		figTechLog << "\n[ERROR] Unexpected exception during "
		           << "peak_simulation: " << e.what() << std::endl;
		throw;
	}

	traial.level = maxImportance;
	traial.state = maxImportanceState;
	TraialPool::set_timeouts(traial, maxImportanceClocks);

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

