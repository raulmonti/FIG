//==============================================================================
//
//  ThresholdsBuilderSMC.cpp
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
#include <cassert>
// C++
#include <sstream>
#include <iterator>   // std::begin(), std::end()
#include <algorithm>  // std::sort(), std::max({})
// FIG
#include <ThresholdsBuilderSMC.h>
#include <ModuleNetwork.h>
#include <ModelSuite.h>
#include <Traial.h>

// ADL
using std::begin;
using std::end;


namespace
{

using fig::ImportanceValue;
using TraialsVec = fig::ThresholdsBuilderAdaptive::TraialsVec;

/// Min simulation length (in # of jumps) to find new thresholds
const unsigned MIN_SIM_EFFORT = 1u<<6;  // 64

/// Max # of failures allowed when searching for a new threshold
const unsigned MAX_NUM_FAILURES = 6u;


/// @todo TODO write docstring
void
build_states_distribution(const fig::ModuleNetwork& network,
						  const fig::ImportanceFunction& impFun,
						  TraialsVec& traials,
						  const unsigned& n,
						  const unsigned& k,
						  const ImportanceValue& lastThr)
{
	assert(0u < k);
	assert(k < n);
	assert(traials.size() == n+k);

	// Function pointers matching ModuleNetwork::peak_simulation() signatures
	auto predicate = [&lastThr](const fig::Traial& t) -> bool {
		return t.level < lastThr;
	};
	auto update = [&impFun](fig::Traial& t) -> void {
		t.level = impFun.importance_of(t.state);
	};

	// Starting from previously computed initial states, advance the first
	// 'n' traials until they meet a state realizing lastThr
	for (unsigned i = 0u ; i < n ; i++) {
		traials[i] = traials[n+(i%k)];
		network.peak_simulation(traials[i], update, predicate);
	}

	// Store 'k' from those 'n' new states as the next-gen initial states
	for (unsigned i = 0u ; i < k ; i++)
		traials[n+i] = traials[i];
}


/**
 *
 *
 * @todo TODO update this docstring
 *
 *
 * @brief Simulate in network exploring states' importance to find thresholds
 *
 *        Launch n_ simulations from specified initial states; resulting
 *        (1 - k_/n_) quantile's importance is proposed as new threshold.
 *        Reachable states realizing such importance are left inside
 *        'initialStates' (discarding previous contents)
 *
 * @param network       User's system model, i.e. a network of modules
 * @param impFun        ImportanceFunction with
 *                      \ref ImportanceFunction::has_importance_info()
 *                      "importance info" for all concrete states
 * @param simEffort     Number of jumps each simulation will incur in
 * @param initialStates Concrete states simulations will start from
 *                      <b>(modified, see details)</b>
 * @param traials       Vector of size >= n_ with references to Traials
 */
ImportanceValue
find_new_threshold(const fig::ModuleNetwork& network,
				   const fig::ImportanceFunction& impFun,
				   TraialsVec& traials,
				   const unsigned& n,
				   const unsigned& k,
				   const ImportanceValue& lastThr)
{
	assert(0u < k);
	assert(k < n);
	assert(traials.size() == n+k);
	fig::Traial& quantile(traials[n-k]);
	unsigned jumpsLeft, failures(0u), simEffort(MIN_SIM_EFFORT);

	// Function pointers matching ModuleNetwork::peak_simulation() signatures
	auto predicate = [&jumpsLeft](const fig::Traial&) -> bool {
		return --jumpsLeft > 0u;
	};
	auto update = [&impFun](fig::Traial& t) -> void {
		t.level = impFun.importance_of(t.state);
	};
	// Function to sort traials according to their ImportanceValue
	auto lesser = [](const fig::Traial& lhs, const fig::Traial& rhs)
				  { return lhs.level < rhs.level; };
	// What happens when the new 1-k/n quantile isn't higher than lastThr
	auto reinit = [&n, &k, &simEffort] (TraialsVec& traials, unsigned& fails) {
		fig::ModelSuite::tech_log("Failed to find new threshold (reached "
								  + std::to_string(static_cast<fig::Traial&>
									(traials[n-k]).level) + " )\n");
		if (++fails >= MAX_NUM_FAILURES)
			return false;
		simEffort *= 2u;
		for (unsigned i = 0u ; i < n ; i++)
			traials[i] = traials[n+(i%k)];
		return true;
	};

	do {
		// Run 'n' simulations
		for (unsigned i = 0u ; i < n ; i++) {
			jumpsLeft = simEffort;
			network.peak_simulation(traials[i], update, predicate);
		}
		// Sort to find 1-k/n quantile
		auto nth_traial(begin(traials));
		std::advance(nth_traial, n);
		std::sort(begin(traials), nth_traial, lesser);
		quantile = traials[n-k];
	} while (quantile.level <= lastThr && reinit(traials, failures));

	return quantile.level;
}

} // namespace



namespace fig
{

ThresholdsBuilderSMC::ThresholdsBuilderSMC() : ThresholdsBuilderAdaptive("smc")
{ /* Not much to do around here */ }


void
ThresholdsBuilderSMC::build_thresholds_vector(const ImportanceFunction& impFun)
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

	std::vector< ImportanceValue >().swap(thresholds_);
	thresholds_.reserve((impFun.max_value()-impFun.min_value()) / 5u);  // magic
	TraialsVec traials = ThresholdsBuilderAdaptive::get_traials(n_+k_, impFun);
	const ModuleNetwork& network = *ModelSuite::get_instance().modules_network();

	// SMC initialization
	thresholds_.push_back(impFun.min_value());
	ImportanceValue newThreshold =
		find_new_threshold(network, impFun, traials, n_, k_, thresholds_.back());
	if (impFun.max_value() <= newThreshold)
		ModelSuite::tech_log("first iteration of SMC reached max importance, "
							 "rare event doesn't seem so rare.");
	thresholds_.push_back(newThreshold);

	/// @todo TODO erase debug print
	std::cerr << "First threshold: " << thresholds_.back() << std::endl;

	// SMC main loop
	while (thresholds_.back() < impFun.max_value()) {
		ImportanceValue lastThr = thresholds_.back();
		// Find "initial states" (and clocks valuations) realizing last threshold
		build_states_distribution(network, impFun, traials, n_, k_, lastThr);
		// Find sims' 1-k/n quantile starting from those initial states
		newThreshold = find_new_threshold(network, impFun, traials, n_, k_, lastThr);
		// Use said quantile as new threshold, if possible
		if (newThreshold <= thresholds_.back())
			goto exit_with_fail;  // well, fuck
		thresholds_.push_back(newThreshold);

		/// @todo TODO erase debug print
		std::cerr << "New threshold: " << thresholds_.back() << std::endl;
	}

	TraialPool::get_instance().return_traials(traials);
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
ThresholdsBuilderSMC::tune(const size_t& numStates,
						   const size_t& numTrans,
						   const ImportanceValue& maxImportance,
						   const unsigned& splitsPerThr)
{
	const unsigned n(n_);
	ThresholdsBuilderAdaptive::tune(numStates, numTrans, maxImportance, splitsPerThr);
	const float p(static_cast<float>(k_)/n_);
	n_ = std::max({n, n_, ThresholdsBuilderAdaptive::MIN_N});
	k_ = std::round(p*n_);
}

} // namespace fig
