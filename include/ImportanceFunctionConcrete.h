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

class Property;
class Transition;

/**
 * @brief Abstract importance function for concrete importance assessment
 *
 *        The assessment is "concrete" because an internal vector is built and
 *        mantained with the ImportanceValue of each reachable concrete state,
 *        viz. importance information for the "concrete state space" is kept.<br>
 *        This can be extremely heavy on memory: precisely the size of the
 *        concrete state space of the assessed element (ModuleInstance or
 *        ModuleNetwork). On the other hand it can considerably more CPU
 *        efficient than on-the-fly importance assessment as
 *        \ref ImportanceFunctionAlgebraic "algebraic importance functions" do.
 *
 * @see ImportanceFunction
 * @see ImportanceFunctionAlgebraic
 */
class ImportanceFunctionConcrete : public ImportanceFunction
{
public:

	typedef std::vector< ImportanceValue > ImportanceVec;

protected:  // Attributes

	/// Concrete importance assessment for the whole system model
	std::vector< ImportanceVec > modulesConcreteImportance;

public:  // Ctor/Dtor

	/// Data ctor
	ImportanceFunctionConcrete(const std::string& name);

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
	inline virtual ImportanceValue info_of(const StateInstance& state) const = 0;

public:  // Utils

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

	/// Erase all internal importance information and free resources
	virtual void clear() noexcept;

	/// Erase internal importance information stored at position "index"
	virtual void clear(const unsigned& index) noexcept;

protected:  // Utils for derived classes

	/**
	 * @brief Populate an internal importance vector
	 *
	 *        For the given "property" and following the "strategy" specified,
	 *        analyse the symbolic "state" and compute the importance of the
	 *        corresponding concrete states. The resulting information will
	 *        be stored internally at position "index".
	 *
	 * @param symbState Vector of variables representing the state of a Module.
	 *                  Its current valuation is considered the initial state.
	 * @param trans     All the transitions of the Module, in any order
	 * @param property  Logical property identifying the special states
	 * @param strategy  Importance assessment strategy to follow
	 * @param index     Internal location where resulting info will be kept
	 *
	 * @note This allocates (tons of) memory internally
	 * @note To assess again for same index with different strategy or property,
	 *       release first the internal info through clear(const unsigned&)
	 *
	 * @throw bad_alloc    if system's memory wasn't enough for internal storage
	 * @throw FigException if there's already importance info for this index
	 */
	void assess_importance(const State<STATE_INTERNAL_TYPE>& symbState,
						   const std::vector<std::shared_ptr<Transition>>& trans,
						   const Property& property,
						   const std::string& strategy,
						   const unsigned& index = 0);
};

} // namespace fig

#endif // IMPORTANCEFUNCTIONCONCRETE_H

