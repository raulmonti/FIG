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

	/// Names of the simulation engines offered to the user,
	/// as he should requested them through the CLI/GUI.
    /// Defined in SimulationEngine.cpp
    static const std::array< std::string, 2 > names;

    /// Minimum amount of generated rare events to consider a simulation "good"
    static const unsigned MIN_COUNT_RARE_EVENTS;

    /// Minimum amount of simulation time units which has to be spent
    /// in rare states to consider a simulation "good"
    static const double MIN_ACC_RARE_TIME;

private:

    /// Simulation strategy implemented by this engine.
    /// Check SimulationEngine::names for available options.
    std::string name_;

    /// Is the engine currently being used in an estimation?
    mutable bool locked_;

protected:

    /// User's system model, already sealed
    std::shared_ptr< const ModuleNetwork > network_;

    /// Importance function currently built
    std::shared_ptr< const ImportanceFunction > impFun_;

	/// Concrete importance function currently built, if any
	std::shared_ptr< const ImportanceFunctionConcrete > cImpFun_;

    /// Were we just interrupted in an estimation timeout?
    mutable bool interrupted;

    /// Maximum simulation time to reach, for long-run simulations only
    mutable CLOCK_INTERNAL_TYPE simsLifetime;

public:  // Ctors/Dtor

    /**
     * Data ctor
     * @param name @copydoc name_
     * @param network @copydoc network_
     * @throw FigException if the name doesn't match a valid engine
     * @throw FigException if the system model hasn't been sealed yet
     */
    SimulationEngine(const std::string& name,
                     std::shared_ptr<const ModuleNetwork> network);
    /// Default copy ctor
    SimulationEngine(const SimulationEngine& that) = default;
    /// Default move ctor
    SimulationEngine(SimulationEngine&& that) = default;
    /// Default copy assignment
    SimulationEngine& operator=(const SimulationEngine& that) = default;
    /// Default move assignment
    SimulationEngine& operator=(SimulationEngine&& that) = default;
    /// Virtual dtor
    virtual ~SimulationEngine();

public:  // Engine setup

    /**
     * @brief Couple with an ImportanceFunction for future estimations
     *
     *        Attach passed ImportanceFunction, which must be
     *        \ref ImportanceFunction::ready() "ready for simulations",
     *        to be used by this engine in estimations to come.
     *        Any previously bound ImportanceFunction is kicked out.
     *
     * @param ifun  ImportanceFunction to use, \ref ImportanceFunction::ready()
     *              "ready for simulations"
     * @throw FigException if the ImportanceFunction isn't
     *                     \ref ImportanceFunction::ready() "ready",
     *                     or if it is incompatible with this engine
     * @throw FigException if the engine was \ref lock() "locked"
     * @see unbind()
     */
    virtual void bind(std::shared_ptr< const ImportanceFunction > ifun);

    /// Deregister the last ImportanceFunction which was tied to us
    /// @throw FigException if the engine was \ref lock() "locked"
    /// @see bind()
    void unbind() noexcept;

private:

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

    /// @copydoc name_
    const std::string& name() const noexcept;

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

	/// 1 + Number of replicas made of a Traial when it crosses
	/// an importance threshold upwards (i.e. gaining on importance)
	/// @see ThresholdsBuilder
	virtual unsigned splits_per_threshold() const noexcept = 0;

public:  // Simulation functions

    /**
     * @brief Simulate in model certain number of independent runs
     *
     *        This is intended for "value simulations", viz. when estimations
     *        finish as soon as certain confidence criterion is met.
     *        The importance function used is taken from the last call to bind()
     *
     * @param property Property whose value is being estimated
     * @param effort   Number of independent runs to perform or
     *                 simulation length in time units
     * @param interval ConfidenceInterval updated with estimation info <b>(modified)</b>
     *
     * @return Whether 'effort' wasn't large enough and ought to be increased
     *
     * @throw FigException if the engine wasn't \ref bound() "bound" to any
     *                     ImportanceFunction
     */
    bool simulate(const Property& property,
                  const size_t& effort,
                  ConfidenceInterval& interval) const;

    /**
     * @brief Simulate in model until externally interrupted
     *
     *        This is intended for "time simulations", viz. when estimations
     *        run indefinitely until they're externally interrupted.
     *        The importance function used is taken from the last call to bind()
     *
     * @param property   Property whose value is being estimated
     * @param effort     Number of consecutive independent simulations or
     *                   simulation length in time units
     *                   before each interval update
     * @param interval   ConfidenceInterval regularly updated with estimation info <b>(modified)</b>
     * @param techLog    Log where technical info will be printed
     * @param effort_inc Function to increase 'effort' in case simulations
     *                   aren't yielding useful results due to its length
     *
     * @throw FigException if the engine wasn't \ref bound() "bound" to any
     *                     ImportanceFunction
     */
    void simulate(const Property& property,
                  size_t effort,
                  ConfidenceInterval& interval,
                  std::ostream& techLog,
                  void (*effort_inc)(const PropertyType&,
                                     const std::string&,
                                     const std::string&,
                                     size_t&) = nullptr) const;

