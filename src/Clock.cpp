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
#include <core_typedefs.h>


namespace
{

typedef  fig::CLOCK_INTERNAL_TYPE     return_t;
typedef  fig::DistributionParameters  params_t;

#ifndef NDEBUG
const unsigned int rngSeed(1234567803u);  // repeatable outcome
#else
const unsigned int rngSeed(std::random_device{}());
#endif

std::mt19937_64  MTrng(rngSeed);
std::minstd_rand LCrng(rngSeed);

#ifndef HQ_RNG
/// Linear-congruential standard RNG
auto rng = LCrng;
#else
/// Mersenne-Twister high quality RNG
auto rng = MTrng;
#endif

std::uniform_real_distribution< fig::CLOCK_INTERNAL_TYPE > uniform01(0.0 , 1.0);
std::exponential_distribution< fig::CLOCK_INTERNAL_TYPE > exponential1(1.0);
std::normal_distribution< fig::CLOCK_INTERNAL_TYPE > normal01(0.0 , 1.0);


/// Random deviate ~ Uniform[a,b]<br>
///  where 'a' = params[0] is the lower bound<br>
///    and 'b' = params[1] is the upper bound<br>
/// Check <a href="https://en.wikipedia.org/wiki/Uniform_distribution_(continuous)">the wiki</a>
return_t uniform(const params_t& params)
{
	return params[0] + (params[1] - params[0]) * uniform01(rng);
}


/// Random deviate ~ Exponential(lambda)<br>
///  where 'lambda' = params[0] is the rate
/// Check <a href="https://en.wikipedia.org/wiki/Exponential_distribution">the wiki</a>
return_t exponential(const params_t& params)
{
	return exponential1(rng) / params[0];  // Grisel me dijo que es así
}


/// Random deviate ~ Normal(m,sd)<br>
///  where  'm' = params[0] is the mean<br>
///    and 'sd' = params[1] is the standard deviation
/// Check <a href="https://en.wikipedia.org/wiki/Normal_distribution">the wiki</a>
return_t normal(const params_t& params)
{
	return std::max<double>(0.000001, normal01(rng) * params[1] + params[0]);
}


/// Random deviate ~ Gamma(a,b)<br>
///  where 'a' = params[0] is the shape parameter<br>
///    and 'b' = params[1] is the scale parameter (aka reciprocal of the rate)<br>
/// Check <a href="https://en.wikipedia.org/wiki/Gamma_distribution">the wiki</a>
return_t gamma(const params_t& params)
{
	std::gamma_distribution< fig::CLOCK_INTERNAL_TYPE > gamma(params[0], params[1]);
	return gamma(rng);
}


/// Random deviate ~ Erlang(k,l) ~ Gamma(k,1/l)<br>
///  where 'k' = params[0] is the <i>integral</i> shape parameter<br>
///    and 'l' = params[1] is the rate parameter (aka reciprocal of the scale)<br>
/// Check <a href="https://en.wikipedia.org/wiki/Erlang_distribution">the wiki</a>
return_t erlang(const params_t& params)
{
	const int k(std::round(params[0]));
    std::gamma_distribution< fig::CLOCK_INTERNAL_TYPE > erlang(k, 1.0/params[1]);
	return erlang(rng);
}

} // namespace



namespace fig
{

std::unordered_map< std::string, Distribution > distributions_list =
{
	{"uniform",     uniform    },
	{"exponential", exponential},
	{"normal",      normal     },
	{"gamma",       gamma      },
	{"erlang",      erlang     },
};

} // namespace fig
