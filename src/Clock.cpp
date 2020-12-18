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
#include <map>
// External code
#include <pcg_random.hpp>
// FIG
#include <Clock.h>
#include <core_typedefs.h>
#include <FigException.h>


#ifndef M_SQRT2f32
#  define M_SQRT2f32 M_SQRT2
#endif


// ADL
using std::find;
using std::begin;
using std::end;


/// @brief RNG and distributions available for time sampling of the clocks
namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

typedef  fig::CLOCK_INTERNAL_TYPE     time_t;
typedef  time_t                       return_t;
typedef  fig::DistributionParameters  params_t;
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
std::string rngType(fig::Clock::DEFAULT_RNG.first);

/// RNG instance
auto rng = RNGs[rngType];

/// For home-made random deviates
thread_local std::uniform_real_distribution< float > Unif01(0.0f,1.0f);
thread_local std::normal_distribution< float > Z(0.0f,1.0f);
struct GammaHash { std::size_t operator()(const params_t& p) const { return p[0]*p[0]*3 + p[1]*p[1]*100; } };
struct GammaEq { bool operator()(const params_t& p1, const params_t& p2) const { return p1[0]==p2[0] && p1[1]==p2[1]; } };
struct GammaFun {
	std::gamma_distribution<time_t> fun;
	GammaFun(const params_t& params) : fun(params[0], params[1]) {}
};
thread_local std::unordered_map< params_t, GammaFun, GammaHash, GammaEq > stored_Gammas;


/// Random deviate ~ Uniform[a,b]<br>
///  where \par a = params[0] is the lower bound,<br>
///    and \par b = params[1] is the upper bound.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Uniform_distribution_(continuous)">the wiki</a>
return_t uniform(const params_t& params)
{

//	std::uniform_real_distribution< fig::CLOCK_INTERNAL_TYPE > uni(params[0], params[1]);
//	return uni(*rng);

	assert(params[0] < params[1]);
	return (params[1]-params[0]) * static_cast<time_t>(Unif01(*rng)) + params[0];
}


/// Random deviate ~ Exponential(lambda)<br>
///  where \par lambda = params[0] is the rate.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Exponential_distribution">the wiki</a>
return_t exponential(const params_t& params)
{

//	std::exponential_distribution< fig::CLOCK_INTERNAL_TYPE > exp(params[0]);
//	return exp(*rng);

	assert(static_cast< time_t >(0) < params[0]);
	__volatile__ float u;
	do {
		u = Unif01(*rng);
	} while (u <= 0.0f);
	return static_cast<time_t>(-1.0*std::log(u)) / params[0];
}


/// Random deviate ~ Hyper-Exponential(p:lambda1,1-p:lambda2)<br>
///  where \par p = params[0] is the probability of choosing the first rate,<br>
///    and 1 - \par p is the probability of choosing the second rate,<br>
///    and \par lambda1 = params[1] is the first  possible rate,<br>
///    and \par lambda2 = params[2] is the second possible rate.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Hyperexponential_distribution">the wiki</a>
return_t hyperexponential2(const params_t& params)
{
	assert(static_cast<time_t>(0) < params[2]);
	assert(static_cast<time_t>(0) < params[1]);
	assert(static_cast<time_t>(0) < params[0]);
	assert(params[0] < static_cast<time_t>(1.0));
	__volatile__ float u;
	do {
		u = Unif01(*rng);
	} while (u <= 0.0f);
	return static_cast<time_t>(-std::log(u)) /
	    (static_cast<time_t>(Unif01(*rng)) < params[0] ? params[1]
	                                                   : params[2]);
}


//	Hyper-Exponential-3 distribution would require at least 5 parameters,
//	and currently NUM_DISTRIBUTION_PARAMS == 4 in core_typedefs.h
//	/// Random deviate ~ Hyper-Exponential(p1:lambda1,p2:lambda2,1-p1-p2:lambda3)<br>
//	///  where \par p1 = params[0] is the probability of choosing the first rate,<br>
//	///    and \par p1 = params[1] is the probability of choosing the second rate,<br>
//	///    and 1 - \par p1 - \par p2 is the probability of choosing the third rate,<br>
//	///    and \par lambda1 = params[2] is the first  possible rate,<br>
//	///    and \par lambda2 = params[3] is the second possible rate,<br>
//	///    and \par lambda3 = params[4] is the third  possible rate.<br>
//	/// Check <a href="https://en.wikipedia.org/wiki/HyperExponential_distribution">the wiki</a>
//	return_t hyperexponential3(const params_t& params)
//	{
//		std::exponential_distribution< fig::CLOCK_INTERNAL_TYPE > exp(params[0]);
//		return exp(*rng);
//	}


/// Random deviate ~ Normal(m,sd)<br>
///  where \par  m = params[0] is the mean,<br>
///    and \par sd = params[1] is the standard deviation.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Normal_distribution">the wiki</a>
/// @bug BUG: when taking the max with 0.000001 (to avoid sampling non-positive
///           time delays) we lose probability mass. This mass must be added to
///           the remaining (positively-supported) mass of the distribution.
return_t normal(const params_t& params)
{

//	std::normal_distribution< fig::CLOCK_INTERNAL_TYPE > normal(params[0], params[1]);
//	return std::max(0.000001f, normal(*rng));

	assert(static_cast<time_t>(0) < params[1]);
	return std::max(static_cast<time_t>(0.000001f),
	                params[0] + params[1] * static_cast<time_t>(Z(*rng)));
}


