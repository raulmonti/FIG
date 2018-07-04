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
namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

typedef  fig::CLOCK_INTERNAL_TYPE                       return_t;
typedef  fig::DistributionParameters                    params_t;
typedef  pcg_extras::seed_seq_from<std::random_device>  PCGSeedSeq;

typedef  std::mt19937_64   MT64_t;
typedef  pcg32_k16384     PCG32_t;
typedef  pcg64_oneseq     PCG64_t;


/**
 * @brief RNG interface offered to clocks for time sampling
 *
 *        BasicRNG offers an interface matching the
 *       \ref http://en.cppreference.com/w/cpp/concept/UniformRandomBitGenerator
 *       "UniformRandomBitGenerator C++ concept".
 *        Implementing classes which derive from this interface
 *        allows an easy switch between RNG types upon the user's request.
 */
class BasicRNG
{
public:
	virtual ~BasicRNG() {}
public:
	typedef unsigned long result_type;
	virtual result_type min() const = 0;
	virtual result_type max() const = 0;
	virtual result_type operator()() = 0;
	virtual void seed(result_type s) = 0;
};


/// BasicRNG instance for C++ STL's 64 bit Mersenne-Twister RNG
class BasicRNG_MT64 : public BasicRNG, MT64_t
{
	typedef BasicRNG::result_type result_type;
public:  // Ctors
	BasicRNG_MT64(result_type seed)  : MT64_t(seed) {}
	BasicRNG_MT64(const MT64_t& rng) : MT64_t(rng)  {}
	BasicRNG_MT64(MT64_t&& rng)      : MT64_t(rng)  {}
	BasicRNG_MT64& operator=(MT64_t rng)
	{
		std::swap(static_cast<MT64_t&>(*this), rng);
		return *this;
	}
public:
	result_type min() const  override { return MT64_t::min(); }
	result_type max() const  override { return MT64_t::max(); }
	result_type operator()() override { return dynamic_cast<MT64_t&>(*this)(); }
	void seed(result_type s) override { dynamic_cast<MT64_t&>(*this).seed(s);  }
};


/// BasicRNG instance for PCG-family 32 bit RNG (with a huge period)
class BasicRNG_PCG32 : public BasicRNG, PCG32_t
{
	typedef BasicRNG::result_type result_type;
public:  // Ctors
	BasicRNG_PCG32(result_type seed)   : PCG32_t(seed) {}
	BasicRNG_PCG32(const PCG32_t& rng) : PCG32_t(rng)  {}
	BasicRNG_PCG32(PCG32_t&& rng)      : PCG32_t(rng)  {}
	BasicRNG_PCG32& operator=(PCG32_t rng)
	{
		std::swap(static_cast<PCG32_t&>(*this), rng);
		return *this;
	}
public:
	result_type min() const  override { return PCG32_t::min(); }
	result_type max() const  override { return PCG32_t::max(); }
	result_type operator()() override { return dynamic_cast<PCG32_t&>(*this)(); }
	void seed(result_type s) override { dynamic_cast<PCG32_t&>(*this).seed(s);  }
};


/// BasicRNG instance for PCG-family 64 bit RNG
class BasicRNG_PCG64 : public BasicRNG, PCG64_t
{
	typedef BasicRNG::result_type result_type;
public:  // Ctors
	BasicRNG_PCG64(result_type seed)   : PCG64_t(seed) {}
	BasicRNG_PCG64(const PCG64_t& rng) : PCG64_t(rng)  {}
	BasicRNG_PCG64(PCG64_t&& rng)      : PCG64_t(rng)  {}
	BasicRNG_PCG64& operator=(PCG64_t rng)
	{
		std::swap(static_cast<PCG64_t&>(*this), rng);
		return *this;
	}
public:
	result_type min() const  override { return PCG64_t::min(); }
	result_type max() const  override { return PCG64_t::max(); }
	result_type operator()() override { return dynamic_cast<PCG64_t&>(*this)(); }
	void seed(result_type s) override { dynamic_cast<PCG64_t&>(*this).seed(s);  }
};


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
unsigned long rngSeed =
#if   !defined RANDOM_RNG_SEED && !defined PCG_RNG
  std::mt19937_64::default_seed;
#elif !defined RANDOM_RNG_SEED &&  defined PCG_RNG
  0xCAFEF00DD15EA5E5ull;  // PCG's default seed
#elif  defined RANDOM_RNG_SEED && !defined PCG_RNG
  MT_nondet_RNG();
#else
  pcg_extras::generate_one<size_t>(std::forward<PCGSeedSeq>(PCG_nondet_RNG));
#endif


