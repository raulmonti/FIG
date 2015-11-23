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

#include <core_typedefs.h>
#include <State.h>
#include <ImportanceFunction.h>


namespace fig
{

/**
 * @brief Abstract importance function for concrete importance assessment
 *
 *        The assessment is "concrete" because we build and mantain an
 *        internal array with the importance of each reachable concrete state.
 *        This can be extremely heavy on memory: precisely the size of the
 *        concrete state space of the assessed element (ModuleInstance or
 *        ModuleNetwork)
 *
 * @see ImportanceFunction
 */
class ImportanceFunctionConcrete : public ImportanceFunction
{
	ImportanceValue* statesImportance;

protected:
	/// @todo TODO implement algorithm from sheet,
	///       the one with 'state[]' and 'redges[]' arrays.
	///       Attribute "statesImportance" would play the role of 'state[]'
	virtual void assess_importance(State& s);
};

} // namespace fig

#endif // IMPORTANCEFUNCTIONCONCRETE_H

