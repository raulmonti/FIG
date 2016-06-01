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
#include <sstream>
// FIG
#include <Precondition.h>
#include <FigException.h>


namespace fig
{

/// @todo TODO erase debug code
// Precondition::Precondition(const Precondition& that) : MathExpression(that)
// { std::cerr << "PRE: copy ctor\n"; }
// Precondition::Precondition(Precondition&& that) :
// 	MathExpression(std::forward<MathExpression>(that))
// { std::cerr << "PRE: move ctor\n"; }
///////////////////////////////

void
Precondition::test_evaluation() const
{
	assert(pinned());
	try {
		const STATE_INTERNAL_TYPE DUMMY(static_cast<STATE_INTERNAL_TYPE>(1.1));
		for (STATE_INTERNAL_TYPE& val: varsValues_)
			val = DUMMY;
		expr_.Eval();
	} catch (mu::Parser::exception_type& e) {
		std::cerr << "Failed parsing expression" << std::endl;
		std::cerr << "    message:  " << e.GetMsg()   << std::endl;
		std::cerr << "    formula:  " << e.GetExpr()  << std::endl;
		std::cerr << "    token:    " << e.GetToken() << std::endl;
		std::cerr << "    position: " << e.GetPos()   << std::endl;
		std::cerr << "    errc:     " << e.GetCode()  << std::endl;
		throw_FigException("bad expression for precondition, "
						   "did you remember to map all the variables?");
	}
}


#ifndef  NRANGECHK
void
Precondition::pin_up_vars(const PositionsMap& globalVars)
{
	MathExpression::pin_up_vars(globalVars);
# ifndef NDEBUG
	test_evaluation();  // Reveal parsing errors in this early stage
# endif
}
#else
void
Precondition::pin_up_vars(PositionsMap& globalVars)
{
	MathExpression::pin_up_vars(globalVars);
# ifndef NDEBUG
	test_evaluation();  // Reveal parsing errors in this early stage
# endif
}
#endif


void
Precondition::pin_up_vars(const fig::State<STATE_INTERNAL_TYPE>& globalState)
{
	MathExpression::pin_up_vars(globalState);
#ifndef NDEBUG
	test_evaluation();  // Reveal parsing errors in this early stage
#endif
}


bool
Precondition::operator()(const StateInstance& state) const
{
#ifndef NDEBUG
	if (!pinned())
		throw_FigException("pin_up_vars() hasn't been called yet");
#endif
	// Copy the useful part of 'state'...
	for (size_t i = 0ul ; i < NVARS_ ; i++)
		varsValues_[i] = state[varsPos_[i]];  // ugly motherfucker
	/// @todo NOTE As an alternative we could use memcpy() to copy the values,
	///            but that means bringing a whole chunk of memory of which
	///            only a few variables will be used. To lighten that we could
	///            impose an upper bound on the number of variables per guard,
	///            but then the language's flexibility will be compromised.
	// ...and evaluate
	return static_cast<bool>(expr_.Eval());
}


bool
Precondition::operator()(const State<STATE_INTERNAL_TYPE>& state) const
{
#ifndef NDEBUG
	if (!pinned())
		throw_FigException("pin_up_vars() hasn't been called yet");
#endif
	// Copy the useful part of 'state'...
	for (size_t i = 0ul ; i < NVARS_ ; i++)
		varsValues_[i] = state[varsPos_[i]]->val();  // NOTE see other note
	// ...and evaluate
	return static_cast<bool>(expr_.Eval());
}

}
