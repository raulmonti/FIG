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
#include <string>
#include <memory>
// FIG
#include <core_typedefs.h>
#include <ImportanceFunction.h>
#include <Property.h>
#include <Traial.h>


namespace fig
{

class ConfidenceInterval;
class StoppingConditions;

/**
 * @brief Abstract base simulation engine
 *
 *        Simulation engines embody the semantics of the different
 *        simulation strategies offered by the FIG tool, such as the
 *        RESTART importance splitting technique.
 */
class SimulationEngine
{
protected:

    /// Simulation strategy implemented by this engine
    std::string name_;

    /// Property whose value is trying to be estimated
    const Property* property;

    /// Importance function currently built
    std::shared_ptr< const ImportanceFunction > impFun;

public:  // Ctors/Dtor

    /// No data ctor, only empty ctor provided
    SimulationEngine() : property(nullptr), impFun(nullptr) {}
    /// Default copy ctor
    SimulationEngine(const SimulationEngine& that) = default;
    /// Default move ctor
    SimulationEngine(SimulationEngine&& that) = default;
    /// Default copy assignment
    SimulationEngine& operator=(const SimulationEngine& that) = default;
    /// Default move assignment
    SimulationEngine& operator=(SimulationEngine&& that) = default;
    /// Virtual dtor
    virtual ~SimulationEngine() { unload(); }

public:  // Engine setup

	/// @brief Is this engine ready for simulations?
	/// @details True after a successfull call to load().
	///          False again after a call to unload().
	inline bool loaded() const noexcept
		{ return nullptr != property && nullptr != impFun; }

	/// @copydoc loaded()
	inline bool ready() const noexcept { return loaded(); }

    /**
     * @brief Register the property and importance function
     *        which will be used in the following simulations
     * @param prop  Property whose value is to be estimated
     * @param ifun  ImportanceFunction which will be used for estimations
     * @throw FigException if either the Property or the ImportanceFunction
     *                     are incompatible with this SimulationEngine
     * @see unload()
     */
    virtual void load(const Property& prop,
                      std::shared_ptr< const ImportanceFunction > ifun)
        { property = &prop; impFun = ifun; }

    /// Deregister the last Property and ImportanceFunction which were setup
    /// @see load()
    inline void unload() noexcept
        { property = nullptr; impFun = nullptr; }

public:  // Accessors

    /// @copydoc name_
    inline const std::string& name() const noexcept { return name_; }

    /// Property currently loaded in the engine, or void string if none is.
    inline const std::string current_prop() const noexcept
        { if (nullptr != property) return property->expression; else return ""; }

    /// Importance strategy of the function currently loaded in the engine,
    /// or void string if none is.
    inline const std::string current_ifun() const noexcept
        { if (nullptr != impFun) return impFun->name(); else return ""; }

public:  // Simulation utils

    /**
     * @brief Simulate in model certain number of independent runs
     *
     *        The property whose value is being estimated as well as the
     *        importance function used are taken from the last call to load()
     *
     * @param numRuns Number of indepdendent runs to perform
     *
     * @return Estimated value for the property at the end of this simulation
     *
     * @throw FigException if the engine wasn't \ref loaded() "ready"
     */
    virtual double simulate(const size_t& numRuns) const = 0;

    /**
     * @brief Simulate in model until externally interrupted
     *
     *        The property whose value is being estimated as well as the
     *        importance function used are taken from the last call to load()
     *
     * @param ci ConfidenceInterval regularly updated with estimation info
     *
     * @throw FigException if the engine wasn't \ref loaded() "ready"
     */
    virtual void simulate(ConfidenceInterval& ci) const = 0;

    /**
     * @brief Were the last events triggered by the given Traial
     *        relevant for this simulation engine?
     *
     *        The property whose value is being estimated as well as the
     *        importance function used are taken from the last call to load()
     *
     * @param traial Embodiment of a simulation running through the system model
     */
    virtual bool eventTriggered(const Traial& traial) const = 0;
};

} // namespace fig

#endif // SIMULATIONENGINE_H

