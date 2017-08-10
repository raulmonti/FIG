//==============================================================================
//
//  ThresholdsBuilderAdaptive.cpp
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
#include <ThresholdsBuilderAdaptive.h>
#include <ModelSuite.h>
#include <TraialPool.h>


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

const unsigned ThresholdsBuilderAdaptive::MIN_N = 1ul<<8ul;   //  256
const unsigned ThresholdsBuilderAdaptive::MAX_N = 1ul<<13ul;  // 8192


ThresholdsBuilderAdaptive::ThresholdsBuilderAdaptive(
	const std::string& name,
	const unsigned& n,
	const unsigned& k) :
		ThresholdsBuilder(name),
		n_(n),
		k_(k),
		thresholds_(),
		halted_(false)
{ /* Not much to do around here */ }


ImportanceVec
ThresholdsBuilderAdaptive::build_thresholds(
	const unsigned& splitsPerThreshold,
	const ImportanceFunction& impFun,
	const float &p,
	const unsigned& n)
{
    const ModuleNetwork& net = *ModelSuite::get_instance().modules_network();

	// For flat importance function we need a dummy thresholds vector
	if (splitsPerThreshold < 2u) {
		ImportanceVec({impFun.initial_value(),impFun.max_value()+1}).swap(thresholds_);
		goto consistency_check;
	}

	// Choose values for n_ and k_
	if (0.0f < p) {
		assert(1.0f > p);  // 'p' is a probability
		n_ = std::max(MIN_N, n);
		k_ = std::round(p*n);
    } else {
		tune(net.num_transitions(),
			 impFun.max_value() - impFun.min_value(),
			 splitsPerThreshold);
	}
	ModelSuite::tech_log("Building thresholds with \""+ name +"\" for k/n == "
						 + std::to_string(k_) + "/" + std::to_string(n_) + " ≈ "
						 + std::to_string(static_cast<float>(k_)/n_) + "\n");

	// Chose the thresholds importance values, i.e. run the adaptive algorithm
	build_thresholds_vector(impFun);

	show_thresholds(thresholds_);

consistency_check:
	assert(!thresholds_.empty());
	assert(thresholds_[0] == impFun.initial_value());
	assert(thresholds_.back() == 1 + impFun.max_value());

	return thresholds_;
}


void
ThresholdsBuilderAdaptive::tune(const size_t& numTrans,
								const ImportanceValue& maxImportance,
								const unsigned& splitsPerThr)
{
    //assert(uint128::uint128_0 < numStates);
	assert(0ul < numTrans);
	assert(0u < splitsPerThr);

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

	// Importance typically grows exponentially with the rarity paremeter,
	// thus we use (a scaled version of) its logarithm.
	const unsigned impFactor = (1u<<6u) * std::ceil(std::log(maxImportance));
	// Instead, the number of transitions tends to grow linearly
	const unsigned transFactor = 5u*numTrans;

	const double balance = 0.5;  // must be within (0.0, 1.0)
	// more relevance to importance  => balance++
	// more relevance to transitions => balance--
	n_  = std::min(impFactor  , static_cast<unsigned>((    balance)*MAX_N))
	    + std::min(transFactor, static_cast<unsigned>((1.0-balance)*MAX_N));

    // Heuristic for 'k_':
    //   splitsPerThr * levelUpProb == 1  ("balanced growth")
    //   where levelUpProb == k_/n_
    k_ = std::round(n_ / static_cast<float>(splitsPerThr));

    assert(0u < k_ || static_cast<ImportanceValue>(1u) >= maxImportance);
    assert(k_ < n_ || static_cast<ImportanceValue>(1u) >= maxImportance);
}


ThresholdsBuilderAdaptive::TraialsVec
ThresholdsBuilderAdaptive::get_traials(const unsigned& numTraials,
									   const fig::ImportanceFunction& impFun)
{
	TraialsVec traials;
	fig::TraialPool::get_instance().get_traials(traials, numTraials);

	/// @todo TODO erase debug print
	std::cerr << "\n*** Requested " << numTraials << " Traials and got " << traials.size() << std::endl;

	assert(traials.size() == numTraials);
	const ModuleNetwork& net = *ModelSuite::get_instance().modules_network();
	for (fig::Traial& t: traials)
		t.initialize(net, impFun);
	return traials;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
