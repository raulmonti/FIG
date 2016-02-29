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
#include <cassert>
// C++
#include <vector>
#include <memory>
#include <sstream>
#include <algorithm>  // std::sort()
#include <unordered_map>
// FIG
#include <ThresholdsBuilderAMS.h>
#include <ImportanceFunctionConcrete.h>
#include <ModelSuite.h>


namespace
{

typedef  std::vector< fig::Reference< fig::Traial > >  TraialsVec;
using fig::ImportanceValue;

/// Min simulation length (in # of jumps) to find new thresholds
const unsigned MIN_SIM_EFFORT = 1u<<6;  // 64

/// Max # of failures allowed when searching for a new threshold
const unsigned MAX_NUM_FAILURES = 6u;

/**
 * Get initialized traial instances
 * @param numTraials Number of Traials to retrieve from the TraialPool
 * @param network    User's system model, i.e. a network of modules
 * @param impFun     ImportanceFunction with \ref ImportanceFunction::has_importance_info()
 *                   "importance info" to use for initialization
 * @return std::vector of references to initialized Traial instances
 */
TraialsVec
get_traials(const unsigned& numTraials,
			const fig::ModuleNetwork& network,
			const fig::ImportanceFunction& impFun)
{
	TraialsVec traials;
	fig::TraialPool::get_instance().get_traials(traials, numTraials);
	assert(traials.size() == numTraials);
	for (fig::Traial& t: traials)
		t.initialize(network, impFun);
	return traials;
}


/**
 * @brief Simulate in network exploring the states importance to find thresholds
 *
 *        Use the first 'numSims' Traials from 'traials' to perform
 *        \ref fig::ModuleNetwork::peak_simulation "peak simulations" in
 *        'network' lasting 'simEffort' synchronized jumps each.
 *        The importance assigned to the states visited is taken from 'impVec'
 *
 * @param network   User's system model, i.e. a network of modules
 * @param impFun    ImportanceFunction with \ref ImportanceFunction::has_importance_info()
 *                  "importance info" for all concrete states
 * @param traials   Vector of size >= numSims with references to Traials
 * @param numSims   Number of Traials to simulate with
 * @param simEffort Number of synchronized jumps each simulation will incur in
 */
void
simulate(const fig::ModuleNetwork& network,
		 const fig::ImportanceFunction& impFun,
		 TraialsVec& traials,
		 const unsigned& numSims,
		 const unsigned& simEffort)
{
	unsigned jumpsLeft;

	// Function pointers matching supported signatures (ModuleNetwork::peak_simulation())
	auto predicate = [&](const fig::Traial&) -> bool {
		/// @todo NOTE: try also stopping when we reach max importance
		/// @todo NOTE: try also stopping when we reach min importance
		/// @todo NOTE: try also stopping when we reach importance == "this retrial start importance"
		return --jumpsLeft > 0u;
	};
	auto update = [&](fig::Traial& t) -> void {
		t.level = impFun.importance_of(t.state);
	};
	// Notice we actually use lambdas with captures, which are incompatible
	// with free function pointers (http://stackoverflow.com/q/7852101)
	// However ModuleNetwork::peak_simulation() templetized interface
	// has no problem taking lambdas as arguments.

	for (unsigned int i = 0u ; i < numSims ; i++) {
		jumpsLeft = simEffort;
		network.peak_simulation(traials[i], update, predicate);
	}
}

} // namespace



