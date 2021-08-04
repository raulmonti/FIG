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
#include <gsl_cdf.h>  // gsl_cdf_ugaussian_P(), gsl_cdf_gamma_P(), etc.
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
using std::pow;
using std::log;
using std::exp;


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

/// Random deviate for hand-made conditional resampling
thread_local std::uniform_real_distribution< float > Unif01(0.0f,1.0f);

[[noreturn]]
void
no_CDF_implementation(const std::string& distName)
{
	throw_FigException("the CDF for the " + distName + " distribution has not "
					   + "been implemented yet, aborting computations. Perhaps "
					   + "try again with the \"--no-resampling\" switch");
}

[[noreturn]]
void
no_conditional_sampling(const std::string& distName)
{
	throw_FigException("conditional sampling not implemented for " + distName
					   + " distribution, aborting computations. Try again"
					   + " with the \"--no-resampling\" switch");
}


/// Default ctor for unimplemented distributions
struct unknown_distribution : public fig::Distribution
{
	unknown_distribution(const std::string& name, const params_t& params)
	    : Distribution(name, params)
	{
		throw_FigException("unknown distribution requested: " + name);
	}
	inline float CDF(float) const override { no_CDF_implementation(name); }
	inline return_t sample() const override { return -1; }
	inline void sample_conditional(time_t&, time_t&) const override { no_conditional_sampling(name); }
};


/// Random deviate ~ Uniform[a,b]<br>
///  where \p a = params[0] is the lower bound,<br>
///    and \p b = params[1] is the upper bound.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Uniform_distribution_(continuous)"><b>the wiki</a>
struct uniform : public fig::Distribution
{
	const float range;
	mutable std::uniform_real_distribution< fig::CLOCK_INTERNAL_TYPE > f;
	uniform(const params_t& params)
	    : Distribution("uniform", params)
		, range(params[1] - params[0])
		, f(params[0], params[1])  // this line creates the function object
	{
		assert(0 <= params[0]);
		assert(params[0] < params[1]);
	}

	/// Don't need it for conditional sampling,
	/// which we implement with the inverse method
	inline float CDF(float x) const override
	{
		return x <= params[0] ? 0.0f :
			   x >= params[1] ? 1.0f :
			   (x-params[0]) / range;
	}

	inline return_t sample() const override { return f(*rng); }

	/// @copydoc fig::Distribution::sample_conditional
	/// @details <b>Implementation:</b><br>
	/// Inverse method for conditional probability (copyleft Pedro D'Argenio):
	/// `F_inv_elapsed(u) = F_inv( u + (1-u)*F(elapsed) ) - elapsed`
	/// where u ~ Uniform[0,1].<br>
	/// For F ~ Uniform this yields two cases:
	/// `... = u(b-elapsed)    `   if `elapsed > a`;
	/// `... = u(b-a)+a-elapsed`   if `elapsed < a`.
	inline void sample_conditional(time_t& previous, time_t& current) const override
	{
		if (static_cast<time_t>(0) >= current)
			return;  // expired Clock: do nothing
		assert(previous <= params[1]);
		assert(previous >= params[0]);
		assert(previous > current);
		const auto elapsed = previous - current;
		const auto u = Unif01(*rng);
		current = elapsed >= params[0] ? u * (params[1] - elapsed)
									   : u * range + params[0] - elapsed;
		previous = current + elapsed;
	}

	/// Alternative way: sample from Uniform[max(0,a-elapsed),b-elapsed]
	inline void sample_conditional_alt(time_t& previous, time_t& current) const
	{
		if (static_cast<time_t>(0) >= current)
			return;  // expired Clock: do nothing
		assert(previous <= params[1]);
		assert(previous >= params[0]);
		assert(previous > current);
		const auto elapsed = previous - current;
		std::uniform_real_distribution<fig::CLOCK_INTERNAL_TYPE>
				f_cond(std::max(0.0f,params[0]-elapsed), params[1]-elapsed);
		current = f_cond(*rng);
		previous = current + elapsed;
	}
};