protected:  // Simulation helper functions

	/// Logarithm of the number of experiments virtually performed
	/// on each internal iteration of a simulate() function
	/// @throw FigException if the engine wasn't bound() to an ImportanceFunction
	virtual double log_experiments_per_sim() const = 0;

	/**
	 * @brief Run independent transient-like simulations to estimate
	 *        the value of a \ref PropertyTransient "transient property"
     *
     *        Using a specific simulation strategy, launch 'numRuns' transient
     *        simulations starting from the system's initial state.
     *        The given 'property' is characterized by two subformulas:
     *        "expr1" and "expr2". Each simulation run stops when a state
     *        which either satisfies "expr2" or doesn't satisfy "expr1"
     *        is visited.
     *
     * @param property PropertyTransient with events of interest (expr1 & expr2)
	 * @param numRuns  Amount of successive independent simulations to run
     *
     * @return Number of states satisfying 'expr2' reached, or its negative
     *         value if less than MIN_COUNT_RARE_EVENTS were observed.
     *
     * @see PropertyTransient
	 */
	virtual double transient_simulations(const PropertyTransient& property,
                                         const size_t& numRuns) const = 0;

	/**
	 * @brief Perform a long-run simulation to estimate the value of a
	 *        \ref PropertyRate "rate property"
	 *
	 *        Using a specific simulation strategy, run a simulation which will
	 *        last for 'runLength' simulated time units. The given 'property'
	 *        is characterized by a subformula "expr". The total amount of
	 *        simulated time spent in states satisfying "expr" is monitored
	 *        and used to compute the return value.
	 *
	 * @param property  PropertyRate with the event of interest (expr)
	 * @param runLength Simulated time units the simulation run will last
	 *
	 * @return Proportion of the total simulated time which was spent
	 *         on states satisfying the property's "expr", or its negative
	 *         value if less than MIN_ACC_RARE_TIME was spent there.
	 *
	 * @see PropertyRate
	 */
	virtual double rate_simulation(const PropertyRate& property,
								   const size_t& runLength) const = 0;

public:  // Traial observers/updaters

    /**
     * @brief Interpret and mark the transient events triggered by a Traial
     *        in its most recent traversal through the system model.
     *
     * @param property PropertyTransient with events of interest (expr1 & expr2)
     * @param traial   Embodiment of a simulation running through the system
     *                 model <b>(modified)</b>
     * @param e        Variable to update with observed events <b>(modified)</b>
     *
     * @return Whether a \ref ModuleNetwork::simulation_step() "simulation step"
     *         has finished and the Traial should be further inspected.
     *
     * @note  The ImportanceFunction used is taken from the last call to bind()
     */
    virtual bool transient_event(const PropertyTransient& property,
                                 Traial& traial,
                                 Event& e) const = 0;

    /**
     * @brief Notice any event triggered by a Traial in its most recent
     *        traversal through the system model. After a positive return
     *        the Traial's evolution should be watched more closely.
     *
     * @param property  PropertyRate with the event of interest (expr)
     * @param traial    Embodiment of a simulation running through the system
     *                  model <b>(modified)</b>
     * @param e         Variable to update with observed events <b>(modified)</b>
     *
     * @return Whether a \ref ModuleNetwork::simulation_step() "simulation step"
     *         has finished and the Traial is in a state whose sojourn time
     *         should be registered.
     *
     * @note  The ImportanceFunction used is taken from the last call to bind()
     */
    virtual bool rate_event(const PropertyRate& property,
                            Traial& traial,
                            Event& e) const = 0;
};

} // namespace fig

#endif // SIMULATIONENGINE_H

