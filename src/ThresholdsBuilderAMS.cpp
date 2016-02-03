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
const unsigned MIN_SIM_EFFORT = 1u<<7;  // 128

/// Max # of failures allowed when searching for a new threshold
const unsigned MAX_NUM_FAILURES = 10u;

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

	// Function pointers matching supported signatures (ModuleNetwork.cpp)
	auto predicate = [&](const fig::Traial&) -> bool {
		return --jumpsLeft > 0u;
	};
	auto update = [&](fig::Traial& t) -> void {
		t.importance = impFun.importance_of(t.state);
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


/**
 * @brief Replace importance information with thresholds information
 *
 *        After this call 'impVec' will have the threshold level of every
 *        concrete state, where the j-th threshold level is composed of all
 *        the states between the j-th and the (j+1)-th threshold.
 *        The result can be regarded as a coarser version of the original
 *        importance information which 'impVec' held before this call.
 *
 * @param impVec Vector with the ImportanceValue of every concrete state
 * @param maxImportance Maximum ImportanceValue stored in impVec
 * @param thrImp Its j-th location is the ImportanceValue of the j-th threshold
 *
 * @return Number of thresholds
 */
unsigned
replace_importance_with_thresholds(std::vector< ImportanceValue >& impVec,
								   const ImportanceValue& maxImportance,
								   const std::vector< ImportanceValue >& thrImp)
{
	std::unordered_map<ImportanceValue, ImportanceValue> imp2thr;
	imp2thr.reserve(maxImportance);
	ImportanceValue currentThr(0u);
	// Build "importance-to-thresholds" mapping
	for (ImportanceValue i = static_cast<ImportanceValue>(0u) ; i <= maxImportance ; i++) {
		while (thrImp[currentThr] <= i
			   && currentThr < thrImp.size())
			currentThr++;
		imp2thr[i] = currentThr;
	}
	// Translate
	#pragma omp parallel for default(shared)
	for (size_t s = 0u ; s < impVec.size() ; s++) {
		fig::Event mask = fig::MASK(impVec[s]);
		impVec[s] = mask | imp2thr[fig::UNMASK(impVec[s])];
	}
	return thrImp.size();
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


unsigned
ThresholdsBuilderAMS::build_thresholds_concrete(
	const unsigned& splitsPerThreshold,
	ImportanceFunctionConcrete& impFun,
	std::vector< ImportanceValue >& impVec)
{
	if (impFun.max_importance() < static_cast<ImportanceValue>(2u))
		return 1u;  // not worth it

	const ModuleNetwork& network = *ModelSuite::get_instance().modules_network();
	tune(network.concrete_state_size(),
		 network.num_transitions(),
		 impFun.max_importance(),
		 splitsPerThreshold);

	unsigned failures(0u);
	unsigned simEffort(MIN_SIM_EFFORT);
	auto lesser = [](const Traial& lhs, const Traial& rhs)
				  { return UNMASK(lhs.importance) < UNMASK(rhs.importance); };
	TraialsVec traials = get_traials(n_, network, impFun);

	// First AMS iteration is atypical and thus separated from main loop
	Traial& kTraial = traials[n_-k_];
	do {
		simulate(network, impFun, traials, n_, simEffort);
		std::sort(begin(traials), end(traials), lesser);
		kTraial = traials[n_-k_];
		simEffort *= 2;
	} while (static_cast<ImportanceValue>(0u) == UNMASK(kTraial.importance));
	if (UNMASK(kTraial.importance) >= impFun.max_importance())
		throw_FigException("first iteration of AMS reached max importance, "
						   "rare event doesn't seem rare enough.");
	thresholds_.push_back(UNMASK(kTraial.importance));
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
		if (UNMASK(kTraial.importance) > thresholds_.back()) {
			// Found valid new threshold
			thresholds_.push_back(UNMASK(kTraial.importance));
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
	return replace_importance_with_thresholds(impVec,
											  impFun.max_importance(),
											  thresholds_);

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
