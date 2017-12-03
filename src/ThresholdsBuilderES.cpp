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
#include <cmath>  // std::log(), std::round(), etc.
#include <cassert>
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
    nSims_(0ul),
    maxSimLen_(0ul),
    property_(nullptr),
    model_(nullptr),
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
	model_ = ModelSuite::get_instance().modules_network().get();
	impFun_ = &impFun;

	// Adapted from Alg. 3 of Budde, D'Argenio, Hartmanns,
	// "Better Automated Importance Splitting for Transient Rare Events", 2017.
	tune();
	n_ = nSims_;
	ModelSuite::tech_log("Building thresholds with \"" + name
	                    + "\" for n = " + std::to_string(n_));

	// Probe for reachable importance values to select threshold candidates
	ImportanceVec thrCandidates = reachable_importance_values();
	if (thrCandidates.size() < 2ul)  // we must have reached beyond initial importance
		throw_FigException("ES could not find reachable importance values");
	const size_t DEFACTO_IMP_RANGE(thrCandidates.size()-1ul);
	std::vector<float> Pup(DEFACTO_IMP_RANGE), aux(DEFACTO_IMP_RANGE);
#ifndef NDEBUG
	ModelSuite::tech_log("\nFound " + std::to_string(DEFACTO_IMP_RANGE)
	                    + " relevant importance values:");
	for (auto imp: thrCandidates)
		ModelSuite::tech_log(" "+std::to_string(imp));
#endif

	// Estimate the probabilities of going from one (reachable) importance value to the next
#ifndef NDEBUG
	ModelSuite::tech_log("\nRunning internal Fixed Effort");
#endif
	ModelSuite::tech_log(" [");
	TraialsVec traials(get_traials(n_, impFun, false));
	for (size_t m = 1ul ; m < 5ul && 0.0f >= Pup.back() ; m++) {
		FE_for_ES(thrCandidates, traials, aux);
		for (size_t i = 0ul ; i < DEFACTO_IMP_RANGE && 0.0f < aux[i] ; i++)
			Pup[i] += (aux[i]-Pup[i])/m;
		if (0.0f >= Pup.back()) {
			ModelSuite::tech_log("-");
			// last iteration didn't reach max importance: Increase effort
			auto moreTraials(get_traials(traials.size(), impFun, false));
			traials.insert(end(traials), begin(moreTraials), end(moreTraials));
		} else {
			ModelSuite::tech_log("+");
		}
	}
	ModelSuite::tech_log("]");
	TraialPool::get_instance().return_traials(traials);
	if (0.0f >= Pup.back())
		artificial_thresholds_selection(thrCandidates, Pup);
#ifndef NDEBUG
	float est(1.0f);
	const auto defaultLogFlags(figTechLog.flags());
	figTechLog << std::setprecision(2) << std::scientific;
	figTechLog << "\nLvl-up probabilities:";
	for (auto p: Pup) {
		figTechLog << " " << p;
		est *= p;
	}
	figTechLog << "\nEstimate spoiler (transient properties only!): "
	           << est << std::endl;
	figTechLog.flags(defaultLogFlags);
#endif

	// Turn level-up probabilities into effort factors
	decltype(aux)& effort(aux);
	typedef decltype(aux)::value_type effort_t;
	std::fill(begin(effort), end(effort), static_cast<effort_t>(0));
	auto postprocess = [] (effort_t& e, const effort_t& MAX_FEASIBLE_EFFORT) {
		static constexpr auto SCALE_DOWN_COEFFICIENT(1.4f);
		e = std::min<float>(e/SCALE_DOWN_COEFFICIENT, MAX_FEASIBLE_EFFORT);
	};
	for (size_t i = 0ul ; i < Pup.size() ; i++) {
		float prev = effort[(i-1)%Pup.size()];
		effort[i] = 1.0f/Pup[i] + (prev-std::round(prev));  // expected # of sims reaching lvl i
		postprocess(effort[i], MAX_FEASIBLE_EFFORT);
	}

	// Select the thresholds based on the effort factors
	thresholds_.clear();
	thresholds_.emplace_back(impFun.initial_value(), 1ul);
	for (size_t i = 1ul ; i < Pup.size() ; i++) {
		const int thisEffort = std::round(effort[i]);
		if (1 < thisEffort)
			thresholds_.emplace_back(thrCandidates[i], thisEffort);
	}
	thresholds_.emplace_back(impFun.max_value()+static_cast<ImportanceValue>(1u), 1ul);

	ModelSuite::tech_log("\n");
	show_thresholds(thresholds_);
	impFun_ = nullptr;
	model_ = nullptr;
	return thresholds_;
}


ImportanceVec
ThresholdsBuilderES::reachable_importance_values() const
{
	static constexpr size_t NUM_INDEPENDENT_RUNS = 20ul;
	static constexpr size_t MAX_FAILS = (1ul)<<(10ul);
	size_t numFails(0ul);

	const ImportanceFunction& impFun(*impFun_);
	auto events_watcher = &fig::ThresholdsBuilderES::importance_seeker;

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
			model_->simulation_step(traial, *property_, *this, events_watcher);
			if (startImp < traial.level) {
				reachableImpValues.emplace(traial.level);
				maxImportanceReached = std::max(maxImportanceReached, traial.level);
				traialsNext.push_back(traial);
			} else {
				if (MAX_FAILS < ++numFails)
					goto end_reachability_analysis;  // assume max importance unreachable and quit
				else if (traialsNow.size() > 1ul)
					traial = traialsNow[numFails%traialsNow.size()].get();
				else
					traial.initialise(*model_,impFun);  // start over
				traialsNow.push_back(traial);
			}
			assert(traialsNow.size()+traialsNext.size() == NUM_INDEPENDENT_RUNS);
		}
		std::swap(traialsNow, traialsNext);
	} while (maxImportanceReached < impFun.max_value());
    end_reachability_analysis:
	    TraialPool::get_instance().return_traials(traialsNow);
		TraialPool::get_instance().return_traials(traialsNext);

	ImportanceVec result(begin(reachableImpValues), end(reachableImpValues));
	std::sort(begin(result), end(result));
	return result;
}