namespace fig
{

ThresholdsBuilderAMS::ThresholdsBuilderAMS() :
	ThresholdsBuilder("ams"),
	n_(0u),
	k_(0u),
	thresholds_()
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


std::vector< ImportanceValue >
ThresholdsBuilderAMS::build_thresholds(const unsigned& splitsPerThreshold,
									   const ImportanceFunction& impFun)
{
	unsigned currThr(0ul);
	std::vector< ImportanceValue > result;

	build_thresholds_vector(splitsPerThreshold, impFun);
	assert(!thresholds_.empty());
	assert(thresholds_[0] == impFun.min_importance());
	assert(thresholds_.back() > impFun.max_importance());

	result.resize(impFun.max_importance() + 1ul);
	for (ImportanceValue i = impFun.min_importance()
		 ; i <= impFun.max_importance()
		 ; i++)
	{
		while (currThr < thresholds_.size()-1 && i >= thresholds_[currThr+1])
			currThr++;
		result[i] = static_cast<ImportanceValue>(currThr);
	}

	assert(result[impFun.min_importance()] == static_cast<ImportanceValue>(0u));
	assert(result[impFun.max_importance()] ==
			static_cast<ImportanceValue>(thresholds_.size()-2));
	std::vector< ImportanceValue >().swap(thresholds_);  // free mem

	return result;
}


void
ThresholdsBuilderAMS::build_thresholds_vector(
	const unsigned& splitsPerThreshold,
	const ImportanceFunction& impFun)
{
	const ImportanceValue impRange = impFun.max_importance() - impFun.min_importance();
	if (impRange < static_cast<ImportanceValue>(2u)) {
		// Too few importance levels: default to max possible # of thresholds
		std::vector< ImportanceValue >(impRange+2u).swap(thresholds_);
		thresholds_[0u] = impFun.min_importance();
		thresholds_[1u] = impFun.max_importance();
		thresholds_[impRange+1u] = impFun.max_importance()
								   + static_cast<ImportanceValue>(1u);
		return;
	}

	const ModuleNetwork& network = *ModelSuite::get_instance().modules_network();
	tune(network.concrete_state_size(),
		 network.num_transitions(),
		 impFun.max_importance(),
		 splitsPerThreshold);

	unsigned failures(0u);
	unsigned simEffort(MIN_SIM_EFFORT);
	std::vector< ImportanceValue >().swap(thresholds_);
	thresholds_.reserve(impFun.max_importance()/3);
	auto lesser = [](const Traial& lhs, const Traial& rhs)
				  { return lhs.level < rhs.level; };
	TraialsVec traials = get_traials(n_, network, impFun);

	// First AMS iteration is atypical and thus separated from main loop
	thresholds_.push_back(impFun.min_importance());
	Traial& kTraial = traials[n_-k_];
	do {
		simulate(network, impFun, traials, n_, simEffort);
		std::sort(begin(traials), end(traials), lesser);
		kTraial = traials[n_-k_];
		simEffort *= 2;
    } while (thresholds_.back() == kTraial.level);
	if (impFun.max_importance() <= kTraial.level)
		throw_FigException("first iteration of AMS reached max importance, "
						   "rare event doesn't seem rare enough.");
	thresholds_.push_back(kTraial.level);
	simEffort = MIN_SIM_EFFORT;

	// AMS main loop
	while (thresholds_.back() < impFun.max_importance()) {
		// Relaunch all n_-k_ simulations below previously built threshold
		for (unsigned i = 0u ; i < n_-k_ ; i++)
			static_cast<Traial&>(traials[i]) = kTraial;
		simulate(network, impFun, traials, n_-k_, simEffort);
		// New k_-th order peak importance should be the new threshold
		std::sort(begin(traials), end(traials), lesser);
		kTraial = traials[n_-k_];
		if (thresholds_.back() < kTraial.level) {
			// Found valid new threshold
			thresholds_.push_back(kTraial.level);
			simEffort = MIN_SIM_EFFORT;
			failures = 0u;
        } else {
			// Failed to reach higher importance => increase effort
			if (++failures > MAX_NUM_FAILURES)
				goto exit_with_fail;
			simEffort *= 2;
        }
	}

	TraialPool::get_instance().return_traials(traials);
	thresholds_.push_back(impFun.max_importance()
						  + static_cast<ImportanceValue>(1u));
	return;

	exit_with_fail:
		std::stringstream errMsg;
		errMsg << "Failed building thresholds." << " ";
		errMsg << "Importance of the ones found so far: ";
		for (const auto thr: thresholds_)
			errMsg << thr << ", ";
		errMsg << "\b\b  \b\b\n";
		throw_FigException(errMsg.str());
}

} // namespace fig
