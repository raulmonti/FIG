//==============================================================================
//
//  ThresholdsBuilderAdaptive.cpp
//
//  Copyleft 2017-
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


// FIG
#include "ThresholdsBuilderAdaptive.h"
#include <ModelSuite.h>
#include <TraialPool.h>


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

const unsigned ThresholdsBuilderAdaptive::MIN_N = 1ul<<8ul;   //  256
const unsigned ThresholdsBuilderAdaptive::MAX_N = 1ul<<13ul;  // 8192
bool ThresholdsBuilderAdaptive::highVerbosity = ModelSuite::get_verbosity();


ThresholdsBuilderAdaptive::ThresholdsBuilderAdaptive(const unsigned& n) :
    n_(n),
    thresholds_(),
    halted_(false)
{ /* Not much to do around here */ }


void
ThresholdsBuilderAdaptive::tune(const size_t& numTrans,
                                const ImportanceValue& maxImportance,
                                const unsigned& globalEffort)
{
	// Heuristic for 'n_':
	//   the more importance values, the more independent runs we need
	//   for some of them to be successfull.
	//   Something similar happens with the number of edges
	//   (aka symbolic transitions).

	// Importance typically grows exponentially with the rarity paremeter.
	// Since we want to run more simulations for rarer events,
	// we use (a scaled version of) the logarithm of the max importance.
	const unsigned impFactor = (1u<<6u) * std::ceil(std::log(maxImportance));
	// The number of transitions is not clearly related to the rarity parameter,
	// but more transitions mean more divergence in the paths towards the rare
	// event. Thus we increase the number of simulation runs linearly
	// with the number of model transitions.
	const unsigned transFactor = 5u*numTrans;

	const double balance = 0.6;  // must be within (0.0, 1.0)
	// more relevance to importance  => balance++
	// more relevance to transitions => balance--
	n_  = std::min(impFactor  , static_cast<unsigned>((    balance)*MAX_N))
	    + std::min(transFactor, static_cast<unsigned>((1.0-balance)*MAX_N));
	n_ *= std::log10(globalEffort);
	assert(0ul < n_);
}


ThresholdsBuilderAdaptive::TraialsVec
ThresholdsBuilderAdaptive::get_traials(const unsigned& numTraials,
                                       const fig::ImportanceFunction& impFun,
                                       bool initialise) const
{
	static auto tpool(fig::TraialPool::get_instance());
	TraialsVec traials;
	tpool.get_traials(traials, numTraials);
	assert(traials.size() == numTraials);
	if (initialise)
		for (fig::Traial& t: traials)
			t.initialise(*ModelSuite::get_instance().modules_network(), impFun);
	return traials;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
