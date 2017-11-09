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
#include <ImportanceFunctionConcreteSplit.h>
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
    EXPAND_EVERY(expandEvery),
    globEff_(2ul),
    postPro_()
{ /* Not much to do around here */ }


void
ThresholdsBuilderFixed::setup(const PostProcessing &pp,
                              std::shared_ptr<const Property>,
                              const unsigned ge)
{
	globEff_ = ge;
	postPro_ = pp;
}


ThresholdsVec
ThresholdsBuilderFixed::build_thresholds(const ImportanceFunction& impFun)
{
	ImportanceVec thresholds;
	const ImportanceValue IMP_RANGE(impFun.max_value()-impFun.initial_value());

	figTechLog << "Building thresholds with \"" << name << "\" ";

	if (globEff_ < 2u) {
		// For flat importance function we need a dummy thresholds vector
		ImportanceVec({impFun.initial_value(),impFun.max_value()+1}).swap(thresholds);
		goto consistency_check;

	} else if (IMP_RANGE < MIN_IMP_RANGE) {
		stride_ = 1u;
		figTechLog << "using all importance values as thresholds.\n";
		thresholds.resize(2+impFun.max_value()-impFun.min_value());
		ImportanceVec thresholds(2+impFun.max_value()-impFun.initial_value());
		std::iota(begin(thresholds), end(thresholds), impFun.initial_value());

	} else {
		stride_ = choose_stride(IMP_RANGE);
		figTechLog << "for 1 out of every " << stride_ << " importance value"
				   << (stride_ > 1 ? ("s.\n") : (".\n"));
		thresholds.push_back(impFun.initial_value());
		// Start above initial importance value? May reduce oversampling
		const unsigned margin(IMP_RANGE>>3);
		build_thresholds(impFun, margin, stride_, thresholds);
	}

	show_thresholds(thresholds);

consistency_check:
	assert(!thresholds.empty());
	assert(thresholds[0] == impFun.initial_value());
	assert(thresholds.back() == 1 + impFun.max_value());

	// Build ThresholdsVec to return
	ThresholdsVec result;
	result.reserve(thresholds.size());
	for (auto imp: thresholds)
		result.emplace_back(imp, globEff_);
	return result;
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


ImportanceValue
ThresholdsBuilderFixed::choose_stride(const size_t& impRange) const
{
	ImportanceValue basicStride(1u), expansionFactor(1u);
	assert(globEff_ > 1u);
	if (impRange < MIN_IMP_RANGE)
		return basicStride;  // Don't even bother

	// What follows is clearly arbitrary, but then we warned the user
	// in the class' docstring, didn't we?
	switch (postPro_.type)
	{
	case (PostProcessing::NONE):
	case (PostProcessing::SHIFT):
		basicStride = globEff_ <  5u ? 2u :      // 2,3,4 ----------> 2
		              globEff_ <  9u ? 3u :      // 5,6,7,8 --------> 3
		              globEff_ < 14u ? 4u : 5u;  // 9,10,11,12,13 --> 4
		expansionFactor = std::ceil(static_cast<float>(impRange) / EXPAND_EVERY);
		// Make sure return type can represent the computed stride
		assert(std::log2(static_cast<float>(basicStride)*expansionFactor)
				< sizeof(decltype(basicStride))*8.0f);
		return basicStride * expansionFactor;
		break;

	case (PostProcessing::EXP):
		basicStride = globEff_ <  5u ? 1u :      // 2,3,4 ------> 1
		              globEff_ <  9u ? 2u : 3u;  // 5,6,7,8 ----> 2
		expansionFactor = std::ceil(std::log(impRange) / EXPAND_EVERY);
		// Make sure return type can represent the computed stride
		assert(basicStride*expansionFactor < sizeof(decltype(basicStride))*8u);
		return std::pow(postPro_.value, basicStride*expansionFactor);
		break;

	default:
		throw_FigException("invalid post-processing \"" + postPro_.name
		                  + "\" (" + std::to_string(postPro_.type) + ")");
		break;
	}
}


void
ThresholdsBuilderFixed::build_thresholds(const ImportanceFunction& impFun,
										 const unsigned& margin,
										 const unsigned& stride,
										 ImportanceVec& thresholds)
{
	assert(impFun.max_value() > impFun.initial_value() + margin);

	const size_t IMP_RANGE(impFun.max_value() - impFun.initial_value() - margin),
	             FIRST_THR(impFun.initial_value() + margin);

	// Importance function concrete split (aka compositional ifun)
	// using '*' as composition operator has the same effect than
	// an exponentiation post-processing; this patch fixes that issue:
	if (impFun.name() == "concrete_split" &&
		ImportanceFunctionConcreteSplit::CompositionType::PRODUCT ==
			static_cast<const ImportanceFunctionConcreteSplit&>(impFun).composition_type())
		postPro_.type = PostProcessing::EXP;

	switch (postPro_.type)
	{
	case (PostProcessing::NONE):
	case (PostProcessing::SHIFT):
		{ const size_t SIZE(IMP_RANGE/stride+2ul), OLD_SIZE(thresholds.size());
		thresholds.resize(OLD_SIZE+SIZE);
		for (size_t i = OLD_SIZE, imp = FIRST_THR;
			 i < thresholds.size() ;
			 imp += stride, i++)
			thresholds[i] = static_cast<ImportanceValue>(imp);
		}; break;

	case (PostProcessing::EXP):
		{ size_t expStride(1ul);
		for (ImportanceValue imp = FIRST_THR;
			 imp < impFun.max_value();
			 expStride *= stride, imp += expStride)
			thresholds.push_back(imp);
		thresholds.push_back(impFun.max_value()+1u);
		}; break;

	default:
		throw_FigException("invalid post-processing \"" + postPro_.name
		                  + "\" (" + std::to_string(postPro_.type) + ")");
		break;
	};

	// Enforce consistency (remember thresholds may contain previous data)
	std::sort(begin(thresholds), end(thresholds));
	auto newEnd = std::unique(begin(thresholds), end(thresholds));
	thresholds.erase(newEnd, end(thresholds));
	thresholds.shrink_to_fit();
	assert(thresholds.back() > impFun.max_value());
	thresholds.back() = impFun.max_value() + static_cast<ImportanceValue>(1u);
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
