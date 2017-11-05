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


ThresholdsBuilderAdaptive::ThresholdsBuilderAdaptive(
        const std::string& name,
        const unsigned& n) :
    ThresholdsBuilder(name),
    n_(n),
    thresholds_(),
    halted_(false)
{ /* Not much to do around here */ }


ThresholdsBuilderAdaptive::TraialsVec
ThresholdsBuilderAdaptive::get_traials(const unsigned& numTraials,
                                       const fig::ImportanceFunction& impFun)
{
	TraialsVec traials;
	fig::TraialPool::get_instance().get_traials(traials, numTraials);
	assert(traials.size() == numTraials);
	const ModuleNetwork& net = *ModelSuite::get_instance().modules_network();
	for (fig::Traial& t: traials)
		t.initialize(net, impFun);
	return traials;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
