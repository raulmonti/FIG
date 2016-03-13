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
#include <algorithm>  // std::sort(), std::max({})
#include <unordered_map>
// FIG
#include <ThresholdsBuilderAMS.h>
#include <ImportanceFunctionConcrete.h>
#include <ModelSuite.h>


namespace
{

using fig::ImportanceValue;
using TraialsVec = fig::ThresholdsBuilderAdaptive::TraialsVec;

/// Min simulation length (in # of jumps) to find new thresholds
const unsigned MIN_SIM_EFFORT = 1u<<7;  // 128

/// Max # of failures allowed when searching for a new threshold
const unsigned MAX_NUM_FAILURES = 5u;


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
	assert(traials.size() >= numSims);
	unsigned jumpsLeft;

	// Function pointers matching supported signatures (ModuleNetwork::peak_simulation())
    auto predicate = [&jumpsLeft, &impFun](const fig::Traial& t) -> bool {
        return --jumpsLeft > 0u && t.level < impFun.max_value();
	};
    auto update = [&impFun](fig::Traial& t) -> void {
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

ThresholdsBuilderAMS::ThresholdsBuilderAMS() : ThresholdsBuilderAdaptive("ams")
{ /* Not much to do around here */ }


void
ThresholdsBuilderAMS::build_thresholds_vector(
	const ImportanceFunction& impFun)
{
	assert(0u < k_);
	assert(k_ < n_);

	const ImportanceValue impRange = impFun.max_value() - impFun.min_value();
	if (impRange < static_cast<ImportanceValue>(2u)) {
		// Too few importance levels: default to max possible # of thresholds
		std::vector< ImportanceValue >(impRange+2u).swap(thresholds_);
		thresholds_[0u] = impFun.min_value();
		thresholds_[1u] = impFun.max_value();
		thresholds_[impRange+1u] = impFun.max_value()
								   + static_cast<ImportanceValue>(1u);
		return;
	}

    unsigned failures(0u), simEffort(MIN_SIM_EFFORT);
	std::vector< ImportanceValue >().swap(thresholds_);
	thresholds_.reserve((impFun.max_value()-impFun.min_value()) / 5u);  // magic
	auto lesser = [](const Traial& lhs, const Traial& rhs)
				  { return lhs.level < rhs.level; };
	TraialsVec traials = ThresholdsBuilderAdaptive::get_traials(n_, impFun);
	const ModuleNetwork& network = *ModelSuite::get_instance().modules_network();

	// AMS initialization
	thresholds_.push_back(impFun.min_value());
	do {
		simulate(network, impFun, traials, n_, simEffort);
		std::sort(begin(traials), end(traials), lesser);
    } while (thresholds_.back() == traials[n_-k_].get().level
             && (simEffort *= 2u));
    if (impFun.max_value() <= traials[n_-k_].get().level)
		ModelSuite::tech_log("first iteration of AMS reached max importance, "
							 "rare event doesn't seem so rare.");
    thresholds_.push_back(traials[n_-k_].get().level);
    simEffort = MIN_SIM_EFFORT;

	// AMS main loop
	while (thresholds_.back() < impFun.max_value()) {
		// Relaunch all n_-k_ simulations below previously built threshold
        for (unsigned i = 0u ; i < n_-k_ ; i++)
            traials[i].get() = traials[n_-k_].get();  // copy values, not addresses
		simulate(network, impFun, traials, n_-k_, simEffort);
        // New 1-k_/n_ importance quantile should be the new threshold
		std::sort(begin(traials), end(traials), lesser);
        const ImportanceValue newThreshold = traials[n_-k_].get().level;
        if (thresholds_.back() < newThreshold) {
			// Found valid new threshold
            thresholds_.push_back(newThreshold);
			simEffort = MIN_SIM_EFFORT;
			failures = 0u;
        } else {
			// Failed to reach higher importance => increase effort
			if (++failures > MAX_NUM_FAILURES)
				goto exit_with_fail;
			simEffort *= 2u;
        }
	}

	TraialPool::get_instance().return_traials(traials);
    { std::stringstream msg;
    msg << "ImportanceValue of the chosen thresholds:";
    for (size_t i = 1ul ; i < thresholds_.size() ; i++)
        msg << " " << thresholds_[i];
    ModelSuite::tech_log(msg.str() + "\n\n"); }
    thresholds_.push_back(impFun.max_value() + static_cast<ImportanceValue>(1u));
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


void
ThresholdsBuilderAMS::tune(const size_t& numStates,
						   const size_t& numTrans,
						   const ImportanceValue& maxImportance,
						   const unsigned& splitsPerThr)
{
	const unsigned n(n_);
	ThresholdsBuilderAdaptive::tune(numStates, numTrans, maxImportance, splitsPerThr);
    // Impose our own minimum if it was larger than the one automatically chosen
    const float p(static_cast<float>(k_)/n_);
	n_ = std::max({n, n_, ThresholdsBuilderAdaptive::MIN_N});
	k_ = std::round(p*n_);
}

} // namespace fig