/// Collection of available RNGs
/// @note Updates in the keys of this unordered_map must be reflected in
///       \ref fig::Clock::DEFAULT_RNG,
///       \ref fig::Clock::RNGs(), and
///       \ref fig::Clock::change_rng_seed()
std::unordered_map< std::string, std::shared_ptr< BasicRNG > > RNGs =
{
    {"mt64",  std::make_shared< BasicRNG_MT64  >(rngSeed) },
    {"pcg32", std::make_shared< BasicRNG_PCG32 >(rngSeed) },
    {"pcg64", std::make_shared< BasicRNG_PCG64 >(rngSeed) }
};

/// Current RNG
std::string rngType(fig::Clock::DEFAULT_RNG);

/// RNG instance
auto rng = RNGs[rngType];


/// Random deviate ~ Uniform[a,b]<br>
///  where 'a' = params[0] is the lower bound<br>
///    and 'b' = params[1] is the upper bound.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Uniform_distribution_(continuous)">the wiki</a>
return_t uniform(const params_t& params)
{
	std::uniform_real_distribution< fig::CLOCK_INTERNAL_TYPE > uni(params[0], params[1]);
	return uni(*rng);
}


/// Random deviate ~ Exponential(lambda)<br>
///  where 'lambda' = params[0] is the rate.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Exponential_distribution">the wiki</a>
return_t exponential(const params_t& params)
{
	std::exponential_distribution< fig::CLOCK_INTERNAL_TYPE > exp(params[0]);
	return exp(*rng);
}


/// Random deviate ~ Normal(m,sd)<br>
///  where  'm' = params[0] is the mean<br>
///    and 'sd' = params[1] is the standard deviation.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Normal_distribution">the wiki</a>
return_t normal(const params_t& params)
{
	std::normal_distribution< fig::CLOCK_INTERNAL_TYPE > normal(params[0], params[1]);
	return std::max<double>(0.000001, normal(*rng));
}


/// Random deviate ~ Lognormal(m,sd)<br>
///  where  'm' = params[0] is the mean<br>
///    and 'sd' = params[1] is the standard deviation<br>
/// of the inherent normally distributed random variable.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Log-normal_distribution">the wiki</a>
return_t lognormal(const params_t& params)
{
	std::lognormal_distribution< fig::CLOCK_INTERNAL_TYPE > lognormal(params[0], params[1]);
	return lognormal(*rng);
}


/// Random deviate ~ Weibull(a,b)<br>
///  where 'a' = params[0] is the shape parameter<br>
///    and 'b' = params[1] is the scale parameter.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Weibull_distribution">the wiki</a>
return_t weibull(const params_t& params)
{
	std::weibull_distribution< fig::CLOCK_INTERNAL_TYPE > weibull(params[0], params[1]);
	return weibull(*rng);
}


/// Random deviate ~ Rayleigh(s) ~ Weibull(2,s*sqrt(2))<br>
///  where 's' = params[0] is the scale parameter.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Rayleigh_distribution">the wiki</a>
return_t rayleigh(const params_t& params)
{
	std::weibull_distribution< fig::CLOCK_INTERNAL_TYPE > rayleigh(2.0, params[0]*M_SQRT2);
	return rayleigh(*rng);
}


/// Random deviate ~ Gamma(a,b)<br>
///  where 'a' = params[0] is the shape parameter<br>
///    and 'b' = params[1] is the scale parameter (aka reciprocal of the rate)<br>
/// Check <a href="https://en.wikipedia.org/wiki/Gamma_distribution">the wiki</a>
return_t gamma(const params_t& params)
{
	std::gamma_distribution< fig::CLOCK_INTERNAL_TYPE > gamma(params[0], params[1]);
	return gamma(*rng);
}


/// Random deviate ~ Erlang(k,l) ~ Gamma(k,1/l)<br>
///  where 'k' = params[0] is the <i>integral</i> shape parameter<br>
///    and 'l' = params[1] is the rate parameter (aka reciprocal of the scale)<br>
/// Check <a href="https://en.wikipedia.org/wiki/Erlang_distribution">the wiki</a>
return_t erlang(const params_t& params)
{
	const int k(std::round(params[0]));
	std::gamma_distribution< fig::CLOCK_INTERNAL_TYPE > erlang(k, 1.0/params[1]);
	return erlang(*rng);
}

} // namespace  // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

#ifndef RANDOM_RNG_SEED
  bool Clock::randomSeed_ = false;
#else
  bool Clock::randomSeed_ = true;
#endif


const std::array<std::string, Clock::NUM_RNGS >&
Clock::RNGs() noexcept
{
	// NOTE: this must reflect the key values of the ::RNGs unordered map
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


bool Clock::rng_seed_is_random() noexcept { return randomSeed_; }


void Clock::change_rng(const std::string& rngType)
{
	static const auto& available_rngs(Clock::RNGs());
	if (end(available_rngs) ==
			find(begin(available_rngs), end(available_rngs), rngType))
		throw_FigException("invalid RNG type specified: " + rngType);
	::rngType = rngType;
	rng = ::RNGs[rngType];
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
	rng->seed(rngSeed);  // if non randomized, this repeats the sequence
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

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
