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

class ModuleInstance;  // Fwd declaration for internal pointer

/// Global container with distributions offered for time sampling
extern std::unordered_map< std::string, Distribution > distributions_list;

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
	/// Clock name
	std::string name_;

	/// Clock's distribution name
	std::string distName_;

	/// Clock's distribution
	Distribution dist_;  // *copy* of one from distribution_list

	/// Clock's distribution parameters
	DistributionParameters distParams_;

public:  // Ctors

	Clock(const std::string& clockName,
		  const std::string& distName,
		  const DistributionParameters& params) :
		name_(clockName),
		distName_(distName),
		dist_(distributions_list.at(distName)),  // may throw out_of_range
		distParams_(params)
		{
			assert(!clockName.empty());
		}

	Clock(const Clock& that)            = default;
	Clock(Clock&& that)                 = default;
	Clock& operator=(const Clock& that) = default;
	Clock& operator=(Clock&& that)      = default;

public:  // Accessors

	/// @copydoc name_
	const std::string& name() const noexcept { return name_; }

	/// @copydoc distName_
	const std::string& dist_name() const noexcept { return distName_; }

	/// @copydoc distParams_
	inline const DistributionParameters& distribution_params() const noexcept
		{ return distParams_; }

public:  // Utils

	/// @brief Sample our distribution function
	inline CLOCK_INTERNAL_TYPE sample()     const { return dist_(distParams_); }
	inline CLOCK_INTERNAL_TYPE operator()() const { return dist_(distParams_); }
};


} // namespace fig

#endif // CLOCK_H
