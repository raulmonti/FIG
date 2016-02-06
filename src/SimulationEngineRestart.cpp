//==============================================================================
//
//  SimulationEngineRestart.cpp
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


#include <SimulationEngineRestart.h>
#include <PropertyTransient.h>
#include <TraialPool.h>
#include <FigException.h>


namespace fig
{

// Available engine names in SimulationEngine::names
SimulationEngineRestart::SimulationEngineRestart(
	std::shared_ptr<const ModuleNetwork> network,
	const unsigned& splitsPerThreshold,
	const unsigned& dieOutDepth) :
		SimulationEngine("restart", network),
		splitsPerThreshold_(splitsPerThreshold),
		dieOutDepth_(dieOutDepth)
{ /* Not much to do around here */ }


void
SimulationEngineRestart::bind(std::shared_ptr< const ImportanceFunction > ifun_ptr)
{
	const std::string impStrategy(ifun_ptr->strategy());
	if (impStrategy == "")
		throw_FigException("ImportanceFunction doesn't seem to have "
						   "internal importance information");
	else if (impStrategy == "flat")
		throw_FigException("RESTART simulation engine requires an importance "
						   "building strategy other than \"flat\"");
	SimulationEngine::bind(ifun_ptr);
}


void
SimulationEngineRestart::set_splits_per_threshold(unsigned splitsPerThreshold)
{
	if (splitsPerThreshold < 2u)
		throw_FigException("at least 1 Traial must be created, besides the "
						   "original one, when crossing a threshold upwards");
	splitsPerThreshold_ = splitsPerThreshold;
}


void
SimulationEngineRestart::set_die_out_depth(unsigned dieOutDepth)
{
	dieOutDepth_ = dieOutDepth;
}


double
SimulationEngineRestart::transient_simulations(const PropertyTransient& property,
											   const size_t& numRuns) const
{
	assert(0u < numRuns);
	std::vector<long> numHits(impFun_->num_thresholds(), 0);
	std::stack< Reference< Traial > > stack;
	auto tpool = TraialPool::get_instance();
	// For the sake of efficiency, distinguish when operating with a concrete ifun
	if (impFun_->concrete()) {
		for (size_t i = 0u ; i < numRuns ; i++) {
			tpool.get_traials(stack, 1u);
			stack.top().initialize(network_, impFun_);
			transient_simulation_concrete(property, stack, numHits);
			assert(stack.empty());
		}
	} else {
		for (size_t i = 0u ; i < numRuns ; i++) {
			tpool.get_traials(stack, 1u);
			stack.top().initialize(network_, impFun_);
			transient_simulation_concrete(property, stack, numHits);
			assert(stack.empty());
		}
	}
	/// @todo TODO copy result computation from rarevent.cc:941
	return ???;
}


void
SimulationEngineRestart::transient_simulation_generic(
	const PropertyTransient& property,
	std::stack< Reference <Traial> >& stack,
	std::vector< long >& numHits)
{
	/// @todo TODO copy from rarevent.cc:580
	throw_FigException("TODO: copy from rarevent.cc:893");
}


void
SimulationEngineRestart::transient_simulation_concrete(
	const PropertyTransient& property,
	std::stack< Reference <Traial> >& stack,
	std::vector< long >& numHits)
{
	/// @todo TODO copy from rarevent.cc:580
	throw_FigException("TODO: copy from rarevent.cc:893");
}

} // namespace fig
