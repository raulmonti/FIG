//==============================================================================
//
//  Module.h
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


#ifndef MODULE_H
#define MODULE_H

#include <ImportanceFunction.h>
#include <Property.h>


namespace fig
{

/**
 * @brief Abstract base module class
 *
 *        The system model described by the user is implemented as a
 *        ModuleNetwork, composed of ModuleInstance objects.
 *
 * @note The accept member function implements the
 *       <a href="https://sourcemaking.com/design_patterns/visitor">
 *       visitor design pattern</a>, where the visitor is the
 *       ImportanceFunction and the visited elements are instances
 *       of the classes which derive from Module.
 */
class Module
{

public:  // Utils

	/// Have the importance of our states assessed by this ImportanceFunction
	/// according to the given Property
	virtual void accept(ImportanceFunction& ifun, const Property& prop) = 0;
};

} // namespace fig

#endif // MODULE_H

