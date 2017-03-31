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
#include <cmath>      // std::round()
// C++
#include <array>
#include <random>
#include <numeric>    // std::max<>
#include <iterator>   // std::begin(), std::end()
#include <algorithm>  // std::find()
#include <unordered_map>
// External code
#include <pcg_random.hpp>
// FIG
#include <Clock.h>
#include <core_typedefs.h>
#include <FigException.h>


// ADL
using std::find;
using std::begin;
using std::end;


/// @brief RNG and distributions available for time sampling of the clocks
namespace
{

typedef  fig::CLOCK_INTERNAL_TYPE                       return_t;
typedef  fig::DistributionParameters                    params_t;
typedef  pcg_extras::seed_seq_from<std::random_device>  PCGSeedSeq;


/// Non-deterministic random number generator for randomized seeding,
/// used by Mersenne-Twister RNG
std::random_device MT_nondet_RNG;

/// Non-deterministic random number generator for randomized seeding,
/// used by PCG-family RNG
pcg_extras::seed_seq_from<std::random_device> PCG_nondet_RNG;


/// Default RNG seed:
/// \if RANDOM_RNG_SEED
///   use system's random device (a quasi-random number generator)
/// \else
///   deterministic (RNG's default)
/// \endif
#if   !defined RANDOM_RNG_SEED && !defined PCG_RNG
  unsigned long rngSeed(std::mt19937_64::default_seed);
#elif !defined RANDOM_RNG_SEED &&  defined PCG_RNG
  unsigned long rngSeed(0xCAFEF00DD15EA5E5ull);  // PCG's default seed
#elif  defined RANDOM_RNG_SEED && !defined PCG_RNG
  unsigned long rngSeed(MT_nondet_RNG());
#else
  unsigned long rngSeed = pcg_extras::generate_one<size_t>(
							  std::forward<PCGSeedSeq>(PCG_nondet_RNG));
#endif


// RNGs offered
// NOTE: reflect changes in RNGs_offered, rngType, and Clock::available_RNGs()
std::mt19937_64 MT_RNG(rngSeed);
pcg32_k16384 PCG32_RNG(rngSeed);
pcg64_oneseq PCG64_RNG(rngSeed);

class ClocksRNG
{
	// Members are based on the RNGs offered above
	std::mt19937_64*  mt64;
	pcg32_k16384*    pcg32;
	pcg64_oneseq*    pcg64;
	size_t i;

public:

	typedef unsigned long result_type;

public:

	ClocksRNG(std::mt19937_64& rng) :
		mt64(&rng),
		pcg32(nullptr),
		pcg64(nullptr),
		i(0ul)
	{}

	ClocksRNG(pcg32_k16384& rng) :
		mt64(nullptr),
		pcg32(&rng),
		pcg64(nullptr),
		i(1ul)
	{}

	ClocksRNG(pcg64_oneseq& rng) :
		mt64(nullptr),
		pcg32(nullptr),
		pcg64(&rng),
		i(2ul)
	{}

	static result_type min() {
		switch (i) :
		case 0:
			return mt64->min();
			break;
		case 1:
			return pcg32->min();
			break;
		case 2:
			return pcg64->min();
			break;
		default:
			throw_FigException("invalid RNG type");
			break;
	}

	result_type max() {
		switch (i) :
		case 0:
			return mt64->max();
			break;
		case 1:
			return pcg32->max();
			break;
		case 2:
			return pcg64->max();
			break;
		default:
			throw_FigException("invalid RNG type");
			break;
	}
};

//class ClocksRNG : public std::mt19937_64, pcg32_k16384, pcg64_oneseq
//	{ /* Inheritance is based on the RNGs offered above */ };
std::unordered_map< std::string, ClocksRNG > RNGs =
{
	{"mt64",  ClocksRNG(MT_RNG)   },
	{"pcg32", ClocksRNG(PCG32_RNG)},
	{"pcg64", ClocksRNG(PCG64_RNG)},
};

/// Default RNG: \if PCG_RNG PCG-family \else Mersenne-Twister (C++ STL) \endif
#ifndef PCG_RNG
  std::string rngType = "mt64";
#elif !defined NDEBUG
  std::string rngType = "pcg32";
#else
  std::string rngType = "pcg64";
#endif

/// RNG instance
auto rng = RNGs[rngType];


/// Default RNG: \if PCG_RNG PCG-family \else Mersenne-Twister (C++ STL) \endif
#ifndef PCG_RNG
  ClocksRNG& rng = RNGs_offered["mt64"];
#elif !defined NDEBUG
  ClocksRNG& rng = RNGs_offered["pcg32"];
#else
  ClocksRNG& rng = RNGs_offered["pcg64"];
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

#ifndef RANDOM_RNG_SEED
  bool Clock::randomSeed_ = false;
#else
  bool Clock::randomSeed_ = true;
#endif


const std::array<std::string, Clock::NUM_RNGS >&
Clock::RNGs() noexcept
{
	// NOTE: reflect updates in change_rng_seed()

	static const std::array< std::string, NUM_RNGS > RNGs =
	{{
		// 64 bit Mersenne-Twister (C++ STL)
		"mt64",

		// 32 bit RNG from PCG family (with an insanely huge period: 2^524352)
		"pcg32",

		// 64 bit RNG from PCG family (single sequence)
		"pcg64",
	}};
	return RNGs;
}


const std::string& Clock::rng_type() noexcept { return rngType; }


unsigned long Clock::rng_seed() noexcept { return rngSeed; }
//
// 	/// @todo TODO update, current code is incorrect
//
//
// 	if (randomSeed_)
// 		return 0ul;
// 	else
// 		return rngSeed;
// #if !defined RANDOM_RNG_SEED || !defined RNG_PCG
// 	return rngSeed;
// #else
// 	// return changing seeds, to make the user realize how's it coming
// 	typedef pcg_extras::seed_seq_from<std::random_device> SeedSeq;
// 	return pcg_extras::generate_one<size_t>(std::forward<SeedSeq>(rngSeed));
// #endif
// }


bool Clock::rng_seed_is_random() noexcept { return randomSeed_; }


void Clock::change_rng(const std::string& rngType)
{
	const auto& available_rngs(RNGs());
	if (end(available_rngs) ==
			find(begin(available_rngs), end(available_rngs), rngType))
		throw_FigException("invalid RNG type specified: " + rngType);
	::rngType = rngType;
	rng = RNGs[rngType];
	seed_rng();
}


void Clock::change_rng_seed(unsigned long seed)
{
	randomSeed_ = 0ul == seed;
	if (randomSeed_) {
		if (std::string::npos == rngType.find("pcg"))
			rngSeed = MT_nondet_RNG();
		else
			rngSeed = pcg_extras::generate_one<size_t>(
						  std::forward<PCGSeedSeq>(PCG_nondet_RNG));
	} else {
		rngSeed = seed;
	}
}


void Clock::seed_rng()
{
	if (randomSeed_)
		change_rng_seed(0ul);
	rng.seed(rngSeed);  // if non randomized, this repeats the sequence

	/// @todo TODO erase dead code below
//	#ifndef RANDOM_RNG_SEED
//		rng.seed(rngSeed);  // repeat sequence
//	#elif !defined PCG_RNG
//		rngSeed = std::random_device{}();
//		rng.seed(rngSeed);
//	#else
//		rng.seed(rngSeed);  // rngSeed is pcg_extras::seed_seq_from<std::random_device>
//	#endif
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
