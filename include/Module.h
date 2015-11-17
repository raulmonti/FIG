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

// C++
#include <memory>
#include <vector>
#include <string>
// FIG
#include <core_typedefs.h>  // CLOCK_INTERNAL_TYPE
#include <Label.h>
#include <Traial.h>


namespace fig
{

/**
 * @brief Abstract base module class, following the composite design pattern
 *
 *        The system model described by the user is implemented as a
 *        ModuleNetwork, composed of ModuleInstance objects.
 *        For more on the composite design patter visit
 *        <a href="https://sourcemaking.com/design_patterns/composite">
 *        this page</a>.
 */
class Module
{

public:  // Utils

	/**
	 * @brief Active module jump caused by expiration of our clock "clockName"
	 *
	 * @param clockName    Name of the clock (from this model!) which expires
	 * @param elapsedTime  Time lapse for the clock to expire
	 * @param traial       Instance of Traial to update
	 *
	 * @return Pointer to output label fired by the transition taken.
	 *         If none was enabled then 'tau' is returned.
	 *
	 * @note <b>Complexity:</b> <i>O(t+c+v)</i>, where
	 *       <ul>
	 *       <li> <i>t</i> is the number of transitions of this module,</li>
	 *       <li> <i>c</i> is the number of   clocks    of this module and</li>
	 *       <li> <i>v</i> is the number of  variables  of this module.</li>
	 *       </ul>
	 *
	 * @note Modifies sections both in State and clock-vector within "traial"
	 *       which correspond to variables and clocks from this module.
	 */
	virtual const Label& jump(const std::string& clockName,
							  const CLOCK_INTERNAL_TYPE& elapsedTime,
							  Traial& traial) = 0;

	/**
	 * @brief Passive module jump following "label" label
	 *
	 * @param label        Output label triggered by current active jump
	 * @param elapsedTime  Time lapse for the clock to expire
	 * @param traial       Instance of Traial to update
	 *
	 * @note <b>Complexity:</b> <i>O(t+c+v)</i>, where
	 *       <ul>
	 *       <li> <i>t</i> is the number of transitions of this module,</li>
	 *       <li> <i>c</i> is the number of   clocks    of this module and</li>
	 *       <li> <i>v</i> is the number of  variables  of this module.</li>
	 *       </ul>
	 *
	 * @note Modifies sections both in State and clock-vector within "traial"
	 *       which correspond to variables and clocks from this module.
	 */
	virtual void jump(const Label& label,
					  const CLOCK_INTERNAL_TYPE& elapsedTime,
					  Traial& traial) = 0;
};

} // namespace fig

#endif // MODULE_H

