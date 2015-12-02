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
#include <utility>  // std::swap()
#include <iostream>
// FIG
#include <Postcondition.h>

using std::cerr;
using std::endl;
// ADL
using std::swap;


namespace fig
{

void
Postcondition::fake_evaluation()
{
	STATE_INTERNAL_TYPE dummy(static_cast<STATE_INTERNAL_TYPE>(1.1));
	try {
		for (const auto& var: varsMap_)
			expr_.DefineVar(var.first, &dummy);
		STATE_INTERNAL_TYPE* ptr = expr_.Eval(numUpdates_);
		assert(expr_.GetNumResults() == numUpdates_);
		// MuParser library handles memory, leave ptr alone
		if (ptr) ptr = nullptr;  // dodge compiler warning
	} catch (mu::Parser::exception_type &e) {
		cerr << "Failed parsing expression" << endl;
		cerr << "    message:  " << e.GetMsg()   << endl;
		cerr << "    formula:  " << e.GetExpr()  << endl;
		cerr << "    token:    " << e.GetToken() << endl;
		cerr << "    position: " << e.GetPos()   << endl;
		cerr << "    errc:     " << e.GetCode()  << endl;
		throw FigException("bad expression for postcondition");
	}
}


void
Postcondition::pin_up_vars(const PositionsMap &globalVars)
{
	// Map general expression variables
	MathExpression::pin_up_vars(globalVars);
	// Map update variables
	assert(NAMES == updatesData_);
	std::vector< size_t > positions(updatesNames_.size());
	size_t i(0u);
	for (const auto& name: updatesNames_)
#ifndef NRANGECHK
		positions_[i++] = globalVars.at(name);
#else
		positions_[i++] = globalVars[name];
#endif
	updatesNames_.~vector< std::string >();
	new (&updatesPositions_) std::vector< size_t >;
	swap(positions, updatesPositions_);
	updatesData_ = POSITIONS;
#ifndef NDEBUG
	fake_evaluation();  // Reveal parsing errors in this early stage
#endif
}


void
Postcondition::operator()(StateInstance& state)
{
	// Bind state variables to our expression...
	for (const auto& pair: varsMap_)
		expr_.DefineVar(pair.first,  const_cast<STATE_INTERNAL_TYPE*>(
						&state[pair.second]));
	// ...evaluate...
	STATE_INTERNAL_TYPE* updates = expr_.Eval(numUpdates_);
	// ...and reflect in state
	for (int i = 0 ; i < numUpdates_ ; i++)
		state[updatesPositions_[i]] = updates[i];
}

} // namespace fig
