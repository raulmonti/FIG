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
#include <core_typedefs.h>
#include <Property.h>
#include <ImportanceFunction.h>


namespace fig
{

class ConfidenceInterval;
class StoppingConditions;
class ModuleNetwork;
class Traial;

/**
 * @brief Abstract base simulation engine
 *
 *        Simulation engines embody the semantics of the different
 *        simulation strategies offered by the FIG tool, such as the
 *        RESTART importance splitting technique.
 */
class SimulationEngine
{
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
    std::shared_ptr< const ImportanceFunction > impFun;

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
     * @brief Register the importance function which will be used in the
     *        following estimations
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
    inline const std::string& name() const noexcept { return name_; }

    /// Importance strategy of the function currently bound to the engine,
    /// or void string if none is.
    inline const std::string current_ifun() const noexcept
        { if (nullptr != impFun) return impFun->name(); else return ""; }

public:  // Simulation utils

    /**
     * @brief Simulate in model certain number of independent runs
     *
     *        This is intended for "value simulations", viz. when estimations
     *        finish as soon as certain confidence criterion is met.
     *        The importance function used is taken from the last call to bind()
     *
     * @param property Property whose value is being estimated
     * @param numRuns  Number of indepdendent runs to perform
     *
     * @return Estimated value for the property at the end of this simulation,
     *         or negative value if something went wrong.
     *
     * @throw FigException if the engine wasn't \ref bound() "bound" to any
     *                     ImportanceFunction
     */
    virtual double simulate(const Property& property,
                            const size_t& numRuns = 1) const = 0;

    /**
     * @brief Simulate in model until externally interrupted
     *
     *        This is intended for "time simulations", viz. when estimations
     *        run indefinitely until they're externally interrupted.
     *        The importance function used is taken from the last call to bind()
     *
     * @param property Property whose value is being estimated
     * @param interval ConfidenceInterval regularly updated with estimation info
     *
     * @throw FigException if the engine wasn't \ref bound() "bound" to any
     *                     ImportanceFunction
     */
    virtual void simulate(const Property& property,
                          ConfidenceInterval& interval) const = 0;

    /**
     * @brief Were the last events triggered by the given Traial
     *        relevant for this property and simulation strategy?
     *
     * @param property Property whose value is being estimated
     * @param traial   Embodiment of a simulation running through the system model
     *
     * @note The importance function used is taken from the last call to bind()
     */
    virtual bool event_triggered(const Property& property,
                                 const Traial& traial) const = 0;
};

} // namespace fig

#endif // SIMULATIONENGINE_H

