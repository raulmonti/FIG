//==============================================================================
//
//  ImportanceFunction.h
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


#ifndef IMPORTANCEFUNCTION_H
#define IMPORTANCEFUNCTION_H

// C++
#include <string>
#include <array>
// FIG
#include <core_typedefs.h>
#include <State.h>


namespace fig
{

class ModuleInstance;
class ModuleNetwork;
class Property;

/**
 * @brief Abstract base importance assessor (or function)
 *
 *        Importance functions are required for the application of
 *        importance splitting techniques during Monte Carlo simulations.
 *        Based on an identifyable set of "rare states", importance functions
 *        are in charge of assessing how likely it is to visit such set
 *        from <i>each other</i> (reachable) system state.
 *
 *        Besides from the "name" which specifies the ImportanceFunction
 *        derived class, importance assessment requires the choice of a
 *        "strategy" (flat, auto, ad hoc...) to decide how the relative
 *        importance between states will be measured.
 *
 * @note This class family follows the
 *       <a href="https://sourcemaking.com/design_patterns/visitor">
 *       visitor design pattern</a>. The visited elements are instances
 *       of the classes which derive from Module.
 */
class ImportanceFunction
{
public:

	/// Names of the importance functions offered to the user,
	/// as he should requested them through the CLI/GUI.
	/// Defined in ImportanceFunction.cpp
	static const std::array< std::string, 1 > names;

	/// Importance assessment strategies offered to the user,
	/// as he should requested them through the CLI/GUI.
	/// Defined in ImportanceFunction.cpp
	static const std::array< std::string, 4 > strategies;

private:

	/// Importance function implemented by this instance
	/// Check ImportanceFunction::names for available options.
	std::string name_;

protected:

	/// Can this instance be used for simulations?
	bool readyForSimulations;

	/// Last used strategy to assess the importance with this function
	std::string strategy_;

public:  // Ctor/Dtor

	/**
	 * Data ctor
	 * @param name @copydoc name_
	 * @throw FigException if the name doesn't match a valid function
	 */
	ImportanceFunction(const std::string& name);

	/// Dtor
	virtual ~ImportanceFunction() {}

public:  // Accessors

	/// @copydoc name_
	const std::string& name() const noexcept;

	/**
	 * @copydoc readyForSimulations
	 *
	 *        This starts out false and becomes true after a successfull call
	 *        to one of the importance assessment functions.
	 *        It becomes false again after a call to clear()
	 *
	 * @see assess_importance(ModuleInstance*, Property*)
	 * @see assess_importance(ModuleNetwork*,  Property*)
	 */
	bool ready() const noexcept;

	/// @copydoc strategy_
	/// @returns Empty string if function isn't ready(), strategy name otherwise
	const std::string& strategy() const noexcept;

	/// Whether this instance stores importance values for the concrete state
	/// space (as opposed to the symbolic state space)
	virtual bool concrete() const noexcept = 0;

public:  // Utils

	/**
	 * @brief Assess the importance of the states on this \ref ModuleInstance
	 *        "module", according to the \ref Property "logical property" and
	 *        strategy specified.
	 *
	 * @param mod      Module whose reachable states will have their importance
	 *                 assessed. Its current state is considered initial.
	 * @param prop     Property guiding the importance assessment
	 * @param strategy Strategy of the assessment (flat, auto, ad hoc...)
	 *
	 * @note After a successfull invocation the ImportanceFunction
	 *       is ready() to be used during simulations
	 *
	 * @see assess_importance(const ModuleNetwork&, const Property&, const std::string&)
	 */
	virtual void assess_importance(const ModuleInstance& mod,
								   const Property& prop,
								   const std::string& strategy = "") = 0;

	/**
	 * @brief Assess the importance of the reachable states of the whole
	 *        \ref ModuleNetwork "system model", according to the
	 *        \ref Property "logical property" and strategy specified.
	 *
	 * @param net      System model (or coupled network of modules)
	 *                 Its current state is taken as the model's initial state.
	 * @param prop     Property guiding the importance assessment
	 * @param strategy Strategy of the assessment (flat, auto, ad hoc...)
	 *
	 * @note After a successfull invocation the ImportanceFunction
	 *       is ready() to be used during simulations
	 *
	 * @see assess_importance(const ModuleInstance&, const Property&, const std::string&)
	 */
	virtual void assess_importance(const ModuleNetwork& net,
								   const Property& prop,
								   const std::string& strategy = "") = 0;

	/// @brief Tell the pre-computed importance of the given StateInstance
	/// @note All reachable states importance should have already been assessed
	/// @see assess_importance()
	virtual ImportanceValue importance_of(const StateInstance& state) const = 0;

	/// Release any memory allocated in the heap
	/// @note After this invocation the ImportanceFunction is no longer ready()
	virtual void clear() noexcept = 0;
};

} // namespace fig

#endif // IMPORTANCEFUNCTION_H

