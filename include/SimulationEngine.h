//==============================================================================
//
//  SimulationEngine.h
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


#ifndef SIMULATIONENGINE_H
#define SIMULATIONENGINE_H

// C++
#include <map>
#include <array>
#include <string>
#include <memory>
#include <ostream>
// FIG
#include <State.h>


namespace fig
{

class ImportanceFunction;
class ImportanceFunctionConcrete;
class Property;
class PropertyRate;
class PropertyTransient;
class ModuleNetwork;
class Traial;
class ConfidenceInterval;
class ConfidenceIntervalRate;
class ConfidenceIntervalTransient;

/**
 * @brief Abstract base simulation engine
 *
 *        Simulation engines embody the semantics of the different
 *        simulation strategies offered by the FIG tool, such as the
 *        RESTART importance splitting technique.
 */
class SimulationEngine
{
    friend class ModelSuite;  // for locking and interruptions signaling

public:

    /// Long story short: number of concrete derived classes.
    /// More in detail this is the size of the array returned by names(), i.e.
    /// how many SimualtionEngine implementations are offered to the end user.
	static constexpr size_t NUM_NAMES = 3;

protected:  // Attributes for simulation update policies

    /// Minimum amount of generated rare events to consider a simulation "good"
    /// @note Relevant for transient-like simulations only
    /// @warning Value is arbitrary af
    /// @deprecated Current transient policies use fixed batch sizes,
    ///             so this value isn't used
    static constexpr unsigned MIN_COUNT_RARE_EVENTS = 3u;

    /// Minimum amount of simulation-time units which has to be spent
    /// in rare states to consider a simulation "good"
    /// @note Relevant for steady-state-like simulations only
    /// @warning Value is arbitrary af
    static constexpr double MIN_ACC_RARE_TIME = 0.3;

    /// Upper bound of CPU time (seconds) for a single simulation.
    /// If simulations take longer than this then the update policies
    /// won't perform further effort increases (batch size / run length)
    /// @note This doesn't imply truncation: simulations running longer than
    ///       MAX_CPU_TIME seconds <b>won't</b> be stopped prematurely.
    /// @warning Value is arbitrary af
    static constexpr long MAX_CPU_TIME = 120l;

    /// Maximum simulation-time units any Traial is allowed to accumulate
    /// before having its lifetime reset
    /// @note Needed due to fp precision issues
    /// @note Value chosen small enough to distinguish variations of 0.01
    ///       simulation-time units when using fp single precision:
    ///       mantissa 1, exponent 12, resulting in 1*2^12 == 4096.
    ///       The corresponding C99 literal is 0x1p12.
    /// @see <a href="http://www.cprogramming.com/tutorial/floating_point/understanding_floating_point_representation.html">
    ///      Floating point arithmetic</a> and the <a href="http://stackoverflow.com/a/4825867">
    ///      C99 fp literals</a>.
    static constexpr CLOCK_INTERNAL_TYPE SIM_TIME_CHUNK = 4096.f;

private:  // Instance attributes

	/// Name of the SimulationEngine strategy implemented by this instance.
	/// @note Check names() for available options.
	std::string name_;

    /// Is the engine currently being used in an estimation?
    mutable bool locked_;

protected:

    /// User's system model, already sealed
	std::shared_ptr< const ModuleNetwork > model_;

    /// Importance function currently built
    std::shared_ptr< const ImportanceFunction > impFun_;

	/// Concrete importance function currently built, if any
	std::shared_ptr< const ImportanceFunctionConcrete > cImpFun_;

	/// Were we just interrupted in an estimation timeout?
	mutable bool interrupted;

	/// The engine is intended to be used by a ThresholdsBuilder
	const bool toBuildThresholds_;

	/// Maximum simulation time to reach, for long-run simulations only
	/// @note Used only by derived classes
	mutable CLOCK_INTERNAL_TYPE simsLifetime;
//	mutable thread_local CLOCK_INTERNAL_TYPE simsLifetime;

public:

	/// Count how many Traials make it to each importance/threshold level
	typedef std::map< ImportanceValue, unsigned > ReachabilityCount;

