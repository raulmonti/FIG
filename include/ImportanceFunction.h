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
 * @note This class family follows the
 *       <a href="https://sourcemaking.com/design_patterns/visitor">
 *       visitor design pattern</a>. The visited elements are instances
 *       of the classes which derive from Module.
 */
class ImportanceFunction
{
	/// Can this instance be used for simulations?
	bool readyForSimulations;

public:

	ImportanceFunction() : readyForSimulations(false) {}

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
	inline bool ready() const noexcept { return readyForSimulations; }

	/**
	 * @brief Assess the importance of the states on this \ref ModuleInstance
	 *        "module", according to the given \ref Property "logical property"
	 *
	 * @param mod  Module whose reachable states will have their importance assessed
	 * @param prop Property guiding the function for the importance assessment
	 *
	 * @return Reference to an instance of a base class of the
	 *         ImportanceFunction family class,
	 *         which can be used during simulations
	 *
	 * @note After a successfull invocation the ImportanceFunction
	 *       is ready() to be used during simulations
	 */
	virtual ImportanceFunction&
	assess_importance(ModuleInstance* mod, Property* const prop) = 0;

	/**
	 * @brief Assess the importance of the reachable states of the whole
	 *        \ref ModuleNetwork "system model", according to the given
	 *        \ref Property "logical property"
	 *
	 * @param net  System model (or coupled network of modules)
	 * @param prop Property guiding the function for the importance assessment
	 *
	 * @return Reference to an instance of a base class of the
	 *         ImportanceFunction family class,
	 *         which can be used during simulations
	 *
	 * @note After a successfull invocation the ImportanceFunction
	 *       is ready() to be used during simulations
	 */
	virtual ImportanceFunction&
	assess_importance(ModuleNetwork* net, Property* const prop) = 0;

	/// Release any memory allocated in the heap
	/// @note After this invocation the ImportanceFunction is no longer ready()
	virtual void clear() = 0;
};

} // namespace fig

#endif // IMPORTANCEFUNCTION_H