/// Random deviate ~ Lognormal(m,sd)<br>
///  where \par  m = params[0] is the mean,<br>
///    and \par sd = params[1] is the standard deviation<br>
/// of the inherent normally distributed random variable.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Log-normal_distribution">the wiki</a>
return_t lognormal(const params_t& params)
{

//	std::lognormal_distribution< fig::CLOCK_INTERNAL_TYPE > lognormal(params[0], params[1]);
//	return lognormal(*rng);

	assert(static_cast<time_t>(0) < params[1]);
	return static_cast<time_t>(std::exp(params[0] + params[1] * static_cast<time_t>(Z(*rng))));
}


/// Random deviate ~ Weibull(a,b)<br>
///  where \par a = params[0] is the shape parameter, aka \par k,<br>
///    and \par b = params[1] is the scale parameter, aka \par lambda.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Weibull_distribution">the wiki</a>
return_t weibull(const params_t& params)
{

//	std::weibull_distribution< fig::CLOCK_INTERNAL_TYPE > weibull(params[0], params[1]);
//	return weibull(*rng);

	assert(static_cast<time_t>(0) < params[0]);
	assert(static_cast<time_t>(0) < params[1]);
	__volatile__ float u;
	do {
		u = Unif01(*rng);
	} while (u <= 0.0f);
	return params[1] * std::pow<time_t>(-std::log(u), 1.0f/static_cast<float>(params[0]));
}


/// Random deviate ~ Rayleigh(s) ~ Weibull(2,s*sqrt(2))<br>
///  where \par s = params[0] is the scale parameter.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Rayleigh_distribution">the wiki</a>
return_t rayleigh(const params_t& params)
{

//	std::weibull_distribution< fig::CLOCK_INTERNAL_TYPE > rayleigh(2.0, params[0]*M_SQRT2f32);
//	return rayleigh(*rng);

	assert(static_cast<time_t>(0) < params[0]);
	__volatile__ float u;
	do {
		u = Unif01(*rng);
	} while (u <= 0.0f);
	return params[0] * std::sqrt(-2.0f * std::log(u));
}


/// Random deviate ~ Gamma(a,b)<br>
///  where \par a = params[0] is the shape parameter,<br>
///    and \par b = params[1] is the scale parameter, aka reciprocal of the rate.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Gamma_distribution">the wiki</a>
return_t gamma(const params_t& params)
{

//	std::gamma_distribution< fig::CLOCK_INTERNAL_TYPE > gamma(params[0], params[1]);
//	return gamma(*rng);

	// If requested Gamma doesn't exists yet, create it
	if (end(stored_Gammas) == stored_Gammas.find(params))
		stored_Gammas.emplace(params, params);
	return stored_Gammas.at(params).fun(*rng);
}


/// Random deviate ~ Erlang(k,l) ~ Gamma(k,1/l)<br>
///  where \par k = params[0] is the <em>integral</em> shape parameter,<br>
///    and \par l = params[1] is the rate parameter, aka reciprocal of the scale.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Erlang_distribution">the wiki</a>
return_t erlang(const params_t& params)
{

//	const int k(static_cast<int>(std::round(params[0])));
//	std::gamma_distribution< fig::CLOCK_INTERNAL_TYPE > erlang(k, 1.0f/params[1]);
//	return erlang(*rng);

	const int k(static_cast<int>(std::round(params[0])));
	assert(0 < k);
	__volatile__ float acum = 1.0f;
	for (int i=0 ; i<k ; i++)
		acum *= Unif01(*rng);
	return static_cast<time_t>(-log(acum)) / params[1];
}


/// (Non-) Random deviate ~ Dirac(x)<br>
///  where \par [x,x] is the <em>point-wise</em> support of the distribution<br>
/// Check <a href="https://en.wikipedia.org/wiki/Dirac_delta_function">the wiki</a>
return_t dirac(const params_t& params)
{
	return params[0];
}

} // namespace  // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

const std::pair<const char*,const char*> Clock::DEFAULT_RNG =
#ifndef PCG_RNG
        std::make_pair("mt64", "64-bit STL's Mersenne-Twister");
#elif !defined NDEBUG
        std::make_pair("pcg64", "64-bit PCG-family generator");
#else
        std::make_pair("pcg32", "32-bit PCG-family generator with huge period");
#endif

#ifndef RANDOM_RNG_SEED
  bool Clock::randomSeed_ = false;
#else
  bool Clock::randomSeed_ = true;
#endif

const size_t Clock::DEFAULT_RNG_SEED =
#ifdef RANDOM_RNG_SEED
        0ul;
#elif !defined PCG_RNG
        5489ul;  // C++ STL's random.h: class mersenne_twister_engine
#else
        0xCAFEF00DD15EA5E5ull;  // pcg_random.hpp: class engine ctor
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
    {"dirac",       dirac      },
	{"hyperexponential2", hyperexponential2},
};

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
