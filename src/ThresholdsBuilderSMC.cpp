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
#include <algorithm>  // std::sort(), std::max({})
// FIG
#include <ThresholdsBuilderSMC.h>
#include <ModelSuite.h>


namespace
{

typedef  std::vector< fig::Reference< fig::Traial > >  TraialsVec;

/// Min simulation length (in # of jumps) to find new thresholds
const unsigned MIN_SIM_EFFORT = 1u<<6;  // 64

/// Max # of failures allowed when searching for a new threshold
const unsigned MAX_NUM_FAILURES = 8u;


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


	/// @todo TODO continue from here implementing Sequential Monte Carlo
	///            according to the '11 paper


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

ThresholdsBuilderSMC::ThresholdsBuilderSMC() : ThresholdsBuilderAdaptive("smc")
{ /* Not much to do around here */ }


void
ThresholdsBuilderSMC::build_thresholds_vector(const ImportanceFunction& impFun)
{
	assert(0 < k_);
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

	unsigned failures(0u);
	unsigned simEffort(MIN_SIM_EFFORT);
	std::vector< ImportanceValue >().swap(thresholds_);
	thresholds_.reserve((impFun.max_value()-impFun.min_value()) / 5u);  // magic
	auto lesser = [](const Traial& lhs, const Traial& rhs)
				  { return lhs.level < rhs.level; };
	TraialsVec traials = ThresholdsBuilderAdaptive::get_traials(n_, impFun);
	const ModuleNetwork& network = *ModelSuite::get_instance().modules_network();

	// SMC initialization

	throw_FigException("TODO implement SMC algorithm from Guyader and Cerou's paper");







	thresholds_.push_back(impFun.min_value());
	Traial& kTraial = traials[n_-k_];
	do {
		simulate(network, impFun, traials, n_, simEffort);
		std::sort(begin(traials), end(traials), lesser);
		kTraial = traials[n_-k_];
		simEffort *= 2;
	} while (thresholds_.back() == kTraial.level);
	if (impFun.max_value() <= kTraial.level)
		throw_FigException("first iteration of SMC reached max importance, "
						   "rare event doesn't seem rare enough.");
	thresholds_.push_back(kTraial.level);
	simEffort = MIN_SIM_EFFORT;
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
