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
// FIG
#include <State.h>


namespace fig
{

class ImportanceFunction;
class ImportanceFunctionConcrete;
class Property;
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
    friend class ModelSuite;  // for interruptions signaling

    typedef const std::string ConstStr;

public:

	/// Names of the simulation engines offered to the user,
	/// as he should requested them through the CLI/GUI.
    /// Defined in SimulationEngine.cpp
    static const std::array< std::string, 2 > names;

protected:

    /// Simulation strategy implemented by this engine.
    /// Check SimulationEngine::names for available options.
    std::string name_;

    /// User's system model, already sealed
    std::shared_ptr< const ModuleNetwork > network_;

    /// Importance function currently built
    std::shared_ptr< const ImportanceFunction > impFun_;

    /// Were we just interrupted in an estimation timeout?
    mutable bool interrupted;

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

    /// @brief Is this engine tied up to an ImportanceFunction,
    ///        and thus ready for simulations?
    /// @details True after a successfull call to bind().
    ///          False again after a call to unbind().
    bool bound() const noexcept;

    /// Alias for bound()
    inline bool ready() const noexcept { return bound(); }

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
     * @see unbind()
     */
    virtual void bind(std::shared_ptr< const ImportanceFunction > ifun);

    /// Deregister the last ImportanceFunction which was tied to us
    /// @see bind()
    void unbind() noexcept;

public:  // Accessors

    /// @copydoc name_
    const std::string& name() const noexcept;

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
     * @param numRuns  Number of indepdendent runs to perform
     * @param interval ConfidenceInterval updated with estimation info <b>(modified)</b>
     *
     * @return Whether 'numRuns' wasn't large enough and ought to be increased
     *
     * @throw FigException if the engine wasn't \ref bound() "bound" to any
     *                     ImportanceFunction
     */
    bool simulate(const Property& property,
                  const size_t& numRuns,
                  ConfidenceInterval& interval) const;

    /**
     * @brief Simulate in model until externally interrupted
     *
     *        This is intended for "time simulations", viz. when estimations
     *        run indefinitely until they're externally interrupted.
     *        The importance function used is taken from the last call to bind()
     *
     * @param property  Property whose value is being estimated
     * @param batchSize Number of consecutive simulations for each interval update
     * @param interval  ConfidenceInterval regularly updated with estimation info <b>(modified)</b>
     * @param batch_inc Function to increment 'batchSize' in case simulations
     *                  aren't yielding useful results due to its length
     *
     * @throw FigException if the engine wasn't \ref bound() "bound" to any
     *                     ImportanceFunction
     */
    void simulate(const Property& property,
                  size_t batchSize,
                  ConfidenceInterval& interval,
                  void (*batch_inc)(size_t&, ConstStr&, ConstStr&)) const;

protected:  // Simulation helper functions

	/// Logarithm of the number of experiments virtually performed
	/// on each internal iteration of a simulate() function
	/// @throw FigException if the engine wasn't bound() to an ImportanceFunction
	virtual double log_experiments_per_sim() const = 0;

	/**
     * @brief Run several independent transient-like simulations
     *
     *        Using a specific simulation strategy perform 'numRuns'
     *        transient simulation runs. These will end when either
     *        a 'goal' or 'stop' event is observed.
     *
     * @param property PropertyTransient with events of interest (goal & stop)
	 * @param numRuns  Amount of successive independent simulations to run
     *
     * @return Number of 'goal' events observed, or its negative value
     *         if less than ModelSuite::MIN_COUNT_RARE_EVENTS were observed.
     *
     * @see PropertyTransient
	 */
	virtual double transient_simulations(const PropertyTransient& property,
                                         const size_t& numRuns) const = 0;

public:  // Traial observers/updaters

    /**
     * @brief Interpret and mark the transient events triggered by a Traial
     *        in its most recent traversal through the system model.
     *
     * @param property PropertyTransient with events of interest (goal & stop)
     * @param traial   Embodiment of a simulation running through the system model <b>(modified)</b>
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
};

} // namespace fig

#endif // SIMULATIONENGINE_H

