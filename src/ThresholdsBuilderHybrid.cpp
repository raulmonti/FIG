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
#include <cmath>
#include <pthread.h>
// C++
#include <utility>
#include <sstream>
#include <thread>
#include <functional>  // std::ref()
// FIG
#include <ThresholdsBuilderHybrid.h>
#include <ImportanceFunction.h>
#include <FigException.h>
#include <FigLog.h>


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ImportanceVec
ThresholdsBuilderHybrid::build_thresholds(const unsigned& splitsPerThreshold,
										  const ImportanceFunction& impFun,
										  const PostProcessing& postProcessing)
{
	// Impose an execution wall time limit...
	const std::chrono::minutes TIMEOUT = ADAPTIVE_TIMEOUT;  // take by value!
	std::thread timer([] (bool& halt, const std::chrono::minutes& limit)
					  { std::this_thread::sleep_for(limit); halt=true; },
					  std::ref(halted_), TIMEOUT);
	try {
		// Start out using an adaptive technique, which may just work btw
		halted_ = false;
		ThresholdsBuilderAdaptive::build_thresholds(splitsPerThreshold, impFun,
													postProcessing);
		pthread_cancel(timer.native_handle());  // it worked!

	} catch (FigException&) {
		// Adaptive algorithm couldn't finish but achievements remain
		// stored in the vector member 'thresholds_'
		assert(thresholds_.back() >= impFun.initial_value());
		const size_t MARGIN(thresholds_.back());
		figTechLog << "\nResorting to fixed choice of thresholds starting "
				   << "above the ImportanceValue " << MARGIN << "\n";
		stride_ = choose_stride(impFun.max_value()-MARGIN, splitsPerThreshold,
								postProcessing);
		ThresholdsBuilderFixed::build_thresholds(impFun,
		                                         MARGIN-impFun.initial_value(),
		                                         stride_,
												 postProcessing,
		                                         thresholds_);
		halted_ = true;

	} catch (std::exception&) {
		timer.detach();
		throw;
	}

	// Tidy-up
	ImportanceVec thresholds;
	std::swap(thresholds, thresholds_);
	timer.detach();

	if (halted_)
		show_thresholds(thresholds);
	assert(!thresholds.empty());
	assert(thresholds[0] == impFun.initial_value());
	assert(thresholds.back() == 1 + impFun.max_value());

	return thresholds;
}


unsigned
ThresholdsBuilderHybrid::choose_stride(const size_t& impRange,
									   const unsigned& splitsPerThreshold,
									   const PostProcessing& postProcessing) const
{
	unsigned basicStride(1u), expansionFactor(1u);
	assert(splitsPerThreshold > 1u);
	if (impRange < MIN_IMP_RANGE)
		return basicStride;  // Don't even bother

	// What follows is clearly arbitrary, but then we warned the user
	// in the class' docstring, didn't we?
	switch (postProcessing.type)
	{
	case (PostProcessing::NONE):
	case (PostProcessing::SHIFT):
		basicStride = splitsPerThreshold <  4u ? 2u :      // 2,3 -------------> 2
					  splitsPerThreshold <  7u ? 3u :      // 4,5,6 -----------> 3
					  splitsPerThreshold < 11u ? 4u :      // 7,8,9,10 --------> 4
					  splitsPerThreshold < 16u ? 5u : 6u;  // 11,12,13,14,15 --> 5
		expansionFactor = std::ceil(static_cast<float>(impRange) / EXPAND_EVERY);
		// Make sure return type can represent the computed stride
		assert(std::log2(static_cast<float>(basicStride)*expansionFactor)
				< sizeof(decltype(basicStride))*8.0f);
		return basicStride * expansionFactor;
		break;

	case (PostProcessing::EXP):
		basicStride = splitsPerThreshold < 4u ? 1u :      // 2,3 ------> 1
					  splitsPerThreshold < 7u ? 2u : 3u;  // 4,5,6 ----> 2
		expansionFactor = std::ceil(std::log(impRange) / EXPAND_EVERY);
		// Make sure return type can represent the computed stride
		assert(basicStride*expansionFactor < sizeof(decltype(basicStride))*8u);
		return std::pow(postProcessing.value, basicStride*expansionFactor);
		break;

	default:
		throw_FigException("invalid post-processing specified (\""
						  + postProcessing.name + "\")");
		break;
	}
}


} // namespace fig  // // // // // // // // // // // // // // // // // // // //
