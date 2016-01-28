//==============================================================================
//
//  ThresholdsBuilderAMS.cpp
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
#include <cmath>
// C++
#include <memory>
// FIG
#include <ThresholdsBuilderAMS.h>
#include <ModelSuite.h>
#include <SimulationEngineNosplit.h>


namespace fig
{

void
ThresholdsBuilderAMS::tune(const size_t&   numStates,
						   const unsigned& numTrans,
						   const unsigned& maxImportance,
						   const unsigned& splitsPerThr)
{
	/// @todo FIXME code imported from bluemoon -- Change to something more solid

	// Heuristic for 'n_':
	//   the more importance values, the more independent runs we need
	//   for some of them to be successfull.
	//   Number of states and transitions in the model play a role too.
	const unsigned explFactor = 50;
	const unsigned statesExtra = std::min<unsigned>(numStates/explFactor, 100);
	const unsigned transExtra = std::min<unsigned>(2*numTrans/explFactor, 150);
	n_  = std::ceil(std::log(maxImportance)) * explFactor + statesExtra + transExtra;

	// Heuristic for 'k_':
	//   splitsPerThr * levelUpProb == 1  ("balanced growth")
	//   where levelUpProb == k_/n_
	k_ = std::round(n_ / static_cast<float>(splitsPerThr));

	assert(k_ < n_);
}


void
ThresholdsBuilderAMS::build_thresholds_concrete(
	const ImportanceFunctionConcrete &impFun,
	std::vector< ImportanceValue >& impVec) const
{
	auto net = ModelSuite::get_instance().modules_network();

	auto engine_ptr = std::make_shared< SimulationEngineNosplit >(net);
	std::shared_ptr< const ImportanceFunction > impFun_ptr(impFun);

	/// @todo TODO translate code from bluemoon's "AMS::get_thresholds" in PR_AMS.cc
}

} // namespace fig
