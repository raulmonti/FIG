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


// C
#include <cassert>
// C++
#include <algorithm>
#include <iterator>
#include <sstream>
// FIG
#include <ThresholdsBuilder.h>
#include <FigException.h>
#include <FigLog.h>

// ADL
using std::begin;
using std::end;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
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

	    // Hybrid thresholds selection: Adaptive + Fixed
		// See ThresholdsBuilderHybrid class
		"hyb",

		// Expected Success (Budde, D'Argenio, Hartmanns '17)
		// See ThresholdsBuilderES class
		"es"
	}};
	return techniques;
}


ThresholdsVec
ThresholdsBuilder::invert_thresholds_map(const ThresholdsVec& t2i) const
{
	/// @todo TODO refactor for the new type, ThresholdsVec

	assert(t2i.size() > 0ul);
	assert(t2i.back() > 1ul);
	const size_t SIZE(t2i.back());
	ImportanceVec i2t(SIZE);
	unsigned currThr(0ul);
	for (size_t i = 0ul ; i < SIZE ; i++) {
		while (currThr < t2i.size()-1 && i >= t2i[currThr+1])
			currThr++;
		i2t[i] = static_cast<ImportanceValue>(currThr);
	}
	return i2t;
	/* * * * *
	 * Assertions to check in the map returned:
	 *   assert(i2t[impFun.min_value()] == 0);
	 *   assert(i2t[impFun.initial_value()] == 0);
	 *   assert(i2t[impFun.max_value()] == t2i.size()-2);
	 */
}


void
ThresholdsBuilder::show_thresholds(const ImportanceVec& t2i)
{
	figTechLog << "ImportanceValue of the chosen thresholds:";
	for (size_t i = 1ul ; i < t2i.size()-1 ; i++)
		figTechLog << " " << t2i[i];
	figTechLog << "\n";
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
