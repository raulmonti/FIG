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
#include <random>
#include <sstream>
#include <iterator>   // std::begin(), std::end()
#include <algorithm>  // std::sort(), std::max({})
// External code
#include <pcg_random.hpp>
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

using fig::Traial;
using fig::ImportanceValue;
using TraialsVec = fig::ThresholdsBuilderAdaptive::TraialsVec;

/// Allowed simulation length (in # of jumps) to find new thresholds
unsigned SIM_EFFORT = 1u<<6u; // 64

/// Allowed # of failures when searching for a new threshold
unsigned NUM_FAILURES = 6u;

// RNG for randomized traial selection  ///////////////////////////
#ifndef NDEBUG
  const unsigned RNG_SEED(1234567803u);  // repeatable outcome
#else
  const unsigned RNG_SEED(std::random_device{}());
#endif
#ifndef PCG_RNG
  /// Mersenne-Twister RNG
  std::mt19937 RNG(RNG_SEED);
#else
  /// PCG family RNG
  pcg32 RNG(RNG_SEED);
#endif
// ////////////////////////////////////////////////////////////////


/**
 * @brief Choose reachable states realizing the last threshold chosen
 *
 *        Starting from the previous initial states stored in the last 'k'
 *        positions of 'traials', look for states with importance == 'lastThr'
 *        Store them in the last 'k' positions of traials (overwriting previous
 *        information) to use them as initial states in simulations to come.
 *
 * @param network User's system model, i.e. a network of modules
 * @param impFun  ImportanceFunction with
 *                \ref ImportanceFunction::has_importance_info()
 *                "importance info" for all concrete states
 * @param traials Vector of size >= n+k with references to Traials;
 *                the first 'n' are used to reach states realizing 'lastThr',
 *                the last  'k' are updated with that info to be used later as
 *                initial states<b>(modified)</b>
 * @param n       Number of traials to use for reaching states realizing 'lastThr'
 * @param k       Number of traials where initial states to start simulations from
 * @param lastThr ImportanceValue of last chosen threshold
 *
 * @return Whether the run was successfull
 *
 * @note The first 'n' positions of 'traials' are left in states with importance
 *       equal to 'lastThr'. The user can therefore run simulations using those
 *       first 'n' traials <b>without the need to intialize them beforehand</b>.
 */
bool
build_states_distribution(const fig::ModuleNetwork& network,
						  const fig::ImportanceFunction& impFun,
						  TraialsVec& traials,
						  const unsigned& n,
						  const unsigned& k,
						  const ImportanceValue& lastThr)
{
	assert(0u < k);
	assert(k < n);
    assert(traials.size() >= n+k);

	const unsigned TOLERANCE(2u*NUM_FAILURES);
	unsigned jumpsLeft, pos, fails(0u);

	// Function pointers matching ModuleNetwork::peak_simulation() signatures
	auto predicate = [&jumpsLeft,lastThr](const Traial& t) -> bool {
		return --jumpsLeft > 0u && lastThr != t.level;
    };
    auto update = [&impFun](Traial& t) -> void {
        t.level = impFun.importance_of(t.state);
    };

	// Starting uniform-randomly from previously computed initial states,
    // advance the first 'n' traials until they meet a state realizing lastThr
    std::uniform_int_distribution<unsigned> uniK(0, k-1);
    for (unsigned i = 0u ; i < n ; i++) {
		fails = 0u;
		Traial& t(traials[i]);
        do {
			jumpsLeft = SIM_EFFORT * (1u+fails);
			t = traials[n + uniK(RNG)];  // choose randomly among last 'k'
			assert(lastThr > t.level);
            network.peak_simulation(t, update, predicate);
		} while (lastThr != t.level && ++fails < TOLERANCE);
	}
	if (fails >= TOLERANCE) {
		// Couldn't make the 'n' traials reach lastThr: we failed
		fig::ModelSuite::tech_log("*");  // report failure
		return false;
	}

    // Store 'k' from those 'n' new states as the next-gen initial states
	// Uniformly choose which will be those 'k' (without repetitions)
    std::vector<bool> used(n, false);
    std::uniform_int_distribution<unsigned> uniN(0, n-1);
    for (unsigned i = 0u ; i < k ; i++) {
        do { pos = uniN(RNG); } while (used[pos]);
		traials[n+i].get() = traials[pos];  // copy values, not addresses
        used[pos] = true;
    }

	return true;
}


