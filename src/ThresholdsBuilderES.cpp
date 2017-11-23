//==============================================================================
//
//  ThresholdsBuilderES.cpp
//
//  Copyleft 2017-
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
#include <cmath>
// C++
#include <vector>
#include <deque>
#include <algorithm>  // std::fill()
#include <iomanip>  // std::setprecision()
// FIG
#include <ThresholdsBuilderES.h>
#include <ImportanceFunction.h>
#include <PropertyTransient.h>
#include <PropertyRate.h>
#include <ModuleNetwork.h>
#include <ModelSuite.h>
#include <TraialPool.h>
#include <Traial.h>
#include <FigLog.h>

// ADL
using std::begin;
using std::end;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

ThresholdsBuilderES::ThresholdsBuilderES(const size_t& n) :
    ThresholdsBuilder("es"),  // virtual inheritance forces this...
    ThresholdsBuilderAdaptive(n),
    property_(nullptr),
    impFun_(nullptr)
{ /* Not much to do around here */ }


void
ThresholdsBuilderES::setup(const PostProcessing &,
                           std::shared_ptr<const Property> property,
                           const unsigned)
{ property_ = property; }


ThresholdsVec
ThresholdsBuilderES::build_thresholds(const ImportanceFunction& impFun)
{
	impFun_ = &impFun;

	// Adapted from Alg. 3 of Budde, D'Argenio, Hartmanns,
	// "Better Automated Importance Splitting for Transient Rare Events", 2017.
	ModelSuite::tech_log("Building thresholds with \"" + name
	                    + "\" for n = " + std::to_string(n_));

	// Probe for reachable importance values to select threshold candidates
	ImportanceVec thrCandidates = reachable_importance_values();
	const size_t DEFACTO_IMP_RANGE(thrCandidates.size());
	std::vector<float> Pup(DEFACTO_IMP_RANGE), aux(DEFACTO_IMP_RANGE);
#ifndef NDEBUG
	ModelSuite::tech_log("\nFound " + std::to_string(DEFACTO_IMP_RANGE)
	                    + " relevant importance values:");
	for (auto imp: thrCandidates)
		ModelSuite::tech_log(" "+std::to_string(imp));
	ModelSuite::tech_log("\nRunning internal Fixed Effort");
#endif

	// Estimate the probabilities of going from one (reachable) importance value to the next
	TraialsVec traials(get_traials(n_, impFun, false));
	for (size_t m = 1ul; m < 5ul || (m < 3ul && 0.0f >= Pup.back()); m++) {
		// until we iterate four times, or twice and reach max importance
		FE_for_ES(thrCandidates, traials, aux);
		for (size_t i = 0ul ; i < DEFACTO_IMP_RANGE && 0.0f < aux[i] ; i++)
			Pup[i] += (aux[i]-Pup[i])/m;
		ModelSuite::tech_log(".");
	}
	TraialPool::get_instance().return_traials(traials);
	if (0.0f >= Pup.back())
		artificial_thresholds_selection(thrCandidates, Pup);
#ifndef NDEBUG  // Some debug print
	float est(1.0f);
	const auto defaultLogFlags(figTechLog.flags());
	figTechLog << std::setprecision(2) << std::scientific;
	figTechLog << "\nLvl-up probabilities:";
	for (auto p: Pup) {
		figTechLog << " " << p;
		est *= p;
	}
	figTechLog << "\nRare event probability spoiler: " << est << std::endl;
	figTechLog.flags(defaultLogFlags);
#endif

	// Turn level-up probabilities into splitting factors
	decltype(aux)& split(aux);
	std::fill(begin(split), end(split), 0.0f);
	for (size_t i = 0ul ; i < Pup.size() ; i++) {
		float prev = split[(i-1)%Pup.size()];
		split[i] = 1.0f/Pup[i] + (prev-std::round(prev));  // expected # of sims reaching lvl i
	}

	// Select the thresholds based on the splitting factors
	thresholds_.clear();
	thresholds_.emplace_back(impFun.initial_value(), 1ul);
	for (size_t i = 1ul ; i < Pup.size() ; i++) {
		const int thisSplit = std::round(split[i]);
		if (1 < thisSplit)
			thresholds_.emplace_back(thrCandidates[i], thisSplit);
	}
	thresholds_.emplace_back(impFun.max_value()+static_cast<ImportanceValue>(1u), 1ul);

	ModelSuite::tech_log("\n");
	show_thresholds(thresholds_);

	impFun_ = nullptr;
	return thresholds_;
}


