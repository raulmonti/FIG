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

const unsigned ThresholdsBuilderAdaptive::MIN_N = 1ul<<8ul;  // 256


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
ThresholdsBuilderAdaptive::build_thresholds(const unsigned& splitsPerThreshold,
	const ImportanceFunction& impFun,
	const float &p,
	const unsigned& n)
{
    const ModuleNetwork& net = *ModelSuite::get_instance().modules_network();

	// Choose values for n_ and k_
	if (0.0f < p) {
		assert(1.0f > p);  // 'p' is a probability
		n_ = std::max(MIN_N, n);
		k_ = std::round(p*n);
    } else {
		tune(net.concrete_state_size(),
			 net.num_transitions(),
			 impFun.max_value() - impFun.min_value(),
			 splitsPerThreshold);
	}
	ModelSuite::tech_log("Building thresholds with \""+ name +"\" for k/n == "
						 + std::to_string(k_) + "/" + std::to_string(n_) + " ≈ "
						 + std::to_string(static_cast<float>(k_)/n_) + "\n");

	// Chose the thresholds importance values, i.e. run the adaptive algorithm
	build_thresholds_vector(impFun);

	show_thresholds(thresholds_);
	assert(!thresholds_.empty());
	assert(thresholds_[0] == impFun.initial_value());
	assert(thresholds_.back() == 1 + impFun.max_value());

	return thresholds_;
}


void
ThresholdsBuilderAdaptive::tune(const uint128_t &numStates,
								const size_t& numTrans,
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

//	/// @todo FIXME heuristic imported from bluemoon -- Replace with calculi
//	///             based on mathematical analysis if possible

//	// Heuristic for 'n_':
//	//   the more importance values, the more independent runs we need
//	//   for some of them to be successfull.
//	//   Number of states and transitions in the model play a role too.
//    const unsigned explFactor = 1u<<6u;
//	const unsigned statesExtra = numStates.upper() > 0ul ? 1u<<7u :
//								 std::min(numStates.lower()/explFactor, 1ul<<7ul);
//	const unsigned transExtra = std::min(2u*numTrans/explFactor, 1ul<<8ul);
//    n_  = std::ceil(std::log(maxImportance)) * explFactor + statesExtra + transExtra;

    n_ = 50 * numTrans;

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
	assert(traials.size() == numTraials);
	const ModuleNetwork& net = *ModelSuite::get_instance().modules_network();
	for (fig::Traial& t: traials)
		t.initialize(net, impFun);
	return traials;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
