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
//	along with PRISM; if not, write to the Free Software Foundation,
//	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//==============================================================================


#ifndef CLOCK_H
#define CLOCK_H

// C++
#include <array>
#include <string>
#include <unordered_map>
#include <functional>
// C
#include <cassert>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


namespace fig
{

#define  NUM_DISTRIBUTION_PARAMS  4u  // argument list for any distribution

typedef  float  CLOCK_INTERNAL_TYPE;

typedef std::array< const CLOCK_INTERNAL_TYPE , NUM_DISTRIBUTION_PARAMS >
	DistributionParameters;

typedef std::function< CLOCK_INTERNAL_TYPE (const DistributionParameters&) >
	Distribution;

/// List of distributions offered for time sampling
extern std::unordered_map< std::string, Distribution > distributions_list;


/**
 * IOSAs' clock, the internal stochastic time passage mechanism.
 */
class Clock
{
	const std::string& distName_;
	const Distribution& dist_;  // *copy* of one from distribution_list
	const DistributionParameters distParams_;

public:

	Clock(const std::string& distName,
		  const DistributionParameters& params) :
		dist_(distributions_list.at(distName)),  // may throw out_of_range
		distName_(distName),
		distParams_(params)
		{
			assert(!distName_.empty());
		}

	/// Name of our distribution function
	inline const std::string& distribution() const { return distName_; }

	/// Parameters characterizing our distribution function
	inline const DistributionParameters& distribution_params() const
	{ return distParams_; }

	/// Sample our distribution function
	inline CLOCK_INTERNAL_TYPE sample() const     { return dist_(distParams_); }
	inline CLOCK_INTERNAL_TYPE operator()() const { return dist_(distParams_); }
};


} // namespace fig

#endif // CLOCK_H