/// Random deviate ~ Exponential(lambda)<br>
///  where \p lambda = params[0] is the rate.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Exponential_distribution"><b>the wiki</a>
struct exponential : public fig::Distribution
{
	mutable std::exponential_distribution< fig::CLOCK_INTERNAL_TYPE > f;
	exponential(const params_t& params)
	    : Distribution("exponential", params)
		, f(params[0])  // this line creates the function object
	{
		assert(0 < params[0]);
	}

	/// Don't need it for conditional sampling,
	/// which we implement by doing nothing (memoryless is the best)
	inline float CDF(float x) const override { return 1-exp(-params[0]*x); }

	inline return_t sample() const override { return f(*rng); }

	/// @copydoc fig::Distribution::sample_conditional
	/// @details <b>Implementation:</b><br>
	/// Sample from same distribution (memoryless is the best)
	inline void sample_conditional(time_t&, time_t& current) const override
	{
		if (static_cast<time_t>(0) >= current)
			return;  // expired Clock: do nothing
		current = f(*rng);
		// memoryless: no need to do "previous = current + elapsed;"
		// thus, we can't include the check "assert(previous > current);"
	}
};


/// Random deviate ~ Hyper-Exponential(p:lambda1,1-p:lambda2)<br>
///  where \p p = params[0] is the probability of choosing the first rate,<br>
///    and 1 - \p p is the probability of choosing the second rate,<br>
///    and \p lambda1 = params[1] is the first  possible rate,<br>
///    and \p lambda2 = params[2] is the second possible rate.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Hyperexponential_distribution"><b>the wiki</a>
struct hyperexponential2 : public fig::Distribution
{
	const float p;
	mutable std::exponential_distribution< fig::CLOCK_INTERNAL_TYPE > l1;
	mutable std::exponential_distribution< fig::CLOCK_INTERNAL_TYPE > l2;
	hyperexponential2(const params_t& params)
	    : Distribution("hyperexponential2", params)
	    , p(params[0])
	    , l1(params[1])  // first  exponential
	    , l2(params[2])  // second exponential
	{
		assert(1 > p);
		assert(0 < p);
		assert(0 < params[1]);
		assert(0 < params[2]);
	}
	inline float CDF(float x) const override { return p*(1-exp(-params[1]*x)) + (1-p)*(1-exp(-params[2]*x)); }
	inline return_t sample() const override { return Unif01(*rng) < p ? l1(*rng) : l2(*rng); }
};


/// Random deviate ~ Normal(m,sd)<br>
///  where \p  m = params[0] is the mean,<br>
///    and \p sd = params[1] is the standard deviation.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Normal_distribution"><b>the wiki</a>
/// @bug BUG: when taking the max with 0.000001 (to avoid sampling non-positive
///           time delays) we lose probability mass. This mass must be added to
///           the remaining (positively-supported) mass of the distribution.
/// @todo TODO Replace by the <a href="https://en.wikipedia.org/wiki/Normal_distribution"><b>
///            folded normal distribution<b></a>, which is exactly what we need.
struct normal : public fig::Distribution
{
	mutable std::normal_distribution< fig::CLOCK_INTERNAL_TYPE > f;
	normal(const params_t& params)
	    : Distribution("normal", params)
		, f(params[0], params[1])  // this line creates the function object
	{
		assert(0 < params[1]);
	}
	inline float CDF(float x) const override { return gsl_cdf_ugaussian_P((x-params[0])/params[1]); }
	inline return_t sample() const override { return std::max(0.000001f, f(*rng)); }
};