	/// Wrapper for member functions to be forwarded to
	/// ModuleNetwork::simulation_step(), to use as TraialMonitor
	/// @see transient_event(), rate_event()
	typedef std::function<bool(const Property&, Traial&, Event&)> EventWatcher;

protected:

	/// How many Traials reached each threshold level in the last simulation
	/// @note Useful for debugging purposes
	/// @note Only applicable to \ref isplit() "engines implementing
	///       some sort of importance splitting"
//	mutable std::vector< size_t > reachCount_;
	mutable ReachabilityCount reachCount_;

public:  // Ctors/Dtor

    /**
     * Data ctor
	 * @param name        @copybrief name_
	 * @param model       @copybrief model_
	 * @param thresholds  @copybrief toBuildThresholds_
     * @throw FigException if the name doesn't match a valid engine
     * @throw FigException if the system model hasn't been sealed yet
     */
	SimulationEngine(const std::string& name,
					 std::shared_ptr<const ModuleNetwork> model,
					 const bool thresholds = false);
    /// Default copy ctor
    SimulationEngine(const SimulationEngine& that) = default;
    /// Default move ctor
    SimulationEngine(SimulationEngine&& that) = default;
    /// Default copy assignment
    SimulationEngine& operator=(const SimulationEngine& that) = default;
    /// Default move assignment
    SimulationEngine& operator=(SimulationEngine&& that) = default;
    /// Virtual dtor
    virtual ~SimulationEngine() noexcept;

protected:  // Engine setup (by ModelSuite)

    /**
     * @brief Couple with an ImportanceFunction for future estimations
     *
	 *        Register \p ifun as the ImportanceFunction to use in estimations,
	 *        and register ourselves into \p ifun as the user engine.
     *        Any previously bound ImportanceFunction is kicked out.
     *
	 * @param ifun  ImportanceFunction to bind and be bound to
	 *
	 * @throw FigException if \p ifun is incompatible with this engine
	 * @throw FigException if the engine was \ref lock() "locked"
	 * @see unbind()
	 */
	virtual void bind(std::shared_ptr< const ImportanceFunction > ifun);

	/// Deregister the last ImportanceFunction coupled to us
	/// @throw FigException if the engine was \ref lock() "locked"
	/// @see bind()
	void unbind();

//	/// Set the global effort to use in all threshold-levels
//	/// @note Relevant only for the Importance Splitting engines
//	/// @see global_effort()
//	/// @see ModelSuite::set_global_effort()
//	virtual void set_global_effort(unsigned) = 0;
//
//	/// Set the engine-specific default global effort
//	/// @see set_global_effort()
//	inline void set_global_effort() { set_global_effort(global_effort_default()); }

private:  // Engine setup (by ModelSuite)

    /**
     * @brief Lock this engine into "simulation mode"
     * @details When an engine is locked only its const-qualified
     *          member functions can be invoked. This includes all "Accessors"
     *          and "Simulation functions", e.g. ready(), current_imp_strat(),
     *          simulate(), etc.
     * @note This is intended for safe use of the engine by the ModelSuite
     *       instance during a call to ModelSuite::estimate()
     * @throw FigException if the engine was already \ref lock() "locked"
     * @see unlock()
     */
    void lock() const;

    /// Lock this engine out of "simulation mode"
    /// @see lock()
    void unlock() const noexcept;

public:  // Accessors

	/// Names of the simulation engines offered to the user,
	/// as he should requested them through the CLI/GUI.
	/// @note Implements the <a href="https://goo.gl/yhTgLq"><i>Construct On
	///       First Use</i> idiom</a> for static data members,
	///       to avoid the <a href="https://goo.gl/chH5Kg"><i>static
	///       initialization order fiasco</i>.
	static const std::array< std::string, NUM_NAMES >& names() noexcept;

    /// @copydoc name_
    const std::string& name() const noexcept;

	/// Does this engine implement some sort of importance splitting?
	virtual bool isplit() const noexcept = 0;

    /// @brief Is this engine tied up to an ImportanceFunction,
    ///        and thus ready for simulations?
    /// @details True after a successfull call to bind().
    ///          False again after a call to unbind().
    bool bound() const noexcept;

