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
#include <thread>
#include <functional>  // std::ref()
// FIG
#include <ThresholdsBuilderHybrid.h>
#include <FigException.h>
#include <ModelSuite.h>


namespace
{
/// Execution time, in minutes, granted to the adaptive technique.<br>
/// If computations don't finish within this limit, resort to a fixed technique
/// to choose the missing thresholds "instantaneously".
///
const size_t ADAPTIVE_TIMEOUT_MINUTES = 5;
}



namespace fig
{

std::vector< ImportanceValue >
ThresholdsBuilderHybrid::build_thresholds(const unsigned &splitsPerThreshold,
										  const ImportanceFunction &impFun)
{
	ImportanceVec result;
	size_t NUMT;

	// Impose an execution wall time limit...
	const std::chrono::minutes timeLimit(::ADAPTIVE_TIMEOUT_MINUTES);
	std::thread timer([] (bool& halt, const std::chrono::minutes& limit)
					  { std::this_thread::sleep_for(limit); halt=true; },
					  std::ref(halted_), std::ref(timeLimit));
	try {
		// Start out using an adaptive technique, which may just work btw
		halted_ = false;
		result = ThresholdsBuilderSMC::build_thresholds(splitsPerThreshold,
														impFun);
		NUMT = result.back();  // it worked!

	} catch (FigException&) {
		// Adaptive algorithm couldn't finish but achievements remain stored
		// in the internal vector 'thresholds_'

		if (thresholds_.back() <= impFun.initial_value()) {  // avoid 'lowest' threshold
			ImportanceValue iteration(0u);
			const ImportanceValue MIN_THR(impFun.initial_value()+1);
			for (auto& thr: thresholds_)
				thr = MIN_THR + (iteration++);
		}
		const size_t LAST_THR(thresholds_.back()),
					 MARGIN(LAST_THR - impFun.initial_value()),
					 IMP_RANGE(impFun.max_value() - impFun.min_value()),
					 EXPANSION_FACTOR(std::ceil(IMP_RANGE/((float)IMP_LEAP_SIZE))),
					 STRIDE((splitsPerThreshold < 5u ? 2u :
							 splitsPerThreshold < 9u ? 3u : 4u )*EXPANSION_FACTOR);

		ModelSuite::tech_log("\nResorting to fixed choice of thresholds "
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
			while (currThr < thresholds_.size()-1 && imp >= thresholds_[currThr+1])
				currThr++;
		} while (static_cast<ImportanceValue>(0u) == result[++imp]);

		// Format the second "fixed section" of 'result'
		for (/* imp value from previous loop */ ; imp < result.size() ; imp++)
			result[imp] += currThr;

		std::stringstream msg;
		msg << "ImportanceValue of the chosen thresholds:";
		for (size_t i = 1ul ; i < thresholds_.size() ; i++)
			msg << " " << thresholds_[i];
		for (size_t i = LAST_THR + STRIDE ; i < impFun.max_value() ; i += STRIDE)
			msg << " " << i;
		ModelSuite::tech_log(msg.str() + "\n");
	}

	// Tidy-up
	ImportanceVec().swap(thresholds_);
	timer.detach();

	assert(result[impFun.min_value()] == static_cast<ImportanceValue>(0u));
	assert(result[impFun.initial_value()] == static_cast<ImportanceValue>(0u));
	assert(result[impFun.max_value()] == static_cast<ImportanceValue>(NUMT));

	return result;
}

} // namespace fig
