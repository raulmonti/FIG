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
#include <numeric>    // std::iota()
#include <iterator>
#include <algorithm>  // std::sort(), std::unique()
// FIG
#include <ThresholdsBuilderFixed.h>
#include <ImportanceFunction.h>
#include <FigLog.h>

// ADL
using std::begin;
using std::end;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ThresholdsBuilderFixed::ThresholdsBuilderFixed(ImportanceValue minImpRange,
											   ImportanceValue expandEvery) :
	ThresholdsBuilder("fix"),
	MIN_IMP_RANGE(minImpRange),
	EXPAND_EVERY(expandEvery)
{ /* Not much to do around here */ }


ImportanceVec
ThresholdsBuilderFixed::build_thresholds(const unsigned& splitsPerThreshold,
										 const ImportanceFunction& impFun,
										 const PostProcessing& postProcessing)
{
	ImportanceVec thresholds;
	const ImportanceValue IMP_RANGE(impFun.max_value()-impFun.initial_value());

	figTechLog << "Building thresholds with \"" << name << "\" ";

	if (IMP_RANGE < MIN_IMP_RANGE) {
		stride_ = 1u;
		figTechLog << "using all importance values as thresholds.\n";
		thresholds.resize(1+impFun.max_value()-impFun.min_value());
		ImportanceVec thresholds(1+impFun.max_value()-impFun.min_value());
		std::iota(begin(thresholds), end(thresholds), impFun.min_value());

	} else {
		stride_ = choose_stride(IMP_RANGE, splitsPerThreshold, postProcessing);
		figTechLog << "for 1 out of every " << stride_ << " importance value"
				   << (stride_ > 1 ? ("s.\n") : (".\n"));
		// Start slightly above impFun's initial value to avoid oversampling
		unsigned margin = std::min(impFun.max_value(), std::max(2u, IMP_RANGE/8u));
		build_thresholds(impFun, margin, stride_, postProcessing, thresholds);
	}

	show_thresholds(thresholds);
	assert(!thresholds.empty());
	assert(thresholds[0] == impFun.initial_value());
	assert(thresholds.back() > impFun.max_value());

	return thresholds;
}


const ImportanceValue&
ThresholdsBuilderFixed::stride() const noexcept
{
	return stride_;
}


const ImportanceValue&
ThresholdsBuilderFixed::min_imp_range() const noexcept
{
	return MIN_IMP_RANGE;
}


unsigned
ThresholdsBuilderFixed::choose_stride(const size_t& impRange,
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
		basicStride = splitsPerThreshold <  5u ? 2u :      // 2,3,4 ----------> 2
					  splitsPerThreshold <  9u ? 3u :      // 5,6,7,8 --------> 3
					  splitsPerThreshold < 14u ? 4u : 5u;  // 9,10,11,12,13 --> 4
		expansionFactor = std::ceil(static_cast<float>(impRange) / EXPAND_EVERY);
		// Make sure return type can represent the computed stride
		assert(std::log2(static_cast<float>(basicStride)*expansionFactor)
				< sizeof(decltype(basicStride))*8.0f);
		return basicStride * expansionFactor;
		break;

	case (PostProcessing::EXP):
		basicStride = splitsPerThreshold <  5u ? 1u :      // 2,3,4 ------> 1
					  splitsPerThreshold <  9u ? 2u : 3u;  // 5,6,7,8 ----> 2
		expansionFactor = std::ceil(std::log(impRange) / EXPAND_EVERY);
		// Make sure return type can represent the computed stride
		assert(basicStride*expansionFactor < sizeof(decltype(basicStride))*8u);
		return std::pow(postProcessing.value, basicStride*expansionFactor);
		break;

	default:
		throw_FigException("invalid post-processing specified ("
						  + std::to_string(postProcessing.type) + ")");
		break;
	}
}


void
ThresholdsBuilderFixed::build_thresholds(const ImportanceFunction& impFun,
										 const unsigned& margin,
										 const unsigned& stride,
										 const PostProcessing& postProcessing,
										 ImportanceVec& thresholds)
{
	assert(impFun.max_value() > impFun.initial_value() + margin);

	const size_t IMP_RANGE(impFun.max_value() - impFun.initial_value() - margin);

	switch (postProcessing.type)
	{
	case (PostProcessing::NONE):
	case (PostProcessing::SHIFT):
		{ const size_t SIZE(IMP_RANGE/stride+2ul), OLD_SIZE(thresholds.size());
		thresholds.resize(OLD_SIZE+SIZE);
		for (size_t i = OLD_SIZE, imp = impFun.initial_value() + margin ;
			 i < thresholds.size() ;
			 imp += stride, i++)
			thresholds[i] = static_cast<ImportanceValue>(imp);
		}; break;

	case (PostProcessing::EXP):
		for (ImportanceValue imp = impFun.initial_value() + margin;
			 imp < impFun.max_value();
			 imp *= stride)
			thresholds.push_back(imp);
		thresholds.push_back(impFun.max_value()+1u);
		break;

	default:
		throw_FigException("invalid post-processing specified (\""
						  + postProcessing.name + "\")");
		break;
	};

	// Enforce consistency (remember thresholds may contain previous data)
	std::sort(begin(thresholds), end(thresholds));
	auto newEnd = std::unique(begin(thresholds), end(thresholds));
	thresholds.erase(newEnd, end(thresholds));
	thresholds.shrink_to_fit();


	/// @todo TODO remove old code
//	const size_t SIZE(impFun.max_value() - impFun.min_value() + 1u);
//	thresholds.resize(SIZE);
//
//	if (SIZE-1u < MIN_IMP_RANGE) {
//		// Too few values: everything above the base will be a threshold
//		ImportanceValue imp(0u);
//		unsigned pos;
//		for (pos = impFun.min_value() ; pos <= impFun.max_value() ; pos++)
//			thresholds[pos] = imp++;
//		return;
//	}
//	// Thresholds building starts at the initial state's importance + margin,
//	// everything from there downwards will be the zeroth level
//	const ImportanceValue zero(0u);
//	size_t pos;
//	for (pos = impFun.min_value() ; pos < impFun.initial_value()+margin ; pos++)
//		thresholds[pos] = zero;
//	unsigned s(0u);
//	ImportanceValue current(zero);
//	for (; pos <= impFun.max_value() ; pos++) {
//		thresholds[pos] = current;
//		if (++s >= stride) {
//			current++;
//			s = 0;
//		}
//	}
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
