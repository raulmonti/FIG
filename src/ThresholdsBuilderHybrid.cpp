//==============================================================================
//
//  ThresholdsBuilderHybrid.cpp
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
#include <cassert>
// C++
#include <vector>
#include <sstream>
// FIG
#include <ThresholdsBuilderHybrid.h>
#include <FigException.h>
#include <ModelSuite.h>


namespace fig
{

std::vector< ImportanceValue >
ThresholdsBuilderHybrid::build_thresholds(const unsigned &splitsPerThreshold,
										  const ImportanceFunction &impFun)
{
	ImportanceVec result;
	size_t NUMT;

	try {
		// Start out using an adaptive technique, which may just work btw
		result = ThresholdsBuilderSMC::build_thresholds(splitsPerThreshold,
														impFun);
		NUMT = result.back();  // it worked!

	} catch (FigException&) {

		// Adaptive algorithm couldn't finish but achievements are
		// still stored in the internal vector 'thresholds_'
		const unsigned LAST_THR(thresholds_.back()),
					   MARGIN(LAST_THR - impFun.initial_value()),
					   STRIDE(splitsPerThreshold < 5u ? 1u : 2u);
		ModelSuite::tech_log("Resorting to fixed choice of thresholds "
							 "starting above the ImportanceValue " +
							 std::to_string(LAST_THR) + "\n");

		// Choose fixed thresholds above LAST_THR
		ThresholdsBuilderFixed::build_thresholds(impFun, result, MARGIN, STRIDE);
		NUMT = std::floor(static_cast<float>(impFun.max_value()-LAST_THR) / STRIDE);
		NUMT += thresholds_.size() - 1;

		// Format the first "adaptive section" of 'result'
		unsigned currThr(0u), imp(0u);
		do {
			result[imp] = static_cast<ImportanceValue>(currThr);
			while (currThr < thresholds_.size()-1 &&
				   imp >= thresholds_[currThr+1])
				currThr++;
		} while (static_cast<ImportanceValue>(0u) == result[++imp]);

		// Format the second "fixed section" of 'result'
		for (/* imp value from previous loop */ ; imp < result.size() ; imp++)
			result[imp] += currThr;

		std::stringstream msg;
		msg << "ImportanceValue of the chosen thresholds:";
		for (size_t i = 1ul ; i < thresholds_.size() ; i++)
			msg << " " << thresholds_[i];
		for (unsigned i = LAST_THR + STRIDE ; i < impFun.max_value() ; i += STRIDE)
			msg << " " << i;
		ModelSuite::tech_log(msg.str() + "\n");
	}
	ImportanceVec().swap(thresholds_);  // free mem

	assert(result[impFun.min_value()] == static_cast<ImportanceValue>(0u));
	assert(result[impFun.initial_value()] == static_cast<ImportanceValue>(0u));
	assert(result[impFun.max_value()] == static_cast<ImportanceValue>(NUMT));

	return result;
}

} // namespace fig
