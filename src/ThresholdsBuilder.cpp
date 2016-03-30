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

ThresholdsBuilder::ThresholdsBuilder(const std::string& thename) :
	name(thename)
{
	if (std::find(begin(techniques()), end(techniques()), name)
			== end(techniques())) {
		std::stringstream errMsg;
		errMsg << "invalid threshold building technique \"" << name << "\". ";
		errMsg << "Available techniques are";
		for (const auto& technique: techniques())
			errMsg << " \"" << technique << "\"";
		errMsg << "\n";
		throw_FigException(errMsg.str());
	}
}


const std::array<std::string, ThresholdsBuilder::NUM_TECHNIQUES>&
ThresholdsBuilder::techniques() noexcept
{
	static const std::array< std::string, NUM_TECHNIQUES > techniques =
	{{
		// Fixed thresholds selection ("1 out of every N importance values")
		// See ThresholdsBuilderFixed class
		"fix",

		// Adaptive Multilevel Splitting (Cerou and Guyader '07)
		// See ThresholdsBuilderAMS class
		"ams",

		// Sequential Monte Carlo (Cerou, Del Moral, Furon and Guyader '11)
		// See ThresholdsBuilderSMC class
		"smc",

		// Hybrid thresholds selection: Sequential Monte Carlo + Fixed
		// See ThresholdsBuilderHybrid class
		"hyb"
	}};
	return techniques;
}

} // namespace fig
