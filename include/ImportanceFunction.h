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

class ThresholdsBuilder;
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

	/// Do we hold importance information from last assessment?
	bool hasImportanceInfo_;

	/// Can this instance be used for simulations?
	bool readyForSims_;

	/// Last used strategy to assess the importance with this function
	std::string strategy_;

	/// Last used technique to build the importance thresholds in this function
	std::string thresholdsTechnique_;

	/// Maximum importance assigned during the last assessment
	ImportanceValue maxImportance_;

	/// Importance of the rare state with lowest importance from last assessment
	ImportanceValue minRareImportance_;

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
	 * @copydoc hasImportanceInfo_
	 *
	 *  This starts out false and becomes true after a successfull call
	 *  to either one of the importance assessment functions.
	 *  It becomes false again after a call to clear()
	 *
	 * @see assess_importance(const ModuleInstance&, const Property&, const std::string&)
	 * @see assess_importance(const ModuleNetwork&, const Property&, const std::string&)
	 */
	bool has_importance_info() const noexcept;

	/**
	 * @copydoc readyForSims_
	 *
	 *  This requires having had the \ref build_thresholds() "thresholds built"
	 *  in addition to holding \ref has_importance_info() "importance information"
	 *
	 * @see has_importance_info()
	 */
	bool ready() const noexcept;

	/// @copydoc strategy_
	/// @returns Empty string if function doesn't has_importance_info(),
	///          last used strategy otherwise
	const std::string strategy() const noexcept;

	/// @copydoc thresholdsTechnique_
	/// @returns Empty string if function isn't ready(),
	///          last thresholds building technique used otherwise
	const std::string thresholds_technique() const noexcept;

	/// @copydoc maxImportance_
	/// @returns Zero if function doesn't has_importance_info(),
	///          last maximum assessed importance otherwise
	ImportanceValue max_importance() const noexcept;

	/// @copydoc minRareImportance_
	/// @returns Zero if function doesn't has_importance_info(),
	///          minimum ImportanceValue of a rare state otherwise
	ImportanceValue min_rare_importance() const noexcept;

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
	 * @param force    Whether to force the computation, even if this
	 *                 ImportanceFunction already has importance information
	 *                 for the specified assessment strategy.
	 *
	 * @note After a successfull invocation the ImportanceFunction holds
	 *       internally the computed \ref has_importance_info()
	 *       "importance information" for the passed assessment strategy.
	 *
	 * @see assess_importance(const ModuleNetwork&, const Property&, const std::string&)
	 * @see has_importance_info()
	 */
	virtual void assess_importance(const ModuleInstance& mod,
								   const Property& prop,
								   const std::string& strategy = "",
								   bool force = false) = 0;

	/**
	 * @brief Assess the importance of the reachable states of the whole
	 *        \ref ModuleNetwork "system model", according to the
	 *        \ref Property "logical property" and strategy specified.
	 *
	 * @param net      System model (or coupled network of modules)
	 *                 Its current state is taken as the model's initial state.
	 * @param prop     Property guiding the importance assessment
	 * @param strategy Strategy of the assessment (flat, auto, ad hoc...)
	 * @param force    Whether to force the computation, even if this
	 *                 ImportanceFunction already has importance information
	 *                 for the specified assessment strategy.
	 *
	 * @note After a successfull invocation the ImportanceFunction holds
	 *       internally the computed \ref has_importance_info()
	 *       "importance information" for the passed assessment strategy.
	 *
	 * @see assess_importance(const ModuleInstance&, const Property&, const std::string&)
	 * @see has_importance_info()
	 */
	virtual void assess_importance(const ModuleNetwork& net,
								   const Property& prop,
								   const std::string& strategy = "",
								   bool force = false) = 0;

	/**
	 * @brief Build thresholds from precomputed importance information
	 *
	 *        The thresholds are built in the ImportanceFunction itself,
	 *        smashing the finely grained importance values and replacing them
	 *        with coarsely grained threshold levels.
	 *        After a successfull call this ImportanceFunction instance is
	 *        \ref ImportanceFunction::ready() "ready for simulations".
	 *
	 * @param tb  ThresholdsBuilder to use
	 * @param splitsPerThreshold 1 + Number of simulation-run-replicas upon a
	 *                           "threshold level up" event
	 *
	 * @throw FigException if there was no precomputed \ref has_importance_info()
	 *                     "importance information"
	 *
	 * @see ready()
	 */
	virtual void build_thresholds(ThresholdsBuilder& tb,
								  const unsigned& splitsPerThreshold) = 0;

	/// @brief Tell the pre-computed importance of the given StateInstance
	/// @note This instance should hold \ref has_importance_info()
	///       "importance information"
	/// @see assess_importance()
	virtual ImportanceValue importance_of(const StateInstance& state) const = 0;

	/// Release any memory allocated in the heap
	/// @note After this invocation the ImportanceFunction doesn't hold
	///       \ref has_importance_info() "importance information" any longer
	///       and it's thus not \ref ready() "ready for simulations"
	virtual void clear() noexcept = 0;
};

} // namespace fig

#endif // IMPORTANCEFUNCTION_H

