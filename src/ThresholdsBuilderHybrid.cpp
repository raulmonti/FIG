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
#include <pthread.h>
// C++
#include <utility>
#include <sstream>
#include <thread>
#include <functional>  // std::ref()
// FIG
#include <ThresholdsBuilderHybrid.h>
#include <FigException.h>
#include <FigLog.h>


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ImportanceVec
ThresholdsBuilderHybrid::build_thresholds(const unsigned &splitsPerThreshold,
										  const ImportanceFunction &impFun,
										  const std::string& postProcessing)
{
	ImportanceVec thresholds;
	size_t NUMT;

	// Impose an execution wall time limit...
	std::thread timer([] (bool& halt, const std::chrono::minutes& limit)
					  { std::this_thread::sleep_for(limit); halt=true; },
					  std::ref(halted_), ADAPTIVE_TIMEOUT);
	try {
		// Start out using an adaptive technique, which may just work btw
		halted_ = false;
		ThresholdsBuilderAdaptive::build_thresholds(splitsPerThreshold, impFun);
		std::swap(thresholds, thresholds_);  // it worked!

	} catch (FigException&) {
		// Adaptive algorithm couldn't finish but achievements remain
		// stored in the vector member 'thresholds_'
		std::swap(thresholds, thresholds_);

	/// @fixme TODO consider the value of postProcessing to determine how
	///             the stride should be applied (arithmetically vs geometrically)

		if (thresholds_.back() <= impFun.initial_value()) {  // avoid 'lowest' threshold
			ImportanceValue iteration(0u);
			const ImportanceValue MIN_THR(impFun.initial_value()+1);
			for (auto& thr: thresholds_)
				thr = MIN_THR + (iteration++);
		}
		const size_t LAST_THR(thresholds_.back()),
					 MARGIN(LAST_THR - impFun.initial_value()),
					 IMP_RANGE(impFun.max_value() - impFun.min_value()),
					 EXPANSION(std::ceil(IMP_RANGE/((float)IMP_LEAP_SIZE))),
					 STRIDE(choose_stride(splitsPerThreshold) * EXPANSION);

		figTechLog << "\nResorting to fixed choice of thresholds starting "
				   << "above the ImportanceValue " << LAST_THR << "\n";

		// Choose fixed thresholds above LAST_THR
		ThresholdsBuilderFixed::build_thresholds(impFun, thresholds, MARGIN, STRIDE);
		NUMT = std::floor(static_cast<float>(impFun.max_value()-LAST_THR) / STRIDE);
		NUMT += thresholds_.size() - 1;

//		// Format the first "adaptive section" of 'thresholds'
//		unsigned currThr(0u), imp(0u);
//		do {
//			thresholds[imp] = static_cast<ImportanceValue>(currThr);
//			while (currThr < thresholds_.size()-1 && imp >= thresholds_[currThr+1])
//				currThr++;
//		} while (static_cast<ImportanceValue>(0u) == thresholds[++imp]);
//
//		// Format the second "fixed section" of 'thresholds'
//		for (/* imp value from previous loop */ ; imp < thresholds.size() ; imp++)
//			thresholds[imp] += currThr;
//
//		std::stringstream msg;
//		msg << "ImportanceValue of the chosen thresholds:";
//		for (size_t i = 1ul ; i < thresholds_.size() ; i++)
//			msg << " " << thresholds_[i];
//		for (size_t i = LAST_THR + STRIDE ; i < impFun.max_value() ; i += STRIDE)
//			msg << " " << i;
//		ModelSuite::tech_log(msg.str() + "\n");
	}

	// Tidy-up
	ImportanceVec().swap(thresholds_);
	pthread_cancel(timer.native_handle());  /// @fixme BUG can this crash?
	timer.detach();

	show_thresholds(thresholds);
	assert(!thresholds.empty());
	assert(thresholds[0] == impFun.initial_value());
	assert(thresholds.back() > impFun.max_value());
	return thresholds;
}


unsigned
ThresholdsBuilderHybrid::choose_stride(const unsigned& splitsPerThreshold) const
{
	/// @todo TODO Tune for arithmetic/geometric threshold building duality

	assert(splitsPerThreshold > 1u);
	// What follows is clearly arbitrary but then we warned the user
	// in the class' docstring, didn't we?     splitting          stride
	return splitsPerThreshold <  4 ? 2u :      // 2,3 -------------> 2
		   splitsPerThreshold <  7 ? 3u :      // 4,5,6 -----------> 3
		   splitsPerThreshold < 11 ? 4u :      // 7,8,9,10 --------> 4
		   splitsPerThreshold < 16 ? 5u : 6u;  // 11,12,13,14,15 --> 5
											   // else ------------> 6
}


} // namespace fig  // // // // // // // // // // // // // // // // // // // //
