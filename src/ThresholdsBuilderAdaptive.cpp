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
// FIG
#include <ThresholdsBuilderAdaptive.h>
#include <ModelSuite.h>
#include <TraialPool.h>


namespace fig
{

const unsigned ThresholdsBuilderAdaptive::MIN_N = 1ul<<9ul;  // 512


ThresholdsBuilderAdaptive::ThresholdsBuilderAdaptive(
	const std::string& name,
	const unsigned& n,
	const unsigned& k) :
		ThresholdsBuilder(name),
		n_(n),
		k_(k),
		thresholds_()
{ /* Not much to do around here */ }


std::vector< ImportanceValue >
ThresholdsBuilderAdaptive::build_thresholds(
	const unsigned& splitsPerThreshold,
	const ImportanceFunction& impFun,
	const float &p,
	const unsigned& n)
{
	unsigned currThr(0ul);
	std::vector< ImportanceValue > result;

	// Choose values for n_ and k_
	if (0.0f < p) {
		assert(1.0f > p);  // 'p' is a probability
		n_ = std::max(MIN_N, n);
		k_ = std::round(p*n);
	} else {
		const ModuleNetwork& net = *ModelSuite::get_instance().modules_network();
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
	assert(!thresholds_.empty());
	assert(thresholds_[0] == impFun.min_value());
	assert(thresholds_.back() > impFun.max_value());

	// Format result and finish up
	result.resize(impFun.max_value() - impFun.min_value() + 1ul);
	for (ImportanceValue i = impFun.min_value() ; i <= impFun.max_value() ; i++)
	{
		while (currThr < thresholds_.size()-1 && i >= thresholds_[currThr+1])
			currThr++;
		result[i] = static_cast<ImportanceValue>(currThr);
	}

	assert(result[impFun.min_value()] == static_cast<ImportanceValue>(0u));
	assert(result[impFun.max_value()] ==
			static_cast<ImportanceValue>(thresholds_.size()-2));
	std::vector< ImportanceValue >().swap(thresholds_);  // free mem

	return result;
}


void
ThresholdsBuilderAdaptive::tune(const size_t& numStates,
								const size_t& numTrans,
								const ImportanceValue& maxImportance,
								const unsigned& splitsPerThr)
{
	assert(0u < numStates);
	assert(0u < numTrans);
	assert(0u < splitsPerThr);

	std::vector< ImportanceValue >().swap(thresholds_);
	thresholds_.reserve(maxImportance/2);
	assert(thresholds_.size() == 0u);
	assert(thresholds_.capacity() > 0u);

	/// @todo FIXME code imported from bluemoon -- Change to something more solid

	// Heuristic for 'n_':
	//   the more importance values, the more independent runs we need
	//   for some of them to be successfull.
	//   Number of states and transitions in the model play a role too.
	const unsigned explFactor = 50;
	const unsigned statesExtra = std::min<unsigned>(numStates/explFactor, 100);
	const unsigned transExtra = std::min<unsigned>(2*numTrans/explFactor, 150);
	n_  = std::ceil(std::log(maxImportance)) * explFactor + statesExtra + transExtra;

	// Heuristic for 'k_':
	//   splitsPerThr * levelUpProb == 1  ("balanced growth")
	//   where levelUpProb == k_/n_
	k_ = std::round(n_ / static_cast<float>(splitsPerThr));

	assert(0u < k_);
	assert(k_ < n_);
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

} // namespace fig
