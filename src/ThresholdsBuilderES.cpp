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
	                    + "\nRunning FE on all " + std::to_string(IMP_RANGE)
	                    + " adjacent importance values");

	// Roughly estimate the level-up probabilities of all importance levels
	impFun_ = &impFun;
	TraialsVec traials(get_traials(n_, impFun, false));
	for (size_t m = 1ul; m < 3ul || (m < 2ul && 0.0f >= Pup.back()); m++) {
		// until we iterate seven times, or
		// until we iterate three times and reach max importance
		FE_for_ES(impFun, traials, aux);
		for (size_t i = 0ul ; i < IMP_RANGE && 0.0f < aux[i] ; i++)
			Pup[i] += (aux[i]-Pup[i])/m;
		ModelSuite::tech_log(".");
	}
	TraialPool::get_instance().return_traials(traials);
	if (0.0f >= Pup.back())
		artificial_thresholds_selection(Pup);  // didn't reach max importance
#ifndef NDEBUG
	// Some debug print
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
				traial = startNow[startNowIdx++].get();  // just copy contents!
				startNowIdx %= startNow.size();
				freeNow.pop_back();
			} else {
				startNow.pop_back();
			}
			// (simulation & bookkeeping)
			assert(traial.level == i);
			traial.depth = 0;
			traial.numLevelsCrossed = 0;
			network.simulation_step(traial, *property_, *this, FE_watcher);
			if (traial.level > i)
				startNext.push_back(traial);
			else
				freeNext.push_back(traial);
		}
		// ... and estimate the probability of reaching 'i+1' from 'i'
		Pup[i] = static_cast<float>(startNext.size()) / N;
		std::swap(freeNow, freeNext);
		std::swap(startNow, startNext);
	}
}


void
ThresholdsBuilderES::artificial_thresholds_selection(std::vector< float >& Pup) const
{
	assert(1ul < Pup.size());  // we require at least two importance levels
	assert(0.0f < Pup.front());
	ModelSuite::tech_log("\nResorting to fixed choice of thresholds starting "
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
			ModelSuite::tech_log(std::to_string(i-1)+"\n");
		print = false;
	}
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
