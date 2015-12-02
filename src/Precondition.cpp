//==============================================================================
//
//  Precondition.cpp
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
#include <iostream>
#include <exception>
// FIG
#include <Precondition.h>
#include <FigException.h>

using std::cerr;
using std::endl;


namespace fig
{

void
Precondition::fake_evaluation()
{
	STATE_INTERNAL_TYPE dummy(static_cast<STATE_INTERNAL_TYPE>(1.1));
	try {
		for (const auto& var: varsMap_)
			expr_.DefineVar(var.first, &dummy);
		expr_.Eval();
	} catch (mu::Parser::exception_type &e) {
		cerr << "Failed parsing expression" << endl;
		cerr << "    message:  " << e.GetMsg()   << endl;
		cerr << "    formula:  " << e.GetExpr()  << endl;
		cerr << "    token:    " << e.GetToken() << endl;
		cerr << "    position: " << e.GetPos()   << endl;
		cerr << "    errc:     " << e.GetCode()  << endl;
		throw FigException("bad expression for precondition");
	}
}


void
Precondition::pin_up_vars(const PositionsMap &globalVars)
{
	MathExpression::pin_up_vars(globalVars);
#ifndef NDEBUG
	fake_evaluation();  // Reveal parsing errors in this early stage
#endif
}


bool
Precondition::operator()(const StateInstance& state)
{
	// Bind state's variables to our expression...
	for (const auto& pair: varsMap_)
		expr_.DefineVar(pair.first,  const_cast<STATE_INTERNAL_TYPE*>(
						&state[pair.second]));
	// ...and evaluate
	return static_cast<bool>(expr_.Eval());
}

}