/// Random deviate ~ Lognormal(m,sd)<br>
///  where \p  m = params[0] is the mean,<br>
///    and \p sd = params[1] is the standard deviation<br>
/// of the inherent normally distributed random variable.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Log-normal_distribution"><b>the wiki</a>
struct lognormal : public fig::Distribution
{
	mutable std::lognormal_distribution< fig::CLOCK_INTERNAL_TYPE > f;
	lognormal(const params_t& params)
	    : Distribution("lognormal", params)
		, f(params[0], params[1])  // this line creates the function object
	{
		assert(0 < params[1]);
	}
	inline float CDF(float x) const override { return gsl_cdf_lognormal_P(x, params[0], params[1]); }
	inline return_t sample() const override { return f(*rng); }
};


/// Random deviate ~ Weibull(a,b)<br>
///  where \p a = params[0] is the shape parameter, aka \p k,<br>
///    and \p b = params[1] is the scale parameter, aka \p lambda.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Weibull_distribution"><b>the wiki</a>
struct weibull : public fig::Distribution
{
	const float k_inv;  // for conditional sampling
	const float l_inv;  // for conditional sampling
	mutable std::weibull_distribution< fig::CLOCK_INTERNAL_TYPE > f;
	weibull(const params_t& params)
	    : Distribution("weibull", params)
		, k_inv(1.0/params[0])
		, l_inv(1.0/params[1])
		, f(params[0], params[1])  // this line creates the function object
	{
		assert(0 < params[0]);
		assert(0 < params[1]);
	}

	/// Don't need it for conditional sampling,
	/// which we implement with the inverse method
	inline float CDF(float x) const override { return gsl_cdf_weibull_P(x, params[0], params[1]); }

	inline return_t sample() const override { return f(*rng); }

	/// @copydoc fig::Distribution::sample_conditional
	/// @details <b>Implementation:</b><br>
	/// Inverse method for conditional probability (copyleft Pedro D'Argenio):
	/// `F_inv_elapsed(u) = F_inv( u + (1-u)*F(elapsed) ) - elapsed`
	/// where u ~ Uniform[0,1].<br>
	/// For F ~ Weibull this yields: `lambda*(-ln(u*e^-((elapsed/lambda)^k)))^(1/k) - elapsed`
	inline void sample_conditional(time_t& previous, time_t& current) const override
	{
		if (static_cast<time_t>(0) >= current)
			return;  // expired Clock: do nothing
		assert(previous > current);
		const auto elapsed = previous - current;
		current = params[1] * pow(-log(Unif01(*rng)*exp(-pow(elapsed*l_inv,params[0]))), k_inv) - elapsed;
		previous = current + elapsed;
	}
};


/// Random deviate ~ Rayleigh(s) ~ Weibull(2,s*sqrt(2))<br>
///  where \p s = params[0] is the scale parameter.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Rayleigh_distribution"><b>the wiki</a>
struct rayleigh : public fig::Distribution
{
	const float s_s_2;  // for conditional sampling
	mutable std::weibull_distribution< fig::CLOCK_INTERNAL_TYPE > f;
	rayleigh(const params_t& params)
		: Distribution("rayleigh", params)
		, s_s_2(params[0]*params[0]*2.0f)
		, f(2.0, params[0]*M_SQRT2f32)  // this line creates the function object
	{
		assert(0 < params[0]);
	}

	/// Don't need it for conditional sampling,
	/// which we implement with the inverse method
	inline float CDF(float x) const override { return gsl_cdf_rayleigh_P(x, params[0]); }

	inline return_t sample() const override { return f(*rng); }

	/// @copydoc fig::Distribution::sample_conditional
	/// @details <b>Implementation:</b><br>
	/// Inverse method for conditional probability (copyleft Pedro D'Argenio):
	/// `F_inv_elapsed(u) = F_inv( u + (1-u)*F(elapsed) ) - elapsed`
	/// where u ~ Uniform[0,1].<br>
	/// For F ~ Rayleigh this yields: `sqrt(elapsed^2-2*s^2*ln(u)) - elapsed`
	inline void sample_conditional(time_t& previous, time_t& current) const override
	{
		if (static_cast<time_t>(0) >= current)
			return;  // expired Clock: do nothing
		assert(previous > current);
		const auto elapsed = previous - current;
		current = sqrtf(elapsed*elapsed - s_s_2*log(Unif01(*rng))) - elapsed;
		previous = current + elapsed;
	}
};


