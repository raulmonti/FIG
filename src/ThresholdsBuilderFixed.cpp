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
#include <cstdlib>
#include <cstring>
// C++
#include <vector>
#include <sstream>
#include <utility>
#include <numeric>    // std::iota()
#include <iterator>
#include <algorithm>  // std::sort(), std::unique()
#include <exception>
// FIG
#include <ThresholdsBuilderFixed.h>
#include <ImportanceFunction.h>
#include <ImportanceFunctionConcreteSplit.h>
#include <FigLog.h>
#include <string_utils.h>

// ADL
using std::begin;
using std::end;

#ifndef strnlen
#  define strnlen(s,n) std::strlen(s)
#endif


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ThresholdsBuilderFixed::ThresholdsBuilderFixed(ImportanceValue minImpRange,
											   ImportanceValue expandEvery) :
	ThresholdsBuilder("fix"),
	MIN_IMP_RANGE(minImpRange),
    EXPAND_EVERY(expandEvery),
    globEff_(2ul),
    thrAdHoc_(""),
    postPro_(),
    stride_(0)
{ /* Not much to do around here */ }


void
ThresholdsBuilderFixed::setup(std::shared_ptr<const Property>, const void *info)
{
	int ge(0);
	try {
        // Does it make sense to interpret 'info' as an ad hoc list of thresholds?
        auto thrAdHoc = static_cast<const std::string*>(info);
        if (nullptr != thrAdHoc
                && !thrAdHoc->empty()
                && 666 > thrAdHoc->length())
            thrAdHoc_ = *thrAdHoc;
        else
            thrAdHoc_ = "";
	} catch (const std::bad_alloc&) {
		thrAdHoc_ = "";
	} catch (const std::length_error&) {
		thrAdHoc_ = "";
	}
    if (thrAdHoc_.empty()) {
        // couldn't cast to string? try casting to int
        auto globalEffort = static_cast<const int*>(info);
        ge = nullptr != globalEffort ? *globalEffort : 0;
    }
	// 'info' must've been either a valid string, or a valid global effort
	// NOTE: global effort '0' is allowed for the flat "importance function"
	if (thrAdHoc_.empty() && (ge < 0 || ge > static_cast<int>(MAX_EFFORT)))
		throw_FigException("cannot build thresholds with \"" + name +
		                   "\" from the information provided");
	globEff_ = static_cast<decltype(globEff_)>(ge);
}


ThresholdsVec
ThresholdsBuilderFixed::build_thresholds(std::shared_ptr<const ImportanceFunction> impFun)
{
	ThresholdsVec result;
	assert(nullptr != impFun);
	if (!thrAdHoc_.empty())
		result = build_thresholds_ad_hoc(*impFun);
	else
		result = build_thresholds_heuristically(*impFun);
	assert(!result.empty());
	assert(result.front().first == impFun->initial_value());
	assert(result.back().first > impFun->max_value());
	assert(result.front().second == static_cast<ImportanceValue>(1));
	assert(result.back().second == static_cast<ImportanceValue>(1));
	show_thresholds(result);
	return result;
}


const unsigned&
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
	unsigned basicStride(1u), expansionFactor(1u);
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
		return static_cast<ImportanceValue>(
		            std::pow(postPro_.value, basicStride*expansionFactor));
		break;

	default:
		throw_FigException("invalid post-processing \"" + postPro_.name
		                  + "\" (" + std::to_string(postPro_.type) + ")");
		break;
	}
}