    /// Alias for bound()
    inline bool ready() const noexcept { return bound(); }

    /// Is this engine \ref lock() "locked" into "simulation mode"?
    bool locked() const noexcept;

    /// Name of the ImportanceFunction currently bound to the engine,
    /// or void string if none is.
    const std::string current_imp_fun() const noexcept;

    /// Importance strategy of the function currently bound to the engine,
    /// or void string if none is.
    const std::string current_imp_strat() const noexcept;

	/**
	 * @brief (Global) Effort performed per step-advance of the simulations
	 *
	 *         If a non-global thresholds selection mechanism is chosen,
	 *         e.g. \ref ThresholdsBuilderES "Expected Success",
	 *         0 is returned; else:
	 *         <ul>
	 *         <li>In RESTART this is 1 + #(replicas) made of a Traial
	 *             when it crosses a threshold-level upwards,</li>
	 *         <li>In Fixed Effort this is the #(simulations) launched
	 *             on each threshold-level.</li>
	 *         </ul>
	 *
	 * @return Global effort as described in the details,
	 *         or 0 if the thresholds builder isn't ThresholdsBuilderAdaptiveSimple
	 * @note Relevant only for Importance Splitting engines,
	 *       and when a global effort mechanism is used to choose the thresholds.
	 * @see ThresholdsBuilderAdaptiveSimple
	 * @see ThresholdsBuilderES
	 */
	virtual unsigned global_effort_default() const noexcept = 0;

//	/// Set an engine-specific value for the global effort
//	virtual unsigned global_effort() const noexcept = 0;

	/// @copydoc reachCount_
	inline decltype(reachCount_) get_reach_counts() const noexcept { return reachCount_; }

public:  // Simulation functions

    /**
     * @brief Run simulation in model
     *
     *        There are two ways of defining when does a simulation end:
     *        "by time" or "by value".<br>
     *        In <i>time simulations</i> the estimation runs indefinitely until
     *        the engine is externally signaled by an update of the
     *        \ref interrupted "interrupted flag". Signals are usually time-
     *        driven, e.g. "stop after running for 2h".<br>
     *        In <i>value simulations</i> the estimation finishes as soon as
     *        certain confidence criterion is met, although truncation by
     *        updates of the interrupted flag is also possible.
     *
     * @param property Property whose probability value is to be estimated
	 * @param ci       ConfidenceInterval where the estimation info will be placed
     *
     * @note  The ImportanceFunction used is taken from the last call to bind()
     *
     * @throw FigException if the engine isn't \ref bound() "bound" to any
     *                     ImportanceFunction, or if simulations are marked
     *                     \ref interrupted "interrupted" from the start
     */
    void simulate(const Property& property, ConfidenceInterval& ci) const;

protected:  // Simulation helper functions

	/**
	 * @brief Run independent transient-like simulations to estimate
	 *        the value of a \ref PropertyTransient "transient property"
     *
     *        Launch 'numRuns' transient simulations starting from the initial
     *        state of the system. The given 'property' is characterized by
     *        two subformulas: "expr1" and "expr2". Each simulation stops when
     *        a state is visited which either satisfies "expr2" or doesn't
     *        satisfy "expr1".
     *
     * @param property PropertyTransient with events of interest (expr1 & expr2)
	 * @param numRuns  Amount of successive independent simulations to run
     *
     * @return Vector with the (weighed) number of states which reached 'expr2'
     *         on each of the simulations performed
     *
     * @note Less than 'numRuns' simulations may be run when the engine is
     *       interrupted. The number of simulations effectively ran equals
     *       the size of the return vector.
     *
     * @see PropertyTransient
	 */
	virtual std::vector<double>
	transient_simulations(const PropertyTransient& property,
						  const size_t& numRuns) const = 0;

