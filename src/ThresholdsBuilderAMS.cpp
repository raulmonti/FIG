//==============================================================================
//
//  ThresholdsBuilderAMS.cpp
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
#include <vector>
#include <memory>
#include <algorithm>  // std::sort()
// FIG
#include <ThresholdsBuilderAMS.h>
#include <ModelSuite.h>
#include <SimulationEngineNosplit.h>


namespace
{

/// @todo TODO document and implement
std::vector< fig::Reference< fig::Traial > >
get_traials(unsigned numTraials, const fig::StateInstance& initialState)
{
	std::vector< fig::Reference< fig::Traial > > traials;

	// ???

	for (unsigned i = 0u ; i < n_ ; i++)
		traials[i].state = network.initial_state();
	return traials;
}

} // namespace


namespace fig
{

ThresholdsBuilderAMS::ThresholdsBuilderAMS() :
	ThresholdsBuilder("ams"),
	n_(0u),
	k_(0u),
	thresholds_()
//    numThresholds_(-1),
//    minRareLvl_(-1)
{ /* Not much to do around here */ }


void
ThresholdsBuilderAMS::tune(const size_t& numStates,
						   const size_t& numTrans,
						   const ImportanceValue& maxImportance,
						   const unsigned& splitsPerThr)
{
	assert(0u < numStates);
	assert(0u < numTrans);
	assert(0u < splitsPerThr);

	std::vector< ImportanceValue >().swap(thresholds_);
	thresholds_.reserve(maxImportance/2);

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


unsigned
ThresholdsBuilderAMS::build_thresholds_concrete(
	const unsigned& splitsPerThreshold,
	ImportanceValue& maxImportance,
	ImportanceValue& minRareImportance,
	std::vector< ImportanceValue >& impVec) const
{
	const ModuleNetwork& network = *ModelSuite::get_instance().modules_network();
	tune(network.concrete_state_size(),
		 network.num_transitions(),
		 maxImportance,
		 splitsPerThreshold);

	assert(thresholds_.size() == 0u);
	assert(thresholds_.capacity() > 0u);


	/// @todo TODO translate code from bluemoon's "AMS::get_thresholds" in PR_AMS.cc


	auto lesser = [](const Traial& lhs, const Traial& rhs)
				  { return lhs.importance < rhs.importance; };
	auto traials = get_traials(n, network.initial_state());

	// First AMS iteration is atypical and thus separated from main loop
	simulate(network, traials, n_, thresholds_);
	std::sort(begin(traials), end(traials), lesser);
	thresholds_.push_back(traials[k_-1].importance);

	// AMS main loop
	do {
		// Relaunch all simulations below the lastly built threshold (n_-k_)
		for (unsigned i = n_-k_ ; i < n_ ; i++)
			traials[i].state = traials[k_-1].state;
		simulate(network, traials, n_-k_, thresholds_);
		// From all simulations, k_-th order importance is the new threshold
		std::sort(begin(traials), end(traials), lesser);
		thresholds_.push_back(traials[k_-1].importance);
	} while (thresholds_[thresholds_.size()-1] < maxImportance);

	return_traials(traials);

	return maxImportance;  // FIXME sure about this???
}

} // namespace fig
