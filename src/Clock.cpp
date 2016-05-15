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
// External code
#include <pcg_random.hpp>
// FIG
#include <Clock.h>
#include <core_typedefs.h>


/// @brief RNG and distributions available for time sampling of the clocks
namespace
{

typedef  fig::CLOCK_INTERNAL_TYPE     return_t;
typedef  fig::DistributionParameters  params_t;

/// RNG seed selection:
/// \ifnot RANDOM_RNG_SEED
///   deterministic
/// \else
///   taken from the system's random device
/// \endif
#if   !defined RANDOM_RNG_SEED && !defined PCG_RNG
  const unsigned long rngSeed(std::mt19937_64::default_seed);
#elif !defined RANDOM_RNG_SEED &&  defined PCG_RNG
  const unsigned long rngSeed(0xCAFEF00DD15EA5E5ull);  // PCG's default seed
#elif  defined RANDOM_RNG_SEED && !defined PCG_RNG
  unsigned long rngSeed(std::random_device{}());
#else
  pcg_extras::seed_seq_from<std::random_device> rngSeed;
#endif

/// \ifnot PCG_RNG Mersenne-Twister RNG \else PCG family RNG \endif
#ifndef PCG_RNG
  std::mt19937_64 rng(rngSeed);
#elif !defined NDEBUG
//  pcg64_fast rng(rngSeed);
  pcg32_k16384 rng(rngSeed);
#else
  pcg64 rng(rngSeed);
#endif


/// Random deviate ~ Uniform[a,b]<br>
///  where 'a' = params[0] is the lower bound<br>
///    and 'b' = params[1] is the upper bound.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Uniform_distribution_(continuous)">the wiki</a>
return_t uniform(const params_t& params)
{
	std::uniform_real_distribution< fig::CLOCK_INTERNAL_TYPE > uni(params[0], params[1]);
	return uni(rng);
}


/// Random deviate ~ Exponential(lambda)<br>
///  where 'lambda' = params[0] is the rate.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Exponential_distribution">the wiki</a>
return_t exponential(const params_t& params)
{
	std::exponential_distribution< fig::CLOCK_INTERNAL_TYPE > exp(params[0]);
	return exp(rng);
}


/// Random deviate ~ Normal(m,sd)<br>
///  where  'm' = params[0] is the mean<br>
///    and 'sd' = params[1] is the standard deviation.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Normal_distribution">the wiki</a>
return_t normal(const params_t& params)
{
	std::normal_distribution< fig::CLOCK_INTERNAL_TYPE > normal(params[0], params[1]);
	return std::max<double>(0.000001, normal(rng));
}


/// Random deviate ~ Lognormal(m,sd)<br>
///  where  'm' = params[0] is the mean<br>
///    and 'sd' = params[1] is the standard deviation<br>
/// of the inherent normally distributed random variable.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Log-normal_distribution">the wiki</a>
return_t lognormal(const params_t& params)
{
	std::lognormal_distribution< fig::CLOCK_INTERNAL_TYPE > lognormal(params[0], params[1]);
	return lognormal(rng);
}


/// Random deviate ~ Weibull(a,b)<br>
///  where 'a' = params[0] is the shape parameter<br>
///    and 'b' = params[1] is the scale parameter.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Weibull_distribution">the wiki</a>
return_t weibull(const params_t& params)
{
	std::weibull_distribution< fig::CLOCK_INTERNAL_TYPE > weibull(params[0], params[1]);
	return weibull(rng);
}


/// Random deviate ~ Rayleigh(s) ~ Weibull(2,s*sqrt(2))<br>
///  where 's' = params[0] is the scale parameter.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Rayleigh_distribution">the wiki</a>
return_t rayleigh(const params_t& params)
{
	std::weibull_distribution< fig::CLOCK_INTERNAL_TYPE > rayleigh(2.0, params[0]*M_SQRT2);
	return rayleigh(rng);
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

unsigned long Clock::rng_seed() noexcept
{
#if !defined RANDOM_RNG_SEED || !defined RNG_PCG
	return rngSeed;
#else
	// return changing seeds, to make the user realize how's it coming
	typedef pcg_extras::seed_seq_from<std::random_device> SeedSeq;
	return pcg_extras::generate_one<size_t>(std::forward<SeedSeq>(rngSeed));
#endif
}

void Clock::seed_rng()
{
#ifndef RANDOM_RNG_SEED
	rng.seed(rngSeed);  // repeat sequence
#elif !defined PCG_RNG
	rngSeed = std::random_device{}();
	rng.seed(rngSeed);
#else
	rng.seed(rngSeed);  // rngSeed is pcg_extras::seed_seq_from<std::random_device>
#endif
}

std::unordered_map< std::string, Distribution > distributions_list =
{
	{"uniform",     uniform    },
	{"exponential", exponential},
	{"normal",      normal     },
	{"lognormal",   lognormal  },
	{"weibull",     weibull    },
	{"rayleigh",    rayleigh   },
	{"gamma",       gamma      },
	{"erlang",      erlang     },
};

} // namespace fig
