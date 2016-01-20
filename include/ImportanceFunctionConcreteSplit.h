//==============================================================================
//
//  ImportanceFunctionConcreteSplit.h
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


#ifndef IMPORTANCEFUNCTIONCONCRETESPLIT_H
#define IMPORTANCEFUNCTIONCONCRETESPLIT_H

#include <FigException.h>
#include <ImportanceFunctionConcrete.h>
#include <ModuleInstance.h>
#include <ModuleNetwork.h>


namespace fig
{

/**
 * @brief Importance function for the concrete importance assessment
 *        of a single ModuleInstance.
 *
 *        Assesses the importance of the concrete state space of a single
 *        module, viz the "split" view of the user model.
 *        This requires a ModuleInstance with its \ref State "symbolic state",
 *        namely an array with its \ref Variable "variables", independently
 *        of the states of the other modules.
 *
 * @see ImportanceFunction
 * @see ImportanceFunctionConcrete
 * @see ImportanceFunctionConcreteCoupled
 *
 * @todo Implement, test, get results and publish !!!
 */
class ImportanceFunctionConcreteSplit : public ImportanceFunctionConcrete
{
	// Make overloads explicit, otherwise Clang whines like a whore
	using ImportanceFunction::assess_importance;
	using ImportanceFunctionConcrete::assess_importance;

public:

	virtual void assess_importance(const ModuleInstance& mod,
								   const Property& prop,
								   const std::string& strategy = "")
		{
			throw_FigException("TODO");
		}

	virtual void assess_importance(const ModuleNetwork& net,
								   const Property& prop,
								   const std::string& strategy = "")
		{
			for (auto& mod: net.modules)  // Visit each module individually
				assess_importance(*mod, prop, strategy);
		}
};

} // namespace fig

#endif // IMPORTANCEFUNCTIONCONCRETESPLIT_H

