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
	const ImportanceValue IMP_RANGE = impFun.max_value() - impFun.min_value();
	if (IMP_RANGE < static_cast<ImportanceValue>(2u)) {
		// Too few importance levels: default to max possible # of thresholds
		std::vector< ImportanceValue > thresholds(IMP_RANGE);
		thresholds.push_back(1u);
		thresholds[0u] = static_cast<ImportanceValue>(0u);
		return thresholds;
	}

	// What follows is clearly arbitrary but then we warned the user
	// in the class' docstring, didn't we?
	const unsigned STRIDE(splitsPerThreshold <  5 ? 2 :
						  splitsPerThreshold <  9 ? 3 :
						  splitsPerThreshold < 14 ? 4 : 5);

	ModelSuite::tech_log("Building thresholds with \""+ name +"\" for 1 out of "
						 "every " + std::to_string(STRIDE) + " importance value"
						 + (STRIDE > 1 ? ("s.\n") : (".\n")));

	// Start slightly above impFun's initial value to avoid oversampling
	const unsigned MARGIN =
		std::min(static_cast<unsigned>(impFun.max_value()),
				 std::max(2u, (impFun.max_value()-impFun.initial_value()) / 5u));
	const unsigned RANGE = impFun.max_value() - impFun.initial_value() - MARGIN;
#ifndef NDEBUG
	const unsigned NUMT = std::floor(static_cast<float>(RANGE) / STRIDE);
#endif

	std::stringstream msg;
	msg << "ImportanceValue of the chosen thresholds:";
	for (unsigned  i = MARGIN+STRIDE ; i <= MARGIN+RANGE ; i += STRIDE)
		msg << " " << i;
	ModelSuite::tech_log(msg.str() + "\n");

	ImportanceVec thresholds;
	build_thresholds(impFun, thresholds, MARGIN, STRIDE);

	assert(thresholds[impFun.min_value()] == static_cast<ImportanceValue>(0u));
	assert(thresholds[impFun.initial_value()] == static_cast<ImportanceValue>(0u));
	assert(thresholds[impFun.max_value()] == static_cast<ImportanceValue>(NUMT));

	return thresholds;
}


void
ThresholdsBuilderFixed::build_thresholds(const ImportanceFunction& impFun,
										 ImportanceVec& thresholds,
										 const unsigned& margin,
										 const unsigned& stride)
{
	const size_t SIZE(impFun.max_value() - impFun.min_value() + 1u);
	if (thresholds.size() != SIZE)
		thresholds.resize(SIZE);

	// Thresholds building starts at the initial state's importance + margin,
	// everything from there downwards will be the zeroth level
	const ImportanceValue zero(0u);
	unsigned pos;
	for (pos = impFun.min_value() ; pos < impFun.initial_value()+margin ; pos++)
		thresholds[pos] = zero;
	unsigned s(0u);
	ImportanceValue current(zero);
	for (; pos <= impFun.max_value() ; pos++) {
		thresholds[pos] = current;
		if (++s >= stride) {
			current++;
			s = 0;
		}
	}
}

} // namespace fig
