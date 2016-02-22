//==============================================================================
//
//  PropertyTransient.cpp
//
//  Copyleft 2016-
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


#include <PropertyTransient.h>


namespace fig
{

const std::string
PropertyTransient::expression1() const noexcept
{
	return expr1_.expression();
}


const std::string
PropertyTransient::expression2() const noexcept
{
	return expr2_.expression();
}


void
PropertyTransient::pin_up_vars(const PositionsMap &globalVars)
{
	expr1_.pin_up_vars(globalVars);
	expr2_.pin_up_vars(globalVars);
}


void
PropertyTransient::pin_up_vars(const fig::State<STATE_INTERNAL_TYPE>& globalState)
{
	expr1_.pin_up_vars(globalState);
	expr2_.pin_up_vars(globalState);
}

} // namespace fig
