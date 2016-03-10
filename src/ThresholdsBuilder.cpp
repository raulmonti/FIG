//==============================================================================
//
//  ThresholdsBuilder.cpp
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


// C++
#include <algorithm>
#include <iterator>
// FIG
#include <ThresholdsBuilder.h>
#include <FigException.h>

// ADL
using std::begin;
using std::end;


namespace fig
{

// Static variables initialization

const std::array< std::string, 2 > ThresholdsBuilder::names =
{{
	 // Adaptive Multilevel Splitting (Cerou and Guyader '07)
	 // See ThresholdsBuilderAMS class
	 "ams",

	 // Sequential Monte Carlo (Cerou, Del Moral, Furon and Guyader '11)
	 // See ThresholdsBuilderSMC class
	 "smc"
}};


// ThresholdsBuilder class member functions

ThresholdsBuilder::ThresholdsBuilder(const std::string& thename) :
	name(thename)
{
	if (std::find(begin(names), end(names), name) == end(names)) {
		std::stringstream errMsg;
		errMsg << "invalid threshold building technique \"" << name << "\". ";
		errMsg << "Available techniques are";
		for (const auto& name: names)
			errMsg << " \"" << name << "\"";
		errMsg << "\n";
		throw_FigException(errMsg.str());
	}
}

} // namespace fig
