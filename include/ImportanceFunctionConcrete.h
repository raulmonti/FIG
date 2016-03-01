//==============================================================================
//
//  ImportanceFunctionConcrete.h
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


#ifndef IMPORTANCEFUNCTIONCONCRETE_H
#define IMPORTANCEFUNCTIONCONCRETE_H

// C+
#include <vector>
#include <string>
// FIG
#include <core_typedefs.h>
#include <ImportanceFunction.h>
#include <State.h>


namespace fig
{

class Module;
class Property;
class Transition;

/**
 * @brief Abstract ImportanceFunction for concrete importance assessment
 *
 *        The assessment is "concrete" because internal vectors are built and
 *        mantained with the ImportanceValue of each reachable concrete state,
 *        viz. importance information for the "concrete state space" is kept.<br>
 *        This can be extremely heavy on memory: precisely the size of the
 *        concrete state space of the assessed elements (ModuleInstance or
 *        ModuleNetwork). On the other hand it can be considerably more CPU
 *        efficient than on-the-fly importance assessment as
 *        \ref ImportanceFunctionAlgebraic "algebraic importance functions" do.
 *
 * @see ImportanceFunction
 * @see ImportanceFunctionAlgebraic
 */
class ImportanceFunctionConcrete : public ImportanceFunction
{
protected:  // Attributes

	/// Concrete importance assessment for all the modules in the system model
	std::vector< ImportanceVec > modulesConcreteImportance;

	/// Copy of the global state of the \ref ModuleNetwork "model"
	mutable State< STATE_INTERNAL_TYPE > globalStateCopy;

public:  // Ctor/Dtor

	/// Data ctor
	ImportanceFunctionConcrete(const std::string& name,
							   const State< STATE_INTERNAL_TYPE >& globalState);

	/// Dtor
	virtual ~ImportanceFunctionConcrete();

public:  // Accessors

	inline virtual bool concrete() const noexcept { return true; }

	/**
	 * Retrieve all pre-computed information about the given StateInstance.
	 * This includes the state importance and some event masks.
	 * @return ImportanceValue possibly mixed with Event information
	 * \ifnot NDEBUG
	 *   @throw FigException if there's no \ref has_importance_info()
	 *                       "importance information" currently
	 * \endif
	 * @see assess_importance()
	 * @see importance_of()
	 */
	virtual ImportanceValue info_of(const StateInstance& state) const = 0;

public:  // Utils

	/**
	 * @brief Assess the importance of the reachable concrete states of the
	 *        whole \ref ModuleNetwork "system model", according to the
	 *        \ref Property "logical property" and strategy specified.
	 *
	 *        Any \ref has_importance_info() "importance information" previously
	 *        computed is discarded. After a successfull invocation the
	 *        ImportanceFunction holds internally the importance corresponding
	 *        to the Property and assessment strategy given.
	 * 
	 * @param prop     Property identifying the special states
	 * @param strategy Importance assessment strategy, currently "flat" or "auto"
	 *
	 * @note To use the "adhoc" importance assessment strategy
	 *       call the other assess_importance() member function
	 * 
	 * @throw bad_alloc if system's memory wasn't enough for internal storage
	 *
	 * @see has_importance_info()
	 */
	virtual void assess_importance(const Property& prop,
								   const std::string& strategy = "") = 0;

	/**
	 * @brief Assess the importance of the reachable concrete states of the
	 *        whole \ref ModuleNetwork "system model", according to the
	 *        \ref Property "logical property" and using an ad hoc
	 *        importance assessment strategy.
	 *
	 *        Any \ref has_importance_info() "importance information" previously
	 *        computed is discarded. After a successfull invocation the
	 *        ImportanceFunction holds internally the importance corresponding
	 *        to the passed ad hoc importance assessment function.
	 *
	 * @param prop     Property identifying the special states
	 * @param formulaExprStr  Mathematical formula to assess the states'
	 *                        importance, expressed as a string
	 * @param varnames Names of variables ocurring in 'formulaExprStr',
	 *                 i.e. which substrings in the formula expression
	 *                 are actually variable names.
	 * 
	 * @note To use other importance assessment strategies (e.g. "flat")
	 *       call the other assess_importance() member function
	 *
	 * @throw FigException if badly formatted 'formulaExprStr' or 'varnames'
	 *                     has names not appearing in 'formulaExprStr'
	 * @throw bad_alloc if system's memory wasn't enough for internal storage
	 *
	 * @see has_importance_info()
	 */
	virtual void assess_importance(const Property& prop,
								   const std::string& formulaExprStr,
								   const std::vector<std::string>& varnames) = 0;

	/// Erase all internal importance information (free resources along the way)
	virtual void clear() noexcept;

protected:  // Utils for derived classes

	/**
	 * @brief Populate an internal importance vector
	 *
	 *        For the given "property" and following the "strategy" specified,
	 *        analyse the state space of "module" and compute the importance
	 *        of all its concrete states. The resulting information will be
	 *        stored internally at position "index".
	 *
	 * @param module   Module whose concrete states' importance will be assessed
	 * @param property Logical property identifying the special states
	 * @param strategy Importance assessment strategy to follow
	 * @param index    Internal location where resulting info will be kept
	 *
	 * @note This allocates (tons of) memory internally
	 * @note To assess again for same index with different strategy or property,
	 *       release first the internal info through clear(const unsigned&)
	 *
	 * @warning The values of the internal inherited attributes minImportance_,
	 *          maxImportance_ and minRareImportance_ are updated.
	 *
	 * @throw bad_alloc    if system's memory wasn't enough for internal storage
	 * @throw FigException if there's already importance info for this index
	 */
	void assess_importance(const Module& module,
						   const Property& property,
						   const std::string& strategy,
						   const unsigned& index = 0);
};

} // namespace fig

#endif // IMPORTANCEFUNCTIONCONCRETE_H

