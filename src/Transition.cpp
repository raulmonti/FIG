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
#include <sstream>    // std::stringstream
#include <stdexcept>  // std::out_of_range
// FIG
#include <Transition.h>

// ADL
using std::begin;
using std::end;


namespace fig
{

Transition::Transition(const Transition& that) :
	label_(that.label_),
	triggeringClock(that.triggeringClock),
	pre(that.pre),
	pos(that.pos),
	resetClocksData_(that.resetClocksData_)
{
	switch (resetClocksData_) {
	case CARBON:
		new (&resetClocksList_) std::vector< std::string >;
		resetClocksList_ = that.resetClocksList_;
		break;
	case CRYSTAL:
		new (&resetClocks_) Bitflag;
		resetClocks_ = that.resetClocks_;
		break;
	}
}


Transition::Transition(Transition&& that) :
	label_(std::move(that.label_)),
	triggeringClock(std::move(that.triggeringClock)),
	pre(std::move(that.pre)),
	pos(std::move(that.pos)),
	resetClocksData_(std::move(that.resetClocksData_))
{
	switch (resetClocksData_) {
	case CARBON:
		new (&resetClocksList_) std::vector< std::string >;
		std::swap(resetClocksList_, that.resetClocksList_);
		break;
	case CRYSTAL:
		new (&resetClocks_) Bitflag;
		std::swap(resetClocks_, that.resetClocks_);
		break;
	}
}


Transition::~Transition()
{
	switch (resetClocksData_) {
	case CARBON:
		resetClocksList_.~vector< std::string >();
		break;
	case CRYSTAL:
		resetClocks_.~Bitflag();
		break;
	}
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

	// Encode as Bitflag the global positions of the clocks to reset
	Bitflag indexedPositions;
	for(const auto& clockName: resetClocksList_) {
#ifndef NRANGECHK
		const size_t idx = globalClocks.at(clockName);
		indexedPositions.test(idx);  // check clock index validity
#else
		const size_t idx = globalClocks[clockName];
#endif
		indexedPositions[idx] = true;
	}
	assert(indexedPositions.any() !=
			(begin(resetClocksList_) == end(resetClocksList_)));

	// Discard carbon and store crystal version
	resetClocksList_.~vector< std::string >();
	new (&resetClocks_) Bitflag;
	std::swap(indexedPositions, resetClocks_);
	resetClocksData_ = CRYSTAL;

	// As a courtesy, check mapping of triggering clock, if any
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
