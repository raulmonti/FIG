//==============================================================================
//
//  ThresholdsBuilderAdaptiveSimple.cpp
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
#include <cmath>  // std::ceil(), std::log(), std::round()
#include <cassert>
// C++
#include <vector>
// External code
#include <uint128_t.h>  // uint128::uint128_0
// FIG
#include <ThresholdsBuilderAdaptiveSimple.h>
#include <ModelSuite.h>


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ThresholdsBuilderAdaptiveSimple::ThresholdsBuilderAdaptiveSimple(
    const unsigned& n,
    const unsigned& k) :
        ThresholdsBuilderAdaptive(n),
        property_(nullptr),
        globEff_(0u),
        k_(k),
        thresholds_()
{ /* Not much to do around here */ }


void
ThresholdsBuilderAdaptiveSimple::setup(std::shared_ptr<const Property> property,
                                       const unsigned globalEffort)
{
	property_ = property;
	globEff_ = globalEffort;
}


ThresholdsVec
ThresholdsBuilderAdaptiveSimple::build_thresholds(std::shared_ptr<const ImportanceFunction> impFun)
{
	// For flat importance function we need a dummy thresholds vector
	if (globEff_ < 2u) {
		ImportanceVec({impFun->initial_value(),impFun->max_value()+1}).swap(thresholds_);
		goto consistency_check;
	}

	// Choose values for n_ and k_
	simEngineName_ = impFun->sim_engine_bound();
	tune(ModelSuite::get_instance().modules_network()->num_transitions(),
	     impFun->max_value() - impFun->min_value(),
	     globEff_);
	ModelSuite::tech_log("Building thresholds with \""+ name +"\" for global "
	                     "effort = " + std::to_string(globEff_) + " and k/n = "
						 + std::to_string(k_) + "/" + std::to_string(n_) + " ≈ "
						 + std::to_string(static_cast<float>(k_)/n_) + "\n");

	// Run the adaptive algorithm to choose the thresholds
	build_thresholds_vector(*impFun);
	show_thresholds(thresholds_);
consistency_check:
	assert(!thresholds_.empty());
	assert(thresholds_[0] == impFun->initial_value());
	assert(thresholds_.back() == 1 + impFun->max_value());

	// Build ThresholdsVec to return
	ThresholdsVec thresholds;
	thresholds.reserve(thresholds_.size());
	for (auto imp: thresholds_)
		thresholds.emplace_back(imp, globEff_);
	thresholds.front().second = 1ul;  // fake first thr
	thresholds.back().second = 1ul;   // fake last thre

	return thresholds;
}


void
ThresholdsBuilderAdaptiveSimple::tune(const size_t& numTrans,
                                      const ImportanceValue& maxImportance,
                                      const unsigned& globalEffort)
{
    //assert(uint128::uint128_0 < numStates);
	assert(0ul < numTrans);
	assert(0u < globalEffort);

	ImportanceVec().swap(thresholds_);
	thresholds_.reserve(MAX_NUM_THRESHOLDS);
	assert(thresholds_.size() == 0u);
	assert(thresholds_.capacity() > 0u);

	/// @todo FIXME Following choices are heuristic -- Replace with calculi
	///             based on mathematical analysis if possible

	// Heuristic for 'n_':
	//   the more importance values, the more independent runs we need
	//   for some of them to be successfull.
	//   The same applies to the number of edges (aka symbolic transitions).
	ThresholdsBuilderAdaptive::tune(numTrans, maxImportance, globalEffort);

    // Heuristic for 'k_':
    //   splitsPerThr * levelUpProb == 1  ("balanced growth")
    //   where levelUpProb == k_/n_
	k_ = std::round(n_ / static_cast<float>(globalEffort));

    assert(0u < k_ || static_cast<ImportanceValue>(1u) >= maxImportance);
    assert(k_ < n_ || static_cast<ImportanceValue>(1u) >= maxImportance);
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