ThresholdsVec
ThresholdsBuilderFixed::build_thresholds_ad_hoc(const ImportanceFunction& impFun)
{
	const auto MIN_THR = impFun.initial_value();
	const auto MAX_THR = impFun.max_value() + 1;

	figTechLog << "Building thresholds specified by the user; "
	           << "ignore global effort if set."
	           << std::endl;

	auto list = thrAdHoc_;
	delete_substring(list, "[");
	delete_substring(list, "]");
	delete_substring(list, "(");
	delete_substring(list, ")");
	auto pairStrVec = split(list, ',');

	ThresholdsVec thresholds;
	thresholds.reserve(pairStrVec.size()+2ul);
	thresholds.emplace_back(MIN_THR, 1u);
	for (const auto& pairStr: pairStrVec) {
		char* err(nullptr);
		// Read next threshold and its effort
		auto THR_EFF = split(pairStr, ':');
		if (THR_EFF.size() != 2)
			throw_FigException("invalid ad hoc threshold:splitting pair given: " + pairStr);
		const auto THR = std::strtol(THR_EFF[0].c_str(), &err, 10);
		if (nullptr != err && err[0] != '\0')
			throw_FigException("invalid threshold value \"" + THR_EFF[0] + "\"");
		const auto EFF = std::strtol(THR_EFF[1].c_str(), &err, 10);
		if (nullptr != err && err[0] != '\0')
			throw_FigException("invalid effort value \"" + THR_EFF[1] + "\"");
		// Check values consistency
		if (THR <= static_cast<long>(MIN_THR))
			throw_FigException("thresholds \"" + THR_EFF[0] + "\" is not greater "
			        "than the min importance " + std::to_string(MIN_THR));
		if (THR >= static_cast<long>(MAX_THR))
			throw_FigException("thresholds \"" + THR_EFF[0] + "\" is greater "
			        "than the max importance " + std::to_string(MAX_THR));
		if (EFF <= 0l || EFF > static_cast<long>(MAX_EFFORT))
			throw_FigException("out-of-bounds effort value \"" + THR_EFF[1] + "\"");
		// Store as (threshold,effort) pair
		thresholds.emplace_back(THR,EFF);
	}
	thresholds.emplace_back(MAX_THR, 1u);

	return thresholds;
}


ThresholdsVec
ThresholdsBuilderFixed::build_thresholds_heuristically(const ImportanceFunction& impFun)
{
	ImportanceVec thresholds;
	const ImportanceValue IMP_RANGE(impFun.max_value()-impFun.initial_value());
	postPro_ = impFun.post_processing();

	figTechLog << "Building thresholds heuristically according to "
	           << "the global effort and the importance function."
	           << std::endl;

	if (globEff_ < 2u) {
		// For flat importance function we need a dummy thresholds vector
		ImportanceVec({impFun.initial_value(),impFun.max_value()+1}).swap(thresholds);
		goto build_thresholds_vec;

	} else if (IMP_RANGE < MIN_IMP_RANGE) {
		stride_ = 1u;
		figTechLog << "using all importance values as thresholds.\n";
		thresholds.resize(2+impFun.max_value()-impFun.min_value());
		ImportanceVec thresholds(2+impFun.max_value()-impFun.initial_value());
		std::iota(begin(thresholds), end(thresholds), impFun.initial_value());

	} else {
		stride_ = static_cast<unsigned>(choose_stride(IMP_RANGE));
		figTechLog << "for 1 out of every " << stride_ << " importance value"
		           << (stride_ > 1 ? ("s.\n") : (".\n"));
		thresholds.emplace_back(impFun.initial_value());
		// Start above initial importance value? May reduce oversampling
		const auto margin = std::min<unsigned>(IMP_RANGE, thresholds.back()+stride_);
		build_thresholds(impFun, margin, stride_, thresholds);
	}

	//show_thresholds(thresholds);

build_thresholds_vec:
	ThresholdsVec result;
	result.reserve(thresholds.size());
	for (auto imp: thresholds)
		result.emplace_back(imp, globEff_);
	result.front().second = 1ul;
	result.back().second = 1u;
	return result;
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
	    } break;

	case (PostProcessing::EXP):
		{ size_t expStride(1ul);
		for (ImportanceValue imp = FIRST_THR;
			 imp < impFun.max_value();
		     expStride *= stride, imp += expStride)
			thresholds.push_back(imp);
		thresholds.push_back(impFun.max_value()+1u);
	    } break;

	default:
		throw_FigException("invalid post-processing \"" + postPro_.name
		                  + "\" (" + std::to_string(postPro_.type) + ")");
	}

	// Enforce consistency (remember thresholds may contain previous data)
	std::sort(begin(thresholds), end(thresholds));
	auto newEnd = std::unique(begin(thresholds), end(thresholds));
	thresholds.erase(newEnd, end(thresholds));
	thresholds.shrink_to_fit();
	assert(thresholds.back() > impFun.max_value());
	thresholds.back() = impFun.max_value() + static_cast<ImportanceValue>(1u);
}


} // namespace fig  // // // // // // // // // // // // // // // // // // // //
