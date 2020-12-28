//==============================================================================
//
//  Clock.h
//
//  Copyleft 2015-
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


#ifndef CLOCK_H
#define CLOCK_H

// C++
#include <string>
#include <memory>  // std::shared_ptr<>
#include <unordered_map>
// C
#include <cassert>
// FIG
#include <core_typedefs.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


namespace fig
{

/**
 * @brief Internal stochastic time passage mechanism for IOSA modules
 *
 *        Clocks have a name and a Distribution which can be sampled.
 *        Each clock object belongs to a single ModuleInstance in the system.
 *
 * @note  This class assumes there's a global map, named "distribution_list",
 *        from distribution names to \ref Distribution "distributions".
 *        Such map is needed to assign the stochastic distributions
 *        to the Clock objects.
 */
class Clock
{
	// Friends for RNG handling, e.g. re-seeding
	friend class ModelSuite;
	friend class Transition;

public:

	/// Number of offered pseudo Random Number Generator algorithms, aka RNGs
	static constexpr size_t NUM_RNGS = 3ul;

	/// Default RNG algorithm
	static const std::pair<const char*,const char*> DEFAULT_RNG;

	/// Default seed for the RNG
	static const size_t DEFAULT_RNG_SEED;

private:

	/// Whether to use randomized RNG seeding (affects all clocks)
	static bool randomSeed_;

	/// Clock name
	std::string name_;

	/// Clock's distribution
	std::shared_ptr<Distribution> dist_;

public:  // Class' RNG observers

	/// RNGs offered to the user,
	/// as he should requested them through the CLI/GUI.
	/// @note Implements the <a href="https://goo.gl/yhTgLq"><i>Construct On
	///       First Use</i> idiom</a> for static data members,
	///       to avoid the <a href="https://goo.gl/chH5Kg"><i>static
	///       initialization order fiasco</i>>.
	static const std::array< std::string, NUM_RNGS >& RNGs() noexcept;

	/// Current RNG used internally for clock values sampling
	static const std::string& rng_type() noexcept;

	/// Seed used to initialized the internal RNG.
	static unsigned long rng_seed() noexcept;

	/// Whether randomized seeding is used
	static bool rng_seed_is_random() noexcept;

private:  // RNG manipulation via ModelSuite


	/**
	 * @brief Change the internal RNG used (must be an eligible option)
	 * @note The new RNG is seeded with the \ref rng_seed() "current seed"
	 * @see RNGs()
	 * @throw FigException if invalid argument passed
	 */
	static void change_rng(const std::string& rngType);

	/**
	 * @brief Change seed used by the internal RNG.
	 * @param seed Seed to use in following calls to seed_rng().
	 * @note Passing 0 as value of \p seed turns on <i>randomized seeding</i>.
	 * @note This doesn't re-seed the RNG; it changes the internally stored
	 *       seed value. To actually re-seed call seed_rng() afterwards.
	 * @see seed_rng()
	 */
	static void change_rng_seed(unsigned long seed);

	/**
	 * @brief Restart RNG sequence
	 * @details Re-seed the RNG with the last value specified with
	 *          change_rng_seed(), or a default value if none was.
	 * @note Seeding might be randomized, see change_rng_seed()
	 * @see change_rng_seed()
	 */
	static void seed_rng();

public:  // Ctors

	Clock(const std::string& clockName,
		  const std::string& distName,
	      const DistributionParameters& params);

	virtual ~Clock() { }

	Clock(const Clock& that)            = default;
	Clock(Clock&& that)                 = default;
	Clock& operator=(const Clock& that) = default;
	Clock& operator=(Clock&& that)      = default;

public:  // Accessors

	/// @copydoc name_
	const std::string& name() const noexcept { return name_; }

	/// @copydoc distName_
	const std::string& dist_name() const noexcept { return dist_->name; }

	/// @copydoc distParams_
	inline const DistributionParameters& distribution_params() const noexcept
	    { return dist_->params; }

public:  // Utils

	/// @brief Sample new time, distributed according to our distribution PDF
	inline CLOCK_INTERNAL_TYPE sample()   const { return dist_->sample(); }
	/// @brief Sample new time from distribution, conditioned on the given \par elapsedTime
	inline CLOCK_INTERNAL_TYPE resample(const CLOCK_INTERNAL_TYPE& elapsedTime) const
	    { return dist_->sample_conditional(elapsedTime); }
//	inline CLOCK_INTERNAL_TYPE operator()() const { return (*dist_)(); }

public:  // Debugging info
	void print_info(std::ostream &out) const;
};


} // namespace fig

#endif // CLOCK_H
