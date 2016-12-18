//==============================================================================
//
//  Postcondition.cpp
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
#include <utility>   // std::swap()
#include <iterator>  // std::begin(), std::end()
#include <iostream>
// FIG
#include <Postcondition.h>

// ADL
using std::swap;
using std::begin;
using std::end;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

Postcondition&
Postcondition::operator=(Postcondition that) {
    /*MathExpression::operator=(std::move(that));
	swap(NUPDATES_, that.NUPDATES_);
	swap(updatesNames_, that.updatesNames_);
	swap(updatesPos_, that.updatesPos_);
    return *this;*/
    return *this;
}

void
Postcondition::operator()(State<STATE_INTERNAL_TYPE>& state) const {
//#ifndef NDEBUG
//	if (!pinned())
//		throw_FigException("pin_up_vars() hasn't been called yet");
// #endif
	// Copy the useful part of 'state'...
    /*
	for (size_t i = 0ul ; i < NVARS_ ; i++) {
		assert(nullptr != state[varsNames_[i]]);
		varsValues_[i] = state[varsNames_[i]]->val();  // NOTE see other note
	}
	// ...evaluate...
	int numUpdates(NUPDATES_);
	STATE_INTERNAL_TYPE* updates = expr_.Eval(numUpdates);
	assert(NUPDATES_ == static_cast<size_t>(numUpdates) || expression().empty());
	// ...and reflect in state
	for (size_t i = 0ul ; i < NUPDATES_ ; i++) {
		assert(nullptr != state[updatesNames_[i]]);
		state[updatesNames_[i]]->assign(updates[i]);
	}
    */
}

void
Postcondition::operator()(StateInstance& state) const
{
//#ifndef NDEBUG
//	if (!pinned())
//		throw_FigException("pin_up_vars() hasn't been called yet");
//#endif
//	// Copy the useful part of 'state'...
//	for (size_t i = 0ul ; i < NVARS_ ; i++) {
//		assert(state.size() > varsPos_[i]);
//		varsValues_[i] = state[varsPos_[i]];  // ugly motherfucker
//		/// @todo
//		/// NOTE As an alternative we could use memcpy() to copy the values,
//		///      but that means bringing a whole chunk of memory of which
//		///      only a few variables will be used. To lighten that we could
//		///      impose an upper bound on the number of variables per guard,
//		///      but then the language's flexibility will be compromised.
//	}
//	// ...evaluate...
//	int numUpdates(NUPDATES_);
//	STATE_INTERNAL_TYPE* updates = expr_.Eval(numUpdates);
//	assert(NUPDATES_ == static_cast<size_t>(numUpdates) || expression().empty());
//	// ...and reflect in state
//	for (size_t i = 0ul ; i < NUPDATES_ ; i++) {
//		assert(state.size() > updatesPos_[i]);
//		state[updatesPos_[i]] = updates[i];
//	}
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
