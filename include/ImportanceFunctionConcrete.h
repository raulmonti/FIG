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

/**
 * @brief Abstract importance function for concrete importance assessment
 *
 *        The assessment is "concrete" because we build and mantain an
 *        internal vector with the importance of each reachable concrete state.
 *        This can be extremely heavy on memory: precisely the size of the
 *        concrete state space of the assessed element (ModuleInstance or
 *        ModuleNetwork)
 *
 * @see ImportanceFunction
 */
class ImportanceFunctionConcrete : public ImportanceFunction
{
protected:

	// Make overloads explicit, otherwise Clang whines like a whore
	using ImportanceFunction::assess_importance;

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

protected:  // Utils

	/**
	 * @brief Populate an internal importance vector
	 *
	 *        For the given "property" and following the "strategy" specified,
	 *        analyse the symbolic "state" and compute the importance of the
	 *        corresponding concrete states. The resulting information will
	 *        be stored internally at position "index".
	 *
	 * @param state    Vector of variables representing the state of a Module.
	 *                 Its current valuation is considered the initial state.
	 * @param property Logical property identifying the special states
	 * @param strategy Importance assessment strategy to follow
	 * @param index    Internal location where resulting info will be kept
	 *
	 * @note This allocates (tons of) memory internally
	 * @note To assess again for same index with different strategy or property,
	 *       release first the internal info through clear(const unsigned&)
	 *
	 * @throw bad_alloc    if system's memory wasn't enough for internal storage
	 * @throw FigException if there's already importance info for this index
	 */
	virtual void assess_importance(const State<STATE_INTERNAL_TYPE>& symbState,
								   const Property& property,
								   const std::string& strategy,
								   const unsigned& index = 0);

	/// Erase all internal importance information and free resources
	virtual void clear() noexcept;

	/// Erase any internal importance information stored at position "index"
	virtual void clear(const unsigned& index) noexcept;
};

} // namespace fig

#endif // IMPORTANCEFUNCTIONCONCRETE_H

