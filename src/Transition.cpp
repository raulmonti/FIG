//==============================================================================
//
//  Transition.cpp
//
//  Copyleft 2015-
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


// C++
#include <random>
#include <sstream>    // std::stringstream
#include <stdexcept>  // std::out_of_range
#include <type_traits>
// FIG
#include <Transition.h>


// ADL
using std::begin;
using std::end;

namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

// RNG for probabilistic branch choice  ///////////////////////////
std::random_device MT_nondet_RNG;  // Mersenne-Twister nondeterministic RNG seeding
unsigned long RNG_SEED =
#ifdef RANDOM_RNG_SEED
    MT_nondet_RNG();
#else
    std::mt19937::default_seed;
#endif

std::mt19937 RNG(RNG_SEED);
std::uniform_real_distribution<float> uniform(0.0f, 1.0f);

} // namespace  // // // // // // // // // // // // // // // // // // // // //


namespace fig
{


Transition::Transition(const Transition& that) :
	label_(that.label_),
	triggeringClock(that.triggeringClock),
	pre(that.pre),
    probabilities(that.probabilities),
    posts(that.posts),
    numClocks(that.numClocks),
	resetClocksData_(that.resetClocksData_)
{
	switch (resetClocksData_) {
	case CARBON:
		new (&resetClocksList_) std::vector< std::string >;
		resetClocksList_ = that.resetClocksList_;
		break;
	case CRYSTAL:
		new (&resetClocks_) std::vector< Bitflag >;
		resetClocks_ = that.resetClocks_;
		break;
	}
}


Transition::Transition(Transition&& that) :
	label_(std::move(that.label_)),
	triggeringClock(std::move(that.triggeringClock)),
    pre(std::move(that.pre)),
    probabilities(std::move(that.probabilities)),
    posts(std::move(that.posts)),
    numClocks(std::move(that.numClocks)),
    resetClocksData_(std::move(that.resetClocksData_))
{
	switch (resetClocksData_) {
	case CARBON:
		new (&resetClocksList_) std::vector< std::string >;
		std::swap(resetClocksList_, that.resetClocksList_);
		break;
	case CRYSTAL:
		new (&resetClocks_) std::vector< Bitflag >;
		std::swap(resetClocks_, that.resetClocks_);
		break;
	}
}


Transition::~Transition()
{
	switch (resetClocksData_) {
	case CARBON:
		resetClocksList_.~vector< std::set< std::string > >();
		break;
	case CRYSTAL:
		resetClocks_.~vector< Bitflag >();
		break;
	}
}


std::vector< std::string >
Transition::reset_clocks_names_list() const noexcept
{
	auto& allClocksNames(allResetClocksNames);
	if (allClocksNames.size() != numClocks) {
		std::set< std::string > allClocks;
		for (auto i = 0ul ; i < num_branches() ; i++)
			allClocks.insert(begin(resetClocksList_[i]), end(resetClocksList_[i]));
		allClocksNames.insert(begin(allClocksNames), begin(allClocks), end(allClocks));
		assert(allClocksNames.size() == numClocks);
	}
	return allClocksNames;
}


const Bitflag&
Transition::reset_clocks_list() const noexcept
{
	auto& allClocks(allResetClocks[0]);
	if (CRYSTAL == resetClocksData_ &&
	        allClocks.none() && 0ul < numClocks) {
		for (auto i = 0ul ; i < num_branches() ; i++)
			allClocks |= resetClocks_[i];
		assert(allClocks.count() == numClocks);
	}
	return allClocks;
}


template< typename Integral >
void
Transition::apply_postcondition(Traial& traial,
                                const std::vector< Clock >& clocks,
                                Integral firstClock) const
{
	static_assert(std::is_integral<Integral>::value,
	              "ERROR: Integral must be an integral type");
	assert(0 <= static_cast<long>(firstClock));
	const size_t fClk = static_cast<size_t>(firstClock),
	             nClk = clocks.size();
	// Choose a branch...
	auto branch = 0ul;
	if (num_branches() > 1) {
		const auto p = uniform(RNG);
		assert(0.0f < p);
		assert(p < 1.0f);
		while (p > probabilities[branch])
			branch++;
#ifndef NDEBUG
		if (branch >= num_branches())
			throw_FigException("Invalid branch chosen, bad probabilistic weights?");
#endif
	}
	// ...apply its postcondition...
	posts[branch](traial.state);
	// ...and reset (only) the corresponing clocks
	auto& clocksValues = traial.clocks_values();
	assert(fClk+nClk <= clocksValues.size());
	for (auto i = fClk ; i < fClk + nClk ; i++) {
		clocksValues[i].first.get() =
#ifndef NDEBUG
		    resetClocks_[branch].test(i)
#else
		    resetClocks_[branch][i]
#endif
		        ? clocks[i-fClk].sample()       // yep, sample anew
		        : clocksValues[i].first.get();  // nope
	}
	for (auto i = fClk ; i < fClk + nClk ; i++) {
		clocksValues[i].second.get() =
#ifndef NDEBUG
		    resetClocks_[branch].test(i)
#else
		    resetClocks_[branch][i]
#endif
		        ? clocksValues[i].first.get()   // yep, take new sample
		        : clocksValues[i].second.get(); // nope
	}
}
// Specialisations
typedef const std::vector< Clock >& CVCR_;
template void Transition::apply_postcondition(Traial&, CVCR_, short) const;
template void Transition::apply_postcondition(Traial&, CVCR_, int) const;
template void Transition::apply_postcondition(Traial&, CVCR_, long) const;
template void Transition::apply_postcondition(Traial&, CVCR_, unsigned short) const;
template void Transition::apply_postcondition(Traial&, CVCR_, unsigned int) const;
template void Transition::apply_postcondition(Traial&, CVCR_, unsigned long) const;


State< STATE_INTERNAL_TYPE >
Transition::apply_postcondition(const State< STATE_INTERNAL_TYPE >& state,
                                int chosenBranch) const
{
	if (0 > chosenBranch || chosenBranch > static_cast<int>(num_branches())) {
		// Choose a branch probabilistically
		const auto p = uniform(RNG);
		assert(0.0f < p);
		assert(p < 1.0f);
		auto branch = 0ul;
		while (branch+1 < num_branches() && p < probabilities[branch+1])
			branch++;
#ifndef NDEBUG
		if (branch >= probabilities.size())
			throw_FigException("Invalid branch chosen, bad probabilistic weights?");
#endif
		chosenBranch = static_cast<int>(branch);
	}
	// Return the state resulting from apply the postcondition of the chosen branch
	auto newState = state;
//	State< STATE_INTERNAL_TYPE > newState = state;
	posts[static_cast<size_t>(chosenBranch)](newState);
	return newState;
}


std::set< State< STATE_INTERNAL_TYPE > >
Transition::apply_postconditions(const State< STATE_INTERNAL_TYPE >& state) const
{
	std::set< State< STATE_INTERNAL_TYPE > > allNewStates;
	for (const auto& pos: posts) {
		auto newState = state;
		pos(newState);
		allNewStates.insert(newState);
	}
	return allNewStates;
}


#ifndef NRANGECHK
void
Transition::crystallize(const PositionsMap& globalClocks)
#else
void
Transition::crystallize(PositionsMap& globalClocks)
#endif
{
	if (CRYSTAL == resetClocksData_)
#ifndef NDEBUG
		throw_FigException("crystallize had already been called before");
#else
		return;
#endif

	// Encode as Bitflag the global positions of the clocks to reset by branch
	std::vector< Bitflag > indexedPositions(resetClocksList_.size());
	for (auto i=0ul ; i < resetClocksList_.size() ; i++ ) {
//	for (const auto& list: resetClocksList()) {
		for(const auto& clockName: resetClocksList_[i]) {
#       ifndef NRANGECHK
			const size_t idx = globalClocks.at(clockName);
			indexedPositions[i].test(idx);  // check clock index validity
#       else
			const size_t idx = globalClocks[clockName];
#       endif
			indexedPositions[i][idx] = true;
		}
		assert(indexedPositions[i].any() !=
		        (begin(resetClocksList_[i]) == end(resetClocksList_[i])));
	}

	// Discard carbon and store crystal version
	resetClocksList_.~vector< std::set< std::string > >();
	new (&resetClocks_) vector< Bitflag >;
	std::swap(indexedPositions, resetClocks_);
	resetClocksData_ = CRYSTAL;

	// Check mapping of triggering clock, if any
#ifndef NRANGECHK
	if (!triggeringClock.empty() &&
			end(globalClocks) == globalClocks.find(triggeringClock)) {
		std::stringstream errMsg;
		errMsg << "triggering clock \"" << triggeringClock;
		errMsg << "\" wasn't found in the global clocks map";
		throw std::out_of_range(errMsg.str());
	}
#endif
}

} // namespace fig
