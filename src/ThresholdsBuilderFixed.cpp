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
#include <numeric>  // std::iota()
#include <iterator>
// FIG
#include <ThresholdsBuilderFixed.h>
#include <ImportanceFunction.h>
#include <FigLog.h>

// ADL
using std::begin;
using std::end;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ImportanceVec
ThresholdsBuilderFixed::build_thresholds(const unsigned& splitsPerThreshold,
										 const ImportanceFunction& impFun,
										 const PostProcessing& postProcessing)
{
	const ImportanceValue IMP_RANGE(impFun.max_value()-impFun.initial_value());

	figTechLog << "Building thresholds with \"" << name << "\" ";

	if (IMP_RANGE < minImpRange_) {
		figTechLog << "using all importance values as thresholds.\n";
		ImportanceVec thresholds(1+impFun.max_value()-impFun.min_value());
		std::iota(begin(thresholds), end(thresholds), impFun.min_value());
		return thresholds;
	}

	// What follows is clearly arbitrary but then we warned the user
	// in the class' docstring, didn't we?
	const unsigned EXPANSION(std::ceil(IMP_RANGE/((float)expandEvery_))),
				   STRIDE(choose_stride(splitsPerThreshold) * EXPANSION);

	figTechLog << "for 1 out of every " << STRIDE << " importance value"
			   << (STRIDE > 1 ? ("s.\n") : (".\n"));

	// Start slightly above impFun's initial value to avoid oversampling
	const unsigned MARGIN =
		std::min(static_cast<unsigned>(impFun.max_value()),
				 std::max(2u, (impFun.max_value()-impFun.initial_value()) / 5u));
	const unsigned RANGE = impFun.max_value() - impFun.initial_value() - MARGIN;

	/// @todo TODO erase old code
// #ifndef NDEBUG
// 	const unsigned NUMT = std::floor(static_cast<float>(RANGE) / STRIDE);
// #endif
//
//	std::stringstream msg;
//	msg << "ImportanceValue of the chosen thresholds:";
//	if (IMP_RANGE < MIN_IMP_RANGE)
//		for (ImportanceValue i = impFun.min_value()+1u ;
//		     i <= impFun.max_value() ;
//		     i++)
//			msg << " " << i;
//	else
//		for (unsigned i = MARGIN+STRIDE ; i <= MARGIN+RANGE ; i += STRIDE)
//			msg << " " << i;
//	ModelSuite::tech_log(msg.str() + "\n");

	ImportanceVec thresholds;
	postProcessing_ = postProcessing;
	build_thresholds(impFun, thresholds, MARGIN, STRIDE);
	show_thresholds(thresholds);
	assert(!thresholds.empty());
	assert(thresholds[0] == impFun.initial_value());
	assert(thresholds.back() > impFun.max_value());

	/// @todo TODO erase old code
//	assert(static_cast<ImportanceValue>(0u)   == thresholds[impFun.min_value()]);
//	assert(static_cast<ImportanceValue>(0u)   == thresholds[impFun.initial_value()]);
//	assert(static_cast<ImportanceValue>(NUMT) == thresholds[impFun.max_value()]);

	return thresholds;
}


unsigned
ThresholdsBuilderFixed::choose_stride(const unsigned& splitsPerThreshold) const
{
	/// @todo TODO Tune for arithmetic/geometric threshold building duality

	assert(splitsPerThreshold > 1u);
	// What follows is clearly arbitrary but then we warned the user
	// in the class' docstring, didn't we?     splitting        stride
	return splitsPerThreshold <  5 ? 2u :      // 2,3,4 ----------> 2
		   splitsPerThreshold <  9 ? 3u :      // 5,6,7,8 --------> 3
		   splitsPerThreshold < 14 ? 4u : 5u;  // 9,10,11,12,13 --> 4
											   // else -----------> 5
}


unsigned
ThresholdsBuilderFixed::choose_stride(const unsigned& splitsPerThreshold,
									  const unsigned& impRange) const
{
	assert(splitsPerThreshold > 1u);
	assert(impRange >= minImpRange_);
	const unsigned BASIC_STRIDE(choose_stride(splitsPerThreshold)),
				   EXPANSION_FACTOR(std::ceil(impRange/((float)expandEvery_)));

	/// @todo TODO Finish, considering arithmetic/geometric threshold building duality
}


void
ThresholdsBuilderFixed::build_thresholds(const ImportanceFunction& impFun,
										 ImportanceVec& thresholds,
										 const unsigned& MARGIN,
										 const unsigned& STRIDE)
{

	/// @fixme TODO consider the value of postProcessing_ to determine how
	///             the stride should be applied (arithmetically vs geometrically)

	const size_t SIZE(impFun.max_value() - impFun.min_value() + 1u);
	thresholds.resize(SIZE);

	if (SIZE-1u < MIN_IMP_RANGE) {
		// Too few values: everything above the base will be a threshold
		ImportanceValue imp(0u);
		unsigned pos;
		for (pos = impFun.min_value() ; pos <= impFun.max_value() ; pos++)
			thresholds[pos] = imp++;
		return;
	}

	// Thresholds building starts at the initial state's importance + margin,
	// everything from there downwards will be the zeroth level
	const ImportanceValue zero(0u);
	size_t pos;
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

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
