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
#include <algorithm>
// FIG
#include <ThresholdsBuilderES.h>
#include <ImportanceFunction.h>
#include <PropertyTransient.h>
#include <PropertyRate.h>
#include <ModuleNetwork.h>
#include <ModelSuite.h>
#include <TraialPool.h>
#include <Traial.h>


// ADL
using std::begin;
using std::end;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

//	namespace  // // // // // // // // // // // // // // // // // // // // // // //
//	{
//
//	using fig::Traial;
//	using fig::ImportanceValue;
//	using TraialsVec = fig::ThresholdsBuilderAdaptive::TraialsVec;
//
//

//	} // namespace  // // // // // // // // // // // // // // // // // // // // //
//
//


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
	const ImportanceValue IMP_RANGE(impFun.max_value()-impFun.initial_value());
	std::vector<float> Pup(IMP_RANGE), aux(IMP_RANGE);

	// Adapted from Alg. 3 of Budde, D'Argenio, Hartmanns,
	// "Better Automated Importance Splitting for Transient Rare Events", 2017.
	ModelSuite::tech_log("Building thresholds with \"" + name
	                    + "\" for n = " + std::to_string(n_)
	                    + "\nRunning FE on all adjacent importance values");

	// Roughly estimate the level-up probabilities of all importance levels
	impFun_ = &impFun;
	TraialsVec traials(get_traials(n_, impFun, false));
	do {
		static size_t m(0ul);
		m++;
		FE_for_ES(impFun, traials, aux);
		for (size_t i = 0ul ; i < IMP_RANGE && 0.0f < aux[i] ; i++)
			Pup[i] += (aux[i]-Pup[i])/m;
		ModelSuite::tech_log(".");
	} while (0.0f >= Pup.back());  // "until we reach the max importance"
	TraialPool::get_instance().return_traials(traials);

	// Turn level-up probabilities into splitting factors
	decltype(aux)& split(aux);
	std::fill(begin(split), end(split), 0.0f);
	for (size_t i = 0ul ; i < IMP_RANGE ; i++) {
		float prev = split[(i-1)%IMP_RANGE];
		split[i] = 1.0f/Pup[i] + (prev-std::round(prev));  // expected # of sims reaching lvl i
	}

	// Select the thresholds based on the splitting factors
	thresholds_.clear();
	thresholds_.emplace_back(impFun.initial_value(), 1ul);
	for (ImportanceValue i = impFun.initial_value()+1 ; i <= impFun.max_value() ; i++) {
		const int thisSplit = std::round(split[i]);
		if (1 < thisSplit)
			thresholds_.emplace_back(i, thisSplit);
	}
	thresholds_.emplace_back(impFun.max_value()+static_cast<ImportanceValue>(1u), 1ul);

	ModelSuite::tech_log("\n");
	show_thresholds(thresholds_);
	impFun_ = nullptr;

	return thresholds_;
}


void
ThresholdsBuilderES::FE_for_ES(const fig::ImportanceFunction& impFun,
                               TraialsVec& traials,
                               std::vector<float>& Pup)
{
	auto FE_watcher = &fig::ThresholdsBuilderES::FE_watcher;
	const ModuleNetwork& network(*fig::ModelSuite::get_instance().modules_network());
	const size_t N = traials.size();  // effort per level
	std::vector< Reference< Traial > > freeNow, freeNext, startNow, startNext;

	assert(0ul < N);
	assert(impFun.initial_value() < impFun.max_value());
	assert(Pup.size() >= impFun.max_value() - impFun.initial_value());

	// Init ADTs
	std::fill(begin(Pup), end(Pup), 0.0f);
	freeNow.reserve(N);
	freeNext.reserve(N);
	startNow.reserve(N);
	startNext.reserve(N);
	for (Traial& t: traials) {
		t.initialise(network,impFun);
		startNow.push_back(t);  // could use 'freeNow' as well
	}

	// For each importance value 'i' ...
	for (ImportanceValue i = impFun.initial_value() ;
	     i < impFun.max_value() && !startNow.empty() ;
	     i++)
	{
		// ... run Fixed Effort until the next importance value 'i+1' ...
		size_t startNowIdx(0ul);
		while ( ! (freeNow.empty() && startNow.empty()) ) {
			// (Traial fetching for simulation)
			const bool useFree = !freeNow.empty();
			Traial& traial( useFree ? freeNow.back() : startNow.back());
			if (useFree) {
				freeNow.pop_back();
				traial = startNow[startNowIdx++].get();  // just copy contents!
				startNowIdx %= startNow.size();
			} else {
				startNow.pop_back();
			}
			// (simulation & bookkeeping)
			assert(traial.level == i);
			traial.depth = 0;
			traial.numLevelsCrossed = 0;
			network.simulation_step(traial, *property_, *this, FE_watcher);

			/// @todo TODO erase debug print
//			std::cerr << "lvl: " << traial.level
//					  << "  steps: " << traial.numLevelsCrossed << std::endl;

			if (traial.level > i)
				startNext.push_back(traial);
			else
				freeNext.push_back(traial);
		}

		/// @todo TODO erase debug print
		std::cerr << startNext.size() << "/" << N << endl;

		// ... and estimate the probability of reaching 'i+1' from 'i'
		Pup[i] = startNext.size() / N;
		std::swap(freeNow, freeNext);
		std::swap(startNow, startNext);
	}
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