/**
 * @brief Simulate in network exploring states' importance to find a new
 *        threshold higher than the last one
 *
 *        Launch n simulations from initial states chosen randomly among those
 *        realizing 'lastThr' in a previous run. Resulting (1 - k/n) quantile's
 *        importance is proposed as new threshold. Reachable states realizing
 *        such importance are left in the first 'n' positions of 'traials'
 *
 * @param network User's system model, i.e. a network of modules
 * @param impFun  ImportanceFunction with
 *                \ref ImportanceFunction::has_importance_info()
 *                "importance info" for all concrete states
 * @param traials Vector of size >= n+k with references to Traials;
 *                the first 'n' are used to carry out simulations,
 *                the last  'k' are kept unchanged to use as initial states
 *                <b>(modified)</b>
 * @param n       Number of simulations to run per iteration
 * @param k       Number of initial states to start simulations from
 * @param lastThr ImportanceValue of last chosen threshold, to be overcome
 *
 * @return New threshold ImportanceValue > lastThr if successfull,
 *         chosen as the 1-k/n quantile of the simulations run
 *         (sorted by maximum importance reached)
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
    assert(traials.size() >= n+k);
    using std::to_string;
	unsigned jumpsLeft, fails(0u), simEffort(SIM_EFFORT);
	const ImportanceValue MAX_IMP(impFun.max_value());
    ImportanceValue newThr(lastThr);

	// Function pointers matching ModuleNetwork::peak_simulation() signatures
	auto predicate = [&jumpsLeft,&MAX_IMP](const Traial& t) -> bool {
		return --jumpsLeft > 0u && MAX_IMP > t.level;
    };
    auto update = [&impFun](Traial& t) -> void {
        t.level = impFun.importance_of(t.state);
    };
    // What happens when the new quantile isn't higher than lastThr
	auto reinit = [&n, &k, &fails, &simEffort] (TraialsVec& traials) {
		fig::ModelSuite::tech_log("-");  // report failure
		if (++fails >= ::NUM_FAILURES)
			return false;
		simEffort *= 2u;
        // Choose the new 'n' initial states uniformly among the last 'k'
        std::uniform_int_distribution<unsigned> uniK(0, k-1);
        for (unsigned i = 0u ; i < n ; i++)
            traials[i].get() = traials[n + uniK(RNG)];  // copy values, not addresses
		return true;
	};

	do {
		// Run 'n' simulations
		for (unsigned i = 0u ; i < n ; i++) {
			jumpsLeft = simEffort;
			network.peak_simulation(traials[i], update, predicate);
		}
		// Sort to find 1-k/n quantile
		std::sort(begin(traials), begin(traials)+n,
				  [](Traial& lhs, Traial& rhs){return lhs.level < rhs.level;});
        newThr = traials[n-k].get().level;
	} while (newThr <= lastThr && reinit(traials));

	if (fails < ::NUM_FAILURES)
		fig::ModelSuite::tech_log("+");  // report success

    return newThr;
}

} // namespace



namespace fig
{

void
ThresholdsBuilderSMC::build_thresholds_vector(const ImportanceFunction& impFun)
{
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

    assert(0u < k_);
    assert(k_ < n_);

    std::vector< ImportanceValue >().swap(thresholds_);
	thresholds_.reserve((impFun.max_value()-impFun.initial_value()) / 5u);  // magic
	TraialsVec traials = ThresholdsBuilderAdaptive::get_traials(n_+k_, impFun);
	const ModuleNetwork& network = *ModelSuite::get_instance().modules_network();

	// SMC initialization
	thresholds_.push_back(impFun.initial_value());  // start from initial state importance
	assert(thresholds_.back() < impFun.max_value());
	ImportanceValue newThreshold =
		find_new_threshold(network, impFun, traials, n_, k_, thresholds_.back());
	if (impFun.max_value() <= newThreshold)
		ModelSuite::tech_log("\nFirst iteration of SMC reached max importance, "
							 "rare event doesn't seem so rare!\n");
	thresholds_.push_back(newThreshold);

	// SMC main loop
	while (thresholds_.back() < impFun.max_value()) {
        const ImportanceValue lastThr = thresholds_.back();
		// Find "initial states" (and clocks valuations) realizing last threshold
		if (!build_states_distribution(network, impFun, traials, n_, k_, lastThr))
			goto exit_with_fail;  // couldn't find those initial states
		// Find sims' 1-k/n quantile starting from those initial states
		newThreshold = find_new_threshold(network, impFun, traials, n_, k_, lastThr);
        // Use said quantile as new threshold if possible
		if (newThreshold <= thresholds_.back())
			goto exit_with_fail;  // well, fuck
		thresholds_.push_back(newThreshold);
	}

	TraialPool::get_instance().return_traials(traials);
    { std::stringstream msg;
	msg << "\nImportanceValue of the chosen thresholds:";
	for (size_t i = 1ul ; i < thresholds_.size() ; i++)
		msg << " " << thresholds_[i];
	ModelSuite::tech_log(msg.str() + "\n"); }
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
ThresholdsBuilderSMC::tune(const uint128_t &numStates,
						   const size_t& numTrans,
						   const ImportanceValue& maxImportance,
						   const unsigned& splitsPerThr)
{
	const unsigned n(n_);
	ThresholdsBuilderAdaptive::tune(numStates, numTrans, maxImportance, splitsPerThr);
    // This algorith is statistically better (way better) than AMS,
	// resulting in the thresholds being chosen really close to each other.
	// The counterpart is that too many thresholds are chosen and thus the
    // thresholds building takes too long.
	// We try to counter that a little by reducing the probability of level up
	const float p((k_*0.333f)/n_);
	n_ = std::max({n, n_, ThresholdsBuilderAdaptive::MIN_N});
	k_ = std::round(p*n_);
	// Simulations length will be directly proportional to the probability 'p'
	// within the range (0.06 , 0.1)
	::SIM_EFFORT = p < 0.06 || numStates > (1ul<<20u) ? MIN_SIM_EFFORT :
				   p > 0.1  || numStates < (1ul<<10u) ? MAX_SIM_EFFORT :
				   std::round(25.f*(MAX_SIM_EFFORT-MIN_SIM_EFFORT) * p
							  + 2.5f*MIN_SIM_EFFORT - 1.5f*MAX_SIM_EFFORT);
	// The allowed # of failures will be inversely proportional to 'density'
	// within the range (0.01 , 5.0)
	const double logStates(log2(numStates.lower())+64*log2(1+numStates.upper()));
	const float density(numTrans/logStates);  // we deal with sparse graphs
	::NUM_FAILURES = density > 5.00f || numStates > (1ul<<20u) ? MIN_NUM_FAILURES :
					 density < 0.01f                           ? MAX_NUM_FAILURES :
					 std::round((MIN_NUM_FAILURES*.2004f-MAX_NUM_FAILURES*.2004f) * density
								 + 1.002f*MAX_NUM_FAILURES - .002f*MIN_NUM_FAILURES);
}

} // namespace fig
