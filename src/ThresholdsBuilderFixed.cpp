//==============================================================================
//
//  ThresholdsBuilderFixed.cpp
//
//  Copyleft 2016-
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


// C
#include <cmath>
// C++
#include <sstream>
// FIG
#include <ThresholdsBuilderFixed.h>
#include <ImportanceFunction.h>
#include <ModelSuite.h>


namespace fig
{

std::vector< ImportanceValue >
ThresholdsBuilderFixed::build_thresholds(const unsigned& splitsPerThreshold,
										 const ImportanceFunction& impFun)
{
	// What follows is as arbitrary as arbitrariness can be
	const unsigned JUMP(splitsPerThreshold <  4 ? 1 :
						splitsPerThreshold <  7 ? 2 :
						splitsPerThreshold < 11 ? 3 : 4);
	const unsigned NUMT(std::floor((impFun.max_value()-impFun.initial_value()) /
								   static_cast<float>(JUMP)));
	std::vector< ImportanceValue > thresholds(impFun.max_value()-impFun.min_value()+1);

	ModelSuite::tech_log("Building thresholds with \""+ name +"\" for 1 out of "
						 "every " + std::to_string(JUMP) + " importance value"
						 + (JUMP > 1 ? ("s.\n") : (".\n")));
	std::stringstream msg;
	msg << "ImportanceValue of the chosen thresholds:";
	for (size_t i = JUMP ; i < thresholds.size() ; i += JUMP)
		msg << " " << i;
	ModelSuite::tech_log(msg.str() + "\n");

	// Everything from the initial state downwards will be the zeroth level
	unsigned pos;
	for (pos = 0u ; pos < impFun.initial_value() ; pos++)
		thresholds[pos] = static_cast<ImportanceValue>(0u);
	// Thresholds building starts from the initial state's importance
	unsigned jump(0u);
	ImportanceValue current(0u);
	for (; pos < thresholds.size() ; pos++) {
		thresholds[pos] = current;
		if (++jump >= JUMP) {
			current++;
			jump = 0;
		}
	}

	/// @todo TODO erase debug print below
	for (size_t i = 0 ; i < thresholds.size() ; i++)
		std::cerr << "(" << i << ") --> [" << thresholds[i] << "]\n";
	std::cerr << thresholds[impFun.max_value()] << std::endl;
	std::cerr << NUMT << std::endl;
	///////////////////////////////////////

	assert(thresholds[impFun.min_value()] == static_cast<ImportanceValue>(0u));
	assert(thresholds[impFun.max_value()] == static_cast<ImportanceValue>(NUMT));

	return thresholds;
}

} // namespace fig
