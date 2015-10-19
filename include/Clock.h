//==============================================================================
//
//	Clock.h
//
//	Copyleft 2015-
//	Authors:
//  * Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
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
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
// C
#include <cassert>


namespace fig
{

typedef float CLOCK_INTERNAL_TYPE;
template< typename T_ > using ParamList = std::vector< T_ >;  // size: MAX_NUM_P

typedef std::function< CLOCK_INTERNAL_TYPE (const ParamList< const CLOCK_INTERNAL_TYPE >&) >
		Distribution;
std::unordered_map< std::string, Distribution > distributions_list;  // size: NUM_DISTS


/**
 * IOSAs' clock, the internal stochastic time passage mechanism.
 */
class Clock
{
	const Distribution&  dist_;  // *copy* of one from distribution_list
	const std::string&   distName_;
	const ParamList< CLOCK_INTERNAL_TYPE > distParams_;

public:

	Clock(const std::string& distName,
		  const ParamList< CLOCK_INTERNAL_TYPE >& params) :
		dist_(distributions_list.at(distName)),  // may throw
		distName_(distName),
		distParams_(params)
		{
			assert(!distName_.empty());
			assert(!distParams_.empty());
		}

	/// Name of our distribution function
	inline const std::string& distribution() const { return distName_; }

	/// Sample our distribution function
	inline const CLOCK_INTERNAL_TYPE sample() const/*RNG state?*/ { return dist_(distParams_); }
};

} // namespace fig

#endif // CLOCK_H
