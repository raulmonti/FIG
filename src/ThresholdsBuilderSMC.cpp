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


namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

using fig::Traial;
using fig::ImportanceValue;
using TraialsVec = fig::ThresholdsBuilderAdaptive::TraialsVec;

/// Allowed simulation length (in # of jumps) to find new thresholds
unsigned SIM_EFFORT = 1u<<6u; // 64

/// Allowed # of failures when searching for a new threshold
unsigned NUM_FAILURES = 6u;

// RNG for randomized traial selection  ///////////////////////////
//
const unsigned long RNG_SEED =
#ifdef RANDOM_RNG_SEED
    std::random_device{}();
#else
    std::mt19937::default_seed;
#endif
//
std::mt19937 RNG(RNG_SEED);
//
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
 * @param halt    For external (thread-parallel) signalling: halt computation
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
						  const ImportanceValue& lastThr,
						  const bool& halt)
{
	assert(0u < k);
	assert(k < n);
    assert(traials.size() >= n+k);

	const unsigned TOLERANCE(2u*NUM_FAILURES),
				   IMP_RANGE(impFun.max_value()-impFun.min_value()),
				   OVERLENGTH(IMP_RANGE <  20u ? 1u :
							  IMP_RANGE > 100u ? 5u : std::round(0.05f*IMP_RANGE)),
				   SIM_LENGTH(SIM_EFFORT*OVERLENGTH);
	unsigned jumpsLeft, pos, fails(0u);

	// Function pointers matching ModuleNetwork::peak_simulation() signature
	auto predicate = [&jumpsLeft,lastThr](const Traial& t) -> bool {
		return --jumpsLeft > 0u && lastThr != t.level;
    };
    auto update = [&impFun](Traial& t) -> void {
        t.level = impFun.importance_of(t.state);
    };

	// Starting uniformly random from initial states computed before,
    // advance the first 'n' traials until they meet a state realizing lastThr
    std::uniform_int_distribution<unsigned> uniK(0, k-1);
	for (size_t i = 0ul ; i < n && !halt ; i++) {
		fails = 0u;
		Traial& t(traials[i]);
        do {
			jumpsLeft = SIM_LENGTH * (1u+fails);
			t = traials[n + uniK(RNG)];  // choose randomly among last 'k'
#ifndef NDEBUG
			/// @bug FIXME is the following check logical?
			if (lastThr <= t.level)
				throw_FigException("internal error during thresholds selection");
#endif
            network.peak_simulation(t, update, predicate);
		} while (!halt && lastThr != t.level && ++fails < TOLERANCE);
	}

	if (fails >= TOLERANCE || halt) {
		// Either halted or couldn't make the 'n' traials reach lastThr
		fig::ModelSuite::tech_log("*");  // report failure
		return false;
	}

    // Store 'k' from those 'n' new states as the next-gen initial states
	// Uniformly choose which will be those 'k' (without repetitions)
    std::vector<bool> used(n, false);
    std::uniform_int_distribution<unsigned> uniN(0, n-1);
	for (size_t i = 0ul ; i < k ; i++) {
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
 * @param halt    For external (thread-parallel) signalling: halt computation
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
				   const ImportanceValue& lastThr,
				   const bool& halt)
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
	// 'reinit' is what happens when the new quantile isn't higher than lastThr
	auto reinit = [&n, &k, &fails, &simEffort] (TraialsVec& traials) {
		fig::ModelSuite::tech_log("-");  // report failure
		if (++fails >= ::NUM_FAILURES)
			return false;
		simEffort *= 2u;
        // Choose the new 'n' initial states uniformly among the last 'k'
        std::uniform_int_distribution<unsigned> uniK(0, k-1);
		for (size_t i = 0ul ; i < n ; i++)
            traials[i].get() = traials[n + uniK(RNG)];  // copy values, not addresses
		return true;
	};

	do {
		// Run 'n' simulations
		for (size_t i = 0ul ; i < n && !halt ; i++) {
			jumpsLeft = simEffort;
			network.peak_simulation(traials[i], update, predicate);
		}
		// Sort to find 1-k/n quantile
		std::sort(begin(traials), begin(traials)+n,
				  [](Traial& lhs, Traial& rhs){return lhs.level < rhs.level;});
        newThr = traials[n-k].get().level;
	} while (!halt && newThr <= lastThr && reinit(traials));

	if (fails < ::NUM_FAILURES && !halt)
		fig::ModelSuite::tech_log("+");  // report success
	else
		newThr = lastThr;

    return newThr;
}

} // namespace  // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ThresholdsBuilderSMC::ThresholdsBuilderSMC() :
    ThresholdsBuilder("smc"),  // virtual inheritance forces this...
    ThresholdsBuilderAdaptiveSimple()
{ /* Not much to do around here */ }


