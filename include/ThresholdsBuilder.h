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

class ImportanceFunction;
class ImportanceFunctionConcrete;

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

protected:

	/// Thresholds importance values
	std::vector< ImportanceValue > thresholds_;

public:

	/// Ctor
	ThresholdsBuilder(const std::string& thename);

	/// Whether the class builds the thresholds <i>adaptively</i>,
	/// viz. taking into consideration the user model's semantics
	virtual bool adaptive() const noexcept = 0;

	/**
	 * @brief Build thresholds based on given importance function
	 *
	 *        Return an importance-to-threshold_level map: the i-th position
	 *        of the resulting vector will hold the threshold level
	 *        corresponding to the i-th ImportanceValue from impFun.
	 *        Here the j-th threshold level is composed of all the states
	 *        to which impFun assigns an ImportanceValue between (including)
	 *        threshold j and (excluding) threshold j+1.
	 *
	 * @param splitsPerThreshold 1 + Number of simulation-run-replicas upon a
	 *                           "threshold level up" event
	 * @param impFun ImportanceFunction with internal
	 *               \ref ImportanceFunction::has_importance_info()
	 *               "importance information" to use for the task
	 *
	 * @return Vector with threshold levels as explained in the details.
	 *
	 * @note The effective number of thresholds built
	 *       equals the value in the last position of the returned vector
	 *
	 * @throw FigException if thresholds building failed
	 */
	virtual std::vector< ImportanceValue >
	build_thresholds(const unsigned& splitsPerThreshold,
					 const ImportanceFunction& impFun) = 0;
};

} // namespace fig

#endif // THRESHOLDSBUILDER_H
