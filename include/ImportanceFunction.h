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
public:
	virtual void assess_importance(ModuleInstance* mod) = 0;
	virtual void assess_importance(ModuleNetwork*  net) = 0;
};

} // namespace fig

#endif // IMPORTANCEFUNCTION_H

