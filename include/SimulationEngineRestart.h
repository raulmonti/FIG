//==============================================================================
//
//  SimulationEngineRestart.h
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

#ifndef SIMULATIONENGINERESTART_H
#define SIMULATIONENGINERESTART_H

#include <SimulationEngine.h>


namespace fig
{

class SimulationEngineRestart : public SimulationEngine
{

	/// 1 + Number of replicas made of a Traial when it crosses
	/// an importance threshold upwards (i.e. gaining on importance)
	/// @see ThresholdsBuilder
	unsigned splitsPerThreshold_;

	/// Number of importance thresholds a simulation run must cross downwards
	/// (i.e. loosing on importance) to be discarded
	unsigned dieOutDepth_;

public:  // Ctor

	/// Data ctor
	SimulationEngineRestart(std::shared_ptr<const ModuleNetwork> network,
							const unsigned& splitsPerThreshold = 2u,
							const unsigned& dieOutDepth = 0u);

public:  // Engine setup

	/// @see splitsPerThreshold_
	/// @throw FigException if the value is invalid
	void set_splits_per_threshold(unsigned splitsPerThreshold);

	/// @see dieOutDepth_
	/// @throw FigException if the value is invalid
	void set_die_out_depth(unsigned dieOutDepth);

public:  // Accessors

	/// @copydoc splitsPerThreshold_
	inline const unsigned& get_splits_per_threshold() const noexcept
		{ return splitsPerThreshold_; }

	/// @copydoc dieOutDepth_
	inline const unsigned& get_die_out_depth() const noexcept
		{ return dieOutDepth_; }

};

} // namespace fig

#endif // SIMULATIONENGINERESTART_H
