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

// C
#include <cmath>    // std::round()
// C++
#include <random>
#include <numeric>  // std::max<>
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

#ifndef HQ_RNG
auto rng = LCrng;  // Linear-congruential  standard RNG
#else
auto rng = MTrng;  // Mersenne-Twister high quality RNG
#endif

std::uniform_real_distribution< fig::CLOCK_INTERNAL_TYPE > uniform01(0.0 , 1.0);
std::exponential_distribution< fig::CLOCK_INTERNAL_TYPE > exponential1(1.0);
std::normal_distribution< fig::CLOCK_INTERNAL_TYPE > normal01(0.0 , 1.0);

} // namespace


namespace fig
{
// We don't need to capture global variables in lambda expressions,
// e.g. the anonymously namespaced "rng" (http://stackoverflow.com/a/20362378)

std::unordered_map< std::string, Distribution > distributions_list =
{
	{"uniform",
	  [] (const DistributionParameters& params)
	  { // Generates a random deviate ~ Uniform[a,b],
		// where 'a' = params[0] is the lower bound
		//   and 'b' = params[1] is the upper bound
		return params[0] + (params[1] - params[0]) * uniform01(rng);
	  }},

	{"exponential",
	  [] (const DistributionParameters& params)
	  { // Generates a random deviate ~ Exponential(lambda)
		// where 'lambda' = params[0] is the rate
		return exponential1(rng) / params[0];  // Grisel me dijo que es así
	  }},

	{"normal",
	  [] (const DistributionParameters& params)
	  { // Generates a random deviate ~ Normal(m,sd)
		// where  'm' = params[0] is the mean
		//   and 'sd' = params[1] is the standard deviation
		return std::max<double>(0.000001, normal01(rng) * params[1] + params[0]);
	  }},

	{"gamma",
	  [] (const DistributionParameters& params)
	  { // Generates a random deviate ~ Gamma(a,b)
		// where 'a' = params[0] is the shape parameter
		//   and 'b' = params[1] is the scale parameter (aka reciprocal of the rate)
		std::gamma_distribution< fig::CLOCK_INTERNAL_TYPE > gamma(params[0], params[1]);
		return gamma(rng);
	  }},

	{"erlang",
	  [] (const DistributionParameters& params)
	  { // Generates a random deviate ~ Erlang(k,mu) ~ Gamma(k,mu)
		// where  'k' = params[0] is the *integer* shape parameter
		//   and 'mu' = params[1] is the scale parameter (aka reciprocal of the rate)
		const int k(std::round(params[0]));
		std::gamma_distribution< fig::CLOCK_INTERNAL_TYPE > erlang(k, params[1]);
		return erlang(rng);
	  }}
};

} // namespace fig
