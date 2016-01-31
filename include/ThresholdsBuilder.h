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
#include <array>
#include <vector>
#include <string>
// FIG
#include <core_typedefs.h>


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
 *        Oppositely, when a simulation crosses a threshold "downwards"
 *        loosing on importance, it is discarded.
 */
class ThresholdsBuilder
{
public:

	/// Names of the (derived) thresholds builders offered to the user,
	/// as he should requested them through the CLI/GUI.
	/// Defined in ThresholdsBuilder.cpp
	static const std::array< std::string, 1 > names;

	/// Thresholds building technique implemented by this instance
	/// Check ThresholdsBuilder::names for available options.
	const std::string name;

public:

	/// Ctor
	ThresholdsBuilder(const std::string thename);

	/**
	 * @brief Build thresholds from given concrete importance function.
	 *
	 *        The thresholds are built in 'impVec', which holds the importance
	 *        of the concrete states and is actually an internal vector from
	 *        the passed concrete importance function 'impFun'.
	 *        Original importance information is destroyed: after the call
	 *        'impVec[i]' will hold the level of the i-th concrete state,
	 *        where the j-th level is composed of all the states between the
	 *        j-th and the (j+1)-th thresholds.
	 *        The result can be regarded as a coarser version of the original
	 *        importance values which were fed into the routine.
	 *
	 * @param splitsPerThreshold 1 + Number of simulation-run-replicas upon a
	 *                           "threshold level up" event
	 * @todo TODO Update for new signature
	 * @param impVec Vector from impFun where the thresholds will be stored <b>(modified)</b>
	 *
	 * @throw FigException if unsupported by the ThresholdsBuilder derived class
	 */
	virtual void
	build_thresholds_concrete(const unsigned& splitsPerThreshold,
							  ImportanceValue& maxImportance,
							  ImportanceValue& minRareImportance,
							  std::vector< ImportanceValue >& impVec) const = 0;
};

} // namespace fig

#endif // THRESHOLDSBUILDER_H
