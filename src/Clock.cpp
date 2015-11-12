//==============================================================================
//
//  Clock.cpp
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

// C++
#include <random>
#include <unordered_map>
// FIG
#include <Clock.h>


namespace
{

#ifndef NDEBUG
const unsigned int rngSeed(1234567803u);  // repeatable outcome
#else
const unsigned int rngSeed(std::random_device{}());
#endif

std::mt19937_64  MTrng(rngSeed);
std::minstd_rand LCrng(rngSeed);

#ifdef HQ_RNG
auto rng = MTrng;  // Mersenne-Twister high quality RNG
#else
auto rng = LCrng;  // Linear-congruential  standard RNG
#endif

std::default_random_engine random_gen;
std::uniform_real_distribution< fig::CLOCK_INTERNAL_TYPE > uniform01(0.0 , 1.0);
std::normal_distribution< fig::CLOCK_INTERNAL_TYPE > normal01(0.0 , 1.0);

} // namespace


namespace fig
{
// We don't need to capture global variables in lambda expressions,
// e.g. the anonymously namespaced "rng" (http://stackoverflow.com/a/20362378)

std::unordered_map< std::string, Distribution >
distributions_list = {
	{"uniform01",
	  [] (const DistributionParameters& params)
	  { return uniform01(rng); }},
	{"uniformAB",
	  [] (const DistributionParameters& params)
	  { return params[0] + (params[1] - params[0]) * uniform01(rng); }}
};

} // namespace fig
