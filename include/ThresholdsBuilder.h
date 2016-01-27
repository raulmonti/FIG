//==============================================================================
//
//  ThresholdsBuilder.h
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

#ifndef THRESHOLDSBUILDER_H
#define THRESHOLDSBUILDER_H

// C++
#include <vector>
// FIG
#include <core_typedefs.h>
#include <State.h>


namespace fig
{

/**
 * @brief Asbtract base builder of importance thresholds.
 *
 *        Importance thresholds are required for the application of
 *        importance splitting techniques during Monte Carlo simulations.
 *        For instance in the RESTART method everytime a simulation crosses a
 *        threshold "upwards", i.e. gaining on importance, the state is saved
 *        and the simulation run is replicated a predefined number of times.
 *        Oppositely, when a threshold is crossed "downwards", i.e. loosing on
 *        importance, the simulation run is discarded.
 */
class ThresholdsBuilder
{
public:

	/**
	 * @brief Tune up internals according to the size of the state space
	 *        and potentially any other data specific to the derived class.
	 * @param state    Symbolic state of the Module
	 * @param initData Tuning data specific to the particular derived class
	 * @throw FigException if 'initData' was incompatible with the
	 *                     ThresholdsBuilder derived class
	 */
	template< typename T_ >
	virtual void
	tune(const State<STATE_INTERNAL_TYPE>& state,
		 const T_* initData = nullptr) = 0;

	/**
	 * @brief Build thresholds from given concrete vector of importance values
	 * @param edges  AdjacencyList for the concrete-states transition graph
	 * @param impVec Vector with the computed importance of every concrete state
	 * @throw FigException if unsupported by the ThresholdsBuilder derived class
	 */
	virtual void
	build_thresholds_concrete(const AdjacencyList& edges,
							  std::vector< ImportanceValue >& impVec) const = 0;
};

} // namespace fig

#endif // THRESHOLDSBUILDER_H
