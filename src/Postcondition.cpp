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


namespace fig
{

Postcondition::Postcondition(const Postcondition& that) :
	MathExpression(that),
	numUpdates_(that.numUpdates_),
	updatesData_(that.updatesData_)
{
	switch (updatesData_) {
	case NAMES:
		new (&updatesNames_) std::vector< std::string >;
		updatesNames_.insert(begin(updatesNames_),
							 begin(that.updatesNames_),
							 end(that.updatesNames_));
		break;
	case POSITIONS:
		new (&updatesPositions_) std::vector< size_t >;
		updatesPositions_.insert(begin(updatesPositions_),
								 begin(that.updatesPositions_),
								 end(that.updatesPositions_));
		break;
	}
}


Postcondition::Postcondition(Postcondition&& that) :
	MathExpression(std::forward<MathExpression&&>(that)),
	numUpdates_(std::move(that.numUpdates_)),
	updatesData_(std::move(that.updatesData_))
{
	switch (updatesData_) {
	case NAMES:
		new (&updatesNames_) std::vector< std::string >;
		swap(updatesNames_, that.updatesNames_);
		break;
	case POSITIONS:
		new (&updatesPositions_) std::vector< size_t >;
		swap(updatesPositions_, that.updatesPositions_);
		break;
	}
}


Postcondition&
Postcondition::operator=(Postcondition that)
{
	MathExpression::operator=(std::move(that));
	swap(numUpdates_, that.numUpdates_);
	swap(updatesData_, that.updatesData_);
	switch (updatesData_) {
	case NAMES:
		swap(updatesNames_, that.updatesNames_);
		break;
	case POSITIONS:
		swap(updatesPositions_, that.updatesPositions_);
		break;
	}
	return *this;
}


Postcondition::~Postcondition()
{
	switch (updatesData_) {
	case NAMES:
		updatesNames_.~vector< std::string >();
		break;
	case POSITIONS:
		updatesPositions_.~vector< size_t >();
		break;
	}
}


void
Postcondition::fake_evaluation() const
{
	STATE_INTERNAL_TYPE dummy(static_cast<STATE_INTERNAL_TYPE>(1.1));
	try {
		for (const auto& var: varsMap_)
			expr_.DefineVar(var.first, &dummy);
		int numUpdates(numUpdates_);
		STATE_INTERNAL_TYPE* ptr = expr_.Eval(numUpdates);
		assert(numUpdates_ == numUpdates || expression() == "");
		assert(expr_.GetNumResults() == numUpdates);
		// MuParser library handles memory, leave ptr alone
		if (ptr) ptr = nullptr;  // dodge compiler warning
	} catch (mu::Parser::exception_type &e) {
		std::cerr << "Failed parsing expression" << std::endl;
		std::cerr << "    message:  " << e.GetMsg()   << std::endl;
		std::cerr << "    formula:  " << e.GetExpr()  << std::endl;
		std::cerr << "    token:    " << e.GetToken() << std::endl;
		std::cerr << "    position: " << e.GetPos()   << std::endl;
		std::cerr << "    errc:     " << e.GetCode()  << std::endl;
		throw FigException("bad expression for postcondition, "
						   "did you remember to map all the variables?");
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
		positions[i++] = globalVars.at(name);
#else
		positions[i++] = globalVars[name];
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
Postcondition::pin_up_vars(const fig::State<STATE_INTERNAL_TYPE>& globalState)
{
	// Map general expression variables
	MathExpression::pin_up_vars(globalState);
	// Map update variables
	assert(NAMES == updatesData_);
	std::vector< size_t > positions(updatesNames_.size());
	size_t i(0u);
	for (const auto& name: updatesNames_)
		positions[i++] = globalState.position_of_var(name);
	updatesNames_.~vector< std::string >();
	new (&updatesPositions_) std::vector< size_t >;
	swap(positions, updatesPositions_);
	updatesData_ = POSITIONS;
#ifndef NDEBUG
	fake_evaluation();  // Reveal parsing errors in this early stage
#endif
}


void
Postcondition::operator()(StateInstance& state) const
{
#ifndef NDEBUG
	if (!pinned())
		throw FigException("pin_up_vars() hasn't been called yet");
#endif
	// Bind state variables to our expression...
	for (const auto& pair: varsMap_)
		expr_.DefineVar(pair.first,  const_cast<STATE_INTERNAL_TYPE*>(
						&state[pair.second]));
	// ...evaluate...
	int numUpdates(numUpdates_);
	STATE_INTERNAL_TYPE* updates = expr_.Eval(numUpdates);
	assert(numUpdates_ == numUpdates || expression() == "");
	// ...and reflect in state
	for (int i = 0 ; i < numUpdates_ ; i++)
		state[updatesPositions_[i]] = updates[i];
}

} // namespace fig