void
ThresholdsBuilderSMC::build_thresholds_vector(const ImportanceFunction& impFun)
{
	const ImportanceValue IMP_RANGE = impFun.max_value() - impFun.min_value();
	if (IMP_RANGE < static_cast<ImportanceValue>(2u)) {
		// Too few importance levels: default to max possible # of thresholds
		std::vector< ImportanceValue >(IMP_RANGE+2u).swap(thresholds_);
		thresholds_[0u] = impFun.min_value();
		thresholds_[1u] = impFun.max_value();
		thresholds_[IMP_RANGE+1u] = impFun.max_value()
		                            + static_cast<ImportanceValue>(1u);
		return;
	}

    assert(0u < k_);
    assert(k_ < n_);
	assert(!halted_);

	if (thresholds_.size() > 0ul)
		std::vector< ImportanceValue >().swap(thresholds_);
	if (thresholds_.capacity() < MAX_NUM_THRESHOLDS)
		thresholds_.reserve(MAX_NUM_THRESHOLDS);

	TraialsVec traials = get_traials(n_+k_, impFun);
	const ModuleNetwork& network = *ModelSuite::get_instance().modules_network();

	// SMC initialization
	thresholds_.push_back(impFun.initial_value());  // start from initial state importance
	assert(thresholds_.back() < impFun.max_value());
	ModelSuite::tech_log("(seed:" + std::to_string(RNG_SEED) + ")");
	ImportanceValue newThreshold =
		find_new_threshold(network, impFun, traials, n_, k_, thresholds_.back(), halted_);
	if (impFun.max_value() <= newThreshold)
		ModelSuite::tech_log("\nFirst iteration of SMC reached max importance, "
							 "rare event doesn't seem so rare!\n");
	else if (newThreshold <= thresholds_.back())
		goto exit_with_fail;  // couldn't make it, so sad
	thresholds_.push_back(newThreshold);

	// SMC main loop
	while (thresholds_.back() < impFun.max_value()) {
        const ImportanceValue lastThr = thresholds_.back();
		// Find "initial states" (and clocks valuations) realizing last threshold
		if (!build_states_distribution(network, impFun, traials, n_, k_, lastThr, halted_)
		    || halted_)
			goto exit_with_fail;  // couldn't find those initial states
		// Find sims' 1-k/n quantile starting from those initial states
		newThreshold = find_new_threshold(network, impFun, traials, n_, k_, lastThr, halted_);
        // Use said quantile as new threshold if possible
		if (newThreshold <= thresholds_.back() || halted_)
			goto exit_with_fail;  // well, fuck
		thresholds_.push_back(newThreshold);
	}

	TraialPool::get_instance().return_traials(traials);
	thresholds_.push_back(impFun.max_value() + static_cast<ImportanceValue>(1u));
	ModelSuite::tech_log("\n");
	return;

	exit_with_fail:
		std::stringstream errMsg;
		errMsg << "Failed building thresholds. ";
		errMsg << "Importance of the ones found so far: ";
		for (const auto thr: thresholds_)
			errMsg << thr << ", ";
		errMsg << "\b\b  ";
		throw_FigException(errMsg.str());
}


void
ThresholdsBuilderSMC::tune(const size_t& numTrans,
						   const ImportanceValue& maxImportance,
						   const unsigned& splitsPerThr)
{
	ThresholdsBuilderAdaptiveSimple::tune(numTrans, maxImportance, splitsPerThr);
    // This algorith is statistically better (way better) than AMS,
	// resulting in the thresholds being chosen really close to each other.
	// The counterpart is that too many thresholds are chosen and thus the
    // thresholds building takes too long.
	// We try to counter that a little by reducing the probability of level up
	/// @note NOTE can we change for "theoretic optimal" p_i ~ e^-1  forall i ?
	///            See analysis by Garvels (PhD thesis) and Rubino&Tuffin (RES book)
	///            which derive this constant for optimal p_i = p^(-T) forall i
	///            where 'T' is the number of threshold levels and 'p_i' the
	///            probability of crossing the i-th level upwards
	///            Note the constant value e^-1 is subject to having the optimal
	///            number of thresholds T = -log(p)/2, which may not be the case
	const float p((k_*0.4f)/n_);
	k_ = std::round(p*n_);

	// Simulations length will be directly proportional to the probability 'p'
	// within the range (0.06 , 0.1)
	const uint128_t numStates =
	        ModelSuite::get_instance().modules_network()->concrete_state_size();
	const bool fewStates = numStates > uint128::uint128_0 && numStates < (1ul<<10u);
	const bool manyStates = numStates == uint128::uint128_0 || numStates > (1ul<<20u);
	::SIM_EFFORT = p < 0.06 || manyStates ? MIN_SIM_EFFORT :
	               p > 0.1  || fewStates  ? MAX_SIM_EFFORT :
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

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