void
ThresholdsBuilderES::FE_for_ES(const ImportanceVec& reachableImportanceValues,
                               TraialsVec& traials,
                               std::vector<float>& Pup) const
{
	auto events_watcher = &fig::ThresholdsBuilderES::FE_watcher;
	const size_t N = traials.size();  // effort per level
	std::vector< Reference< Traial > > freeNow, freeNext, startNow, startNext;

	assert(0ul < N);
	if (reachableImportanceValues.size() < 2ul)
		return;  // only one reachable importance value: ES failed!
	assert(Pup.size() == reachableImportanceValues.size()-1ul);

	// Init ADTs
	std::fill(begin(Pup), end(Pup), 0.0f);
	freeNow.reserve(N);
	freeNext.reserve(N);
	startNow.reserve(N);
	startNext.reserve(N);
	for (Traial& t: traials)
		startNow.push_back(t.initialise(*model_,*impFun_));

	// For each reachable importance value 'i' ...
	for (auto i = 0ul ; i < reachableImportanceValues.size()-1ul && !startNow.empty() ; i++) {
		const auto& currImp = reachableImportanceValues[i];
		// ... run Fixed Effort until the next reachable importance value 'j' > 'i' ...
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
			//assert(traial.level >= currImp || startNow.size() < 2ul);
			traial.depth = 0;
			traial.numLevelsCrossed = 0;
			model_->simulation_step(traial, *property_, *this, events_watcher);
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

	if (reachableImportanceValues.size() < 2ul) {
		// There's *nothing*, try desperately to build some threshold
		auto moreValues = impFun_->random_sample(model_->initial_state());
		reachableImportanceValues.insert(end(reachableImportanceValues),
		                                 begin(moreValues), end(moreValues));
		std::sort(begin(reachableImportanceValues), end(reachableImportanceValues));
		std::unique(begin(reachableImportanceValues), end(reachableImportanceValues));
		if (reachableImportanceValues.size() < 2ul) {
			ModelSuite::tech_log("; cannot find *any* threshold!");
			return;
		}
		Pup.resize(reachableImportanceValues.size());
		ModelSuite::tech_log(" utterly");
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
		Pup[i] = std::max(trend, 1.0/MAX_FEASIBLE_EFFORT);  // bound max effort
		if (print)
			ModelSuite::tech_log(std::to_string(reachableImportanceValues[i-1]));
		print = false;
	}
}


void
ThresholdsBuilderES::tune(const size_t &,
                          const ImportanceValue &,
                          const unsigned &)
{
	assert(nullptr != model_);
	assert(nullptr != impFun_);

	// Factor [1], #(FE-sims) per iteration,
	// will be inversely proportional to the importance range
	// in the interval (3,20)
	const auto PP_EXP(impFun_->post_processing().type == PostProcessing::EXP);
	const auto PP_EXP_BASE(PP_EXP ? impFun_->post_processing().value : 0.0f);
	const auto normalise = [&PP_EXP,&PP_EXP_BASE] (const double& x)
	    { return PP_EXP ? (log(x)/log(PP_EXP_BASE)) : x; };
	const size_t IMP_RANGE(normalise(impFun_->max_value(true))
	                       - normalise(impFun_->initial_value(true)));
	nSims_ = IMP_RANGE <  3ul ? MAX_NSIMS :
	         IMP_RANGE > 20ul ? MIN_NSIMS :
	         std::round((MIN_NSIMS*0.0588f-MAX_NSIMS*0.0588f) * IMP_RANGE
	                    + 1.176f*MAX_NSIMS - 0.176f*MIN_NSIMS);
	assert(nSims_ >= MIN_NSIMS);
	assert(nSims_ <= MAX_NSIMS);

	// Factor [2], #(steps) per FE-sim,
	// will be inversely proportional to the model size,
	// understood as the #(clocks)+log(#(concrete_states)),
	// in the interval (1K,5K)
	const uint128_t NSTATES(model_->concrete_state_size());
	const auto LOG_STATES(std::log(NSTATES.lower())+64*std::log(1+NSTATES.upper()));
	const size_t MODEL_SIZE(model_->num_clocks()+LOG_STATES);
	const auto ONE_K(1ul<<10ul);
	maxSimLen_ = MODEL_SIZE <     (ONE_K) ? MAX_SIM_LEN :
	             MODEL_SIZE > (5ul*ONE_K) ? MIN_SIM_LEN :
	             std::round((MIN_SIM_LEN*0.25-MAX_SIM_LEN*0.25) * MODEL_SIZE
	                        + 1.25f*MAX_SIM_LEN - 0.25f*MIN_SIM_LEN);
	assert(maxSimLen_ >= MIN_SIM_LEN);
	assert(maxSimLen_ <= MAX_SIM_LEN);
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