/// Random deviate ~ Gamma(a,b)<br>
///  where \p a = params[0] is the shape parameter,<br>
///    and \p b = params[1] is the scale parameter, aka theta, aka reciprocal of the rate.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Gamma_distribution"><b>the wiki</a>
struct Gamma : public fig::Distribution
{
	mutable std::gamma_distribution< fig::CLOCK_INTERNAL_TYPE > f;
	Gamma(const params_t& params)
	    : Distribution("gamma", params)
		, f(params[0], params[1])  // this line creates the function object
	{
		assert(0 < params[0]);
		assert(0 < params[1]);
	}
	inline float CDF(float x) const override { return gsl_cdf_gamma_P(x, params[0], params[1]); }
	inline return_t sample() const override { return f(*rng); }
};


/// Random deviate ~ Erlang(k,l) ~ Gamma(k,1/l)<br>
///  where \p k = params[0] is the <em>integral</em> shape parameter,<br>
///    and \p l = params[1] is the rate parameter, aka reciprocal of the scale.<br>
/// Check <a href="https://en.wikipedia.org/wiki/Erlang_distribution"><b>the wiki</a>
/// @note Erlang = Gamma iff the shape parameter is integral
struct erlang : public fig::Distribution
{
	const int k;
	const float l;
	mutable std::gamma_distribution< fig::CLOCK_INTERNAL_TYPE > f;
	erlang(const params_t& params)
	    : Distribution("erlang", params)
	    , k(std::round(params[0]))
		, l(1.0f/params[1])
		, f(k,l)  // this line creates the function object
	{
		assert(0 < params[0]);
		assert(0 < params[1]);
	}
	inline float CDF(float x) const override { return gsl_cdf_gamma_P(x, k, l); }
	inline return_t sample() const override { return f(*rng); }
};


/// (Non-) Random deviate ~ Dirac(x)<br>
///  where \p [x,x] is the <em>point-wise</em> support of the distribution<br>
/// Check <a href="https://en.wikipedia.org/wiki/Dirac_delta_function"><b>the wiki</a>
struct dirac : public fig::Distribution
{
	const time_t x;
	dirac(const params_t& params)
	    : Distribution("dirac", params)
	    , x(std::round(params[0]))
	{
		assert(0 < params[0]);
	}
	inline float CDF(float y) const override { return y < x ? 0.0f : 1.0f; }
	inline return_t sample() const override { return x; }
	inline void sample_conditional(time_t&, time_t&) const override { }
};


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


Clock::Clock(const std::string& clockName,
             const std::string& distName,
             const DistributionParameters& params) :
        name_(clockName),
        dist_(distName == "uniform"           ? dynamic_cast<Distribution*>(new uniform(params)) :
              distName == "exponential"       ? dynamic_cast<Distribution*>(new exponential(params)) :
              distName == "hyperexponential2" ? dynamic_cast<Distribution*>(new hyperexponential2(params)) :
              distName == "normal"            ? dynamic_cast<Distribution*>(new normal(params)) :
              distName == "lognormal"         ? dynamic_cast<Distribution*>(new lognormal(params)) :
              distName == "weibull"           ? dynamic_cast<Distribution*>(new weibull(params)) :
              distName == "rayleigh"          ? dynamic_cast<Distribution*>(new rayleigh(params)) :
              distName == "gamma"             ? dynamic_cast<Distribution*>(new Gamma(params)) :
              distName == "erlang"            ? dynamic_cast<Distribution*>(new erlang(params)) :
              distName == "dirac"             ? dynamic_cast<Distribution*>(new dirac(params)) :
              dynamic_cast<Distribution*>(new unknown_distribution(distName, params)))
{
	assert(!clockName.empty());
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