	/**
	 * @brief Perform a long-run simulation to estimate the value of a
	 *        \ref PropertyRate "rate property"
	 *
	 *        Using the engine's simulation strategy, run a simulation lasting
	 *        'runLength' simulation-time units. The given 'property' is
	 *        characterized by a subformula "expr". The total amount of
	 *        simulation-time spent in states satisfying "expr" is tracked.
	 *
	 * @param property  PropertyRate with the event of interest (expr)
	 * @param runLength Simulation-time units the simulation run will last
	 * @param reinit    Whether to start from the system's initial state
	 *                  instead of the state saved from the last call.
	 *                  Make this parameter false to use <i>batch means</i>.
	 *
	 * @return Amount of simulation-time spent on states which satisfy "expr".
	 *         The desired <i>rate</i>, i.e. the  proportion of simulation-time
	 *         spent on rare states, is: returnValue / runLength
	 *
	 * @note The routine supports the <i>batch means simulation method</i>,
	 *       viz. execution can start from the last saved state, as if the
	 *       simulation run continued from the previous call.
	 * @note The first time this routine is called (globally) simulations
	 *       forcefully start from the system's initial state.
	 * @warning Implementations are currently <b>not thread-safe</b>.
	 *
	 * @see PropertyRate
	 */
	virtual double
	rate_simulation(const PropertyRate& property,
					const size_t& runLength,
					bool reinit = false) const = 0;

protected:  // Traial observers/updaters

    /**
     * @brief Interpret and mark the transient events triggered by a Traial
     *        in its most recent traversal through the system model.
     *
	 * @param transientProperty  PropertyTransient with events of interest (expr1 & expr2)
	 * @param traial  Embodiment of a simulation running through the system
	 *                model <b>(modified)</b>
	 * @param e       Variable to update with observed events <b>(modified)</b>
     *
     * @return Whether a \ref ModuleNetwork::simulation_step() "simulation step"
     *         has finished and the Traial should be further inspected.
     *
     * @note  The ImportanceFunction used is taken from the last call to bind()
     */
	virtual bool transient_event(const Property& transientProperty,
                                 Traial& traial,
                                 Event& e) const = 0;

    /**
     * @brief Notice any event triggered by a Traial in its most recent
     *        traversal through the system model. After a positive return
     *        the Traial's evolution should be watched more closely.
     *
	 * @param rateProperty  PropertyRate with the event of interest (expr)
	 * @param traial  Embodiment of a simulation running through the system
	 *                model <b>(modified)</b>
	 * @param e       Variable to update with observed events <b>(modified)</b>
     *
     * @return Whether a \ref ModuleNetwork::simulation_step() "simulation step"
     *         has finished and the Traial is in a state whose sojourn time
     *         should be registered.
     *
     * @note  The ImportanceFunction used is taken from the last call to bind()
     */
	virtual bool rate_event(const Property& rateProperty,
                            Traial& traial,
                            Event& e) const = 0;

private:  // Class utils

	/**
	 * @brief Update the ConfidenceInterval for transient-like properties
	 *
	 * @param ci           ConfidenceInterval to update <b>(modified)</b>
	 * @param weighedNREs  Vector with the (weighed) number of rate states
	 *                     visited in last simulations batch
	 *
	 * @note Simulations can be truncated by external updates to the
	 *       \ref interrupted "interrupted flag": <b>nothing will be done
	 *       if such flag is set</b>
	 */
	void transient_update(ConfidenceIntervalTransient& ci,
						  const std::vector<double>& weighedNREs) const;

	/**
	 * @brief Update the ConfidenceInterval and the simulation effort
	 *        for rate-like properties
	 *
	 * @param ci       ConfidenceInterval to update <b>(modified)</b>
	 * @param rareTime Simulation-time units spent on rare states in the last simulation
	 * @param simTime  Total simulation-time units spent in last simulation <b>(modified)</b>
	 * @param CPUtime  Processor time used in last simulation, in seconds
	 *
	 * @note Current policy discards the first "not-steady-state" trace
	 *       (check source for details)
	 * @note Simulations can be truncated by external updates to the
	 *       \ref interrupted "interrupted flag": <b>nothing will be done
	 *       if such flag is set</b>
	 */
	void rate_update(ConfidenceIntervalRate& ci,
					 const double& rareTime,
					 size_t& simTime,
					 const long& CPUtime) const;
};

} // namespace fig

#endif // SIMULATIONENGINE_H

