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
#include <memory>
// FIG
#include <core_typedefs.h>
#include <FigException.h>


namespace fig
{

class Property;
class ImportanceFunction;

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

	/// Default max number of thresholds
	static constexpr size_t MAX_NUM_THRESHOLDS = 200ul;

	/// Default max effort per threshold
	static constexpr size_t MAX_EFFORT = 333ul;

	/// Long story short: number of concrete derived classes.
	/// More in detail this is the size of the array returned by techniques(),
	/// i.e. how many ThresholdsBuilder implementations are offered to the end user.
	static constexpr size_t NUM_TECHNIQUES = 5;

	/// Thresholds building technique implemented by this instance
	/// Check ThresholdsBuilder::names for available options.
	const std::string name;

public:

	/// Ctor
	ThresholdsBuilder(const std::string& thename);

	virtual ~ThresholdsBuilder() {}

	/// Whether the class builds the thresholds <i>adaptively</i>,
	/// viz. taking into consideration the user model's semantics
	virtual bool adaptive() const noexcept = 0;

	/// Whether a global effort value is needed to select the thresholds
	/// @see setup()
	virtual bool uses_global_effort() const noexcept = 0;

	/// Threshold building techniques offered to the user,
	/// as he should requested them through the CLI/GUI.
	/// @note Implements the <a href="https://goo.gl/yhTgLq"><i>Construct On
	///       First Use</i> idiom</a> for static data members,
	///       to avoid the <a href="https://goo.gl/chH5Kg"><i>static
	///       initialization order fiasco</i>>.
	static const std::array< std::string, NUM_TECHNIQUES >& techniques() noexcept;

	/**
	 * @brief Depending on the thresholds builder sub-class,
	 *        provide extra information required to select the thresholds.
	 *
	 * @param property User property query being estimated
	 * @param extra    Additional data to build thresholds,
	 *                 e.g. global splitting, or ad hoc thresholds/effort list
	 *
	 * @note @a gobalEffort means different things depending on
	 *       the type of importance splitting used:
	 *       <ul>
	 *       <li>For RESTART it means the same splitting value is used
	 *           in all thresholds, i.e. @a globalEffort-1 replicas
	 *           will be created in a level-up;</li>
	 *       <li>For Fixed Effort it will run the same number of simulations
	 *           (namely @a globalEffort) in all ("threshold-") levels.</li>
	 *       </ul>
	 *
	 * @bug The presence of this function is a consequence of a
	 *      <b>poorly designed class</b>
	 */
	virtual void
	setup(std::shared_ptr<const Property> property = nullptr,
	      const void* extra = nullptr) = 0;

	/**
	 * @brief Choose thresholds based on given importance function
	 *
	 *        Choose the importance values to use as thresholds for splitting.
	 *        A map of pairs is built where the pair at the i-th position holds:
	 * 	      <ol>
	 *        <li>the minimum ImportanceValue of the i-th level;</li>
	 *        <li>the splitting/effort to perform there.</li>
	 * 		  </ol>
	 *        A <i>threshold-level</i> or simply <i>level</i>
	 *        is a range of importance values.
	 *        The i-th level comprises all importance values between
	 *        threshold i (including it) and threshold i+1 (excluding it).
	 *
	 * @param impFun ImportanceFunction with internal
	 *               \ref ImportanceFunction::has_importance_info()
	 *               "importance information", i.e. the structure must not be empty
	 *
	 * @return Thresholds levels map as explained in the details.
	 *
	 * @note The size of the resulting vector  <br>
	 *       == 1 + number of threshold levels <br>
	 *       == 2 + number of thresholds built
	 * @note The first ImportanceValue in the map == initial state importance
	 * @note The last  ImportanceValue in the map == 1 + impFun.max_importance()
	 *
	 * @throw FigException if thresholds building failed
	 */
	virtual ThresholdsVec
	build_thresholds(std::shared_ptr<const ImportanceFunction> impFun) = 0;

	/**
	 * @brief Turn map around, building an importance-to-threshold map
	 *
	 *        From the threshold-to-importance map passed as argument,
	 *        build a reversed importance-to-threshold map: the j-th position
	 *        of the vector returned will hold the ("threshold-") level and
	 *        corresponding splitting/effort of the j-th ImportanceValue.
	 *
	 * @param t2i threshold-to-importance map as returned by build_thresholds()
	 *
	 * @return Vector with threshold levels as explained in the details.
	 *
	 * @throw bad_alloc if there wasn't enough memory to allocate the vector
	 * @throw FigException if the translation failed
	 *
	 * @warning The size of the map returned is the maximum importance in @a t2i
	 */
	ThresholdsVec
	invert_thresholds_map(const ThresholdsVec &t2i) const;

public:  // Utils

	/// Print thresholds info in FIG's tech log
	/// @param t2i threshold-to-importance map as returned by build_thresholds()
	static void show_thresholds(const ThresholdsVec& t2i);

	/// Print thresholds info in FIG's tech log
	/// @param t2i threshold-to-importance map as returned by build_thresholds()
	///            but without the per-level-effort
	static void show_thresholds(const ImportanceVec& t2i);
};

} // namespace fig

#endif // THRESHOLDSBUILDER_H