ImportanceVec
ThresholdsBuilderES::reachable_importance_values() const
{
	static constexpr size_t NUM_INDEPENDENT_RUNS = 20ul;
	static constexpr size_t MAX_FAILS = (1ul)<<(10ul);  // 1 K
	size_t numFails(0ul);

	const ImportanceFunction& impFun(*impFun_);
	auto FE_watcher = &fig::ThresholdsBuilderES::FE_watcher;
	const ModuleNetwork& network(*ModelSuite::get_instance().modules_network());

	Traial& backupTraial(TraialPool::get_instance().get_traial());
	ImportanceValue maxImportanceReached(impFun.initial_value());
	std::unordered_set< ImportanceValue > reachableImpValues = {impFun.initial_value()};
	TraialsVec traialsNow(get_traials(NUM_INDEPENDENT_RUNS, impFun)),
	           traialsNext;
	assert(!traialsNow.empty());
	assert(traialsNext.empty());
	traialsNext.reserve(traialsNow.size());

	do {
		while (!traialsNow.empty()) {
			Traial& traial(traialsNow.back());
			traialsNow.pop_back();
			const ImportanceValue startImp(traial.level);
			traial.depth = 0;
			traial.numLevelsCrossed = 0;
			backupTraial = traial;
			network.simulation_step(traial, *property_, *this, FE_watcher);
			if (startImp < traial.level) {

				reachableImpValues.emplace(traial.level);
				maxImportanceReached = std::max(maxImportanceReached, traial.level);
				traialsNext.push_back(traial);
			} else {
				if (MAX_FAILS < ++numFails)
					goto fuck_this_shit;  // assume max importance unreachable and quit
				else if (traialsNow.size() > 1ul)
					traial = traialsNow[numFails%traialsNow.size()].get();
				else
					traial = backupTraial;  // a new chance
				traialsNow.push_back(traial);
			}
			assert(traialsNow.size()+traialsNext.size() == NUM_INDEPENDENT_RUNS);
		}
		std::swap(traialsNow, traialsNext);
		numFails = 0ul;
	} while (maxImportanceReached < impFun.max_value());
    fuck_this_shit:
	    TraialPool::get_instance().return_traials(traialsNow);
		TraialPool::get_instance().return_traials(traialsNext);
		TraialPool::get_instance().return_traial(backupTraial);

	ImportanceVec result(begin(reachableImpValues), end(reachableImpValues));
	std::sort(begin(result), end(result));
	return result;
}


void
ThresholdsBuilderES::FE_for_ES(const ImportanceVec& reachableImportanceValues,
                               TraialsVec& traials,
                               std::vector<float>& Pup) const
{
	auto FE_watcher = &fig::ThresholdsBuilderES::FE_watcher;
	const ModuleNetwork& network(*ModelSuite::get_instance().modules_network());
	const size_t N = traials.size();  // effort per level
	std::vector< Reference< Traial > > freeNow, freeNext, startNow, startNext;

	assert(0ul < N);
	assert(Pup.size() >= reachableImportanceValues.size());

	// Init ADTs
	std::fill(begin(Pup), end(Pup), 0.0f);
	freeNow.reserve(N);
	freeNext.reserve(N);
	startNow.reserve(N);
	startNext.reserve(N);
	for (Traial& t: traials) {
		t.initialise(network,*impFun_);
		startNow.push_back(t);
	}

	// For each reachable importance value 'i' ...
	for (size_t i = 0ul ; i < reachableImportanceValues.size() && !startNow.empty() ; i++) {
		const auto& currImp = reachableImportanceValues[i];
		// ... run Fixed Effort until the next rechable importance value 'j' > 'i' ...
		size_t startNowIdx(0ul);
		while ( ! (freeNow.empty() && startNow.empty()) ) {
			// (Traial fetching for simulation)
			const bool useFree = !freeNow.empty();
			Traial& traial( useFree ? freeNow.back() : startNow.back());
			if (useFree) {
				traial = startNow[startNowIdx].get();  // copy *contents*
				++startNowIdx %= std::max(1ul,startNow.size());
				freeNow.pop_back();
			} else {
				startNow.pop_back();
			}
			// (simulation & bookkeeping)
//			traial.level = currImp;  // enforce this
			assert(traial.level >= currImp);
			traial.depth = 0;
			traial.numLevelsCrossed = 0;
			network.simulation_step(traial, *property_, *this, FE_watcher);
			if (traial.level > currImp)
				startNext.push_back(traial);
			else
				freeNext.push_back(traial);
		}
		// ... and estimate the probability of reaching 'j' from 'i'
		Pup[i] = static_cast<float>(startNext.size()) / N;
		std::swap(freeNow, freeNext);
		std::swap(startNow, startNext);
	}
}


void
ThresholdsBuilderES::artificial_thresholds_selection(
        ImportanceVec& reachableImportanceValues,
        std::vector< float >& Pup) const
{
	assert(reachableImportanceValues.size() == Pup.size());
	ModelSuite::tech_log("\nExpected Success failed");

	if (reachableImportanceValues.size() < 3ul) {
		// There's *nothing*, try desperately to build some threshold
		auto moreValues = impFun_->random_sample(
		            ModelSuite::get_instance().modules_network()->initial_state());
		moreValues.insert(begin(reachableImportanceValues), end(reachableImportanceValues));
		reachableImportanceValues.clear();
		reachableImportanceValues.insert(end(reachableImportanceValues),
		                                 begin(moreValues), end(moreValues));
		if (reachableImportanceValues.size() < 2ul) {
			ModelSuite::tech_log("; cannot find *any* threshold!");
			return;
		}
		Pup.resize(reachableImportanceValues.size());
	}
	if (0.0f >= Pup[0]) {
		Pup[0] = 5.0e-2;
		Pup[1] = 1.0e-3;
		ModelSuite::tech_log(" on the first iteration");
	} else if (0.0 >= Pup[1]) {
		Pup[1] = Pup[0]/10.0f;
		ModelSuite::tech_log(" on the second iteration");
	}

	ModelSuite::tech_log("!\nResorting to fixed choice of thresholds starting "
	                     "above the ImportanceValue ");
	/// @todo Implement regresive average to follow the trend of the probabilities
	bool print(true);
	for (size_t i = 2ul ; i < Pup.size() ; i++) {
		if (0.0 < Pup[i])
			continue;  // this value was chosen by Expected Success
		const auto HI = std::max(Pup[i-1], Pup[i-2]),
		           LO = std::min(Pup[i-1], Pup[i-2]);
		const double trend = std::pow(LO,2.0)/HI;
		Pup[i] = std::max(trend, 1e-2);  // bound the max effort we can choose
		if (print)
			ModelSuite::tech_log(std::to_string(reachableImportanceValues[i-1])+"+\n");
		print = false;
	}
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
