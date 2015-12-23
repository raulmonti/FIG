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
	triggeringClock_(that.triggeringClock_),
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
	triggeringClock_(std::move(that.triggeringClock_)),
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


// Transition&
// Transition::operator=(Transition that)
// {
// 	std::swap(label_, that.label_);
// 	std::swap(triggeringClock_, that.triggeringClock_);
// 	std::swap(pre, that.pre);
// 	std::swap(pos, that.pos);
// 	std::swap(resetClocksData_, that.resetClocksData_);
// 	switch (resetClocksData_) {
// 	case CARBON:
// 		std::swap(resetClocksList_, that.resetClocksList_);
// 		break;
// 	case CRYSTAL:
// 		std::swap(resetClocks_, that.resetClocks_);
// 		break;
// 	}
// 	return *this;
// }


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


void
Transition::crystallize(const PositionsMap& globalClocks)
{
	if (CRYSTAL == resetClocksData_)
#ifndef NDEBUG
		throw FigException("crystallize had already been called before");
#else
		return;
#endif

	// Encode as Bitflag the global positions of the clocks to reset
	Bitflag indexedPositions(static_cast<Bitflag>(0u));
	for(const auto& clockName: resetClocksList_) {
#ifndef NRANGECHK
		unsigned idx = globalClocks.at(clockName);
		if (8*sizeof(Bitflag) < idx) {
			std::stringstream errMsg;
			errMsg << "invalid clock index: " << idx;
			errMsg << " -- Indices can range up to " << 8*sizeof(Bitflag);
			throw std::out_of_range(errMsg.str());
		}
#else
		unsigned idx = globalClocks[clockName];
#endif
		indexedPositions |= static_cast<Bitflag>(1u) << idx;
	}
	assert((static_cast<Bitflag>(0u) != indexedPositions) !=
			(begin(resetClocksList_) == end(resetClocksList_)));

	// Discard carbon and store crystal version
	resetClocksList_.~vector< std::string >();
	new (&resetClocks_) Bitflag;
	std::swap(indexedPositions, resetClocks_);
	resetClocksData_ = CRYSTAL;

	// As a courtesy, check mapping of triggering clock, if any
#ifndef NRANGECHK
	if (!triggeringClock_.empty() &&
			end(globalClocks) == globalClocks.find(triggeringClock_)) {
		std::stringstream errMsg;
		errMsg << "triggering clock \"" << triggeringClock_;
		errMsg << "\" wasn't found in the global clocks map";
		throw std::out_of_range(errMsg.str());
	}
#endif
}

} // namespace fig