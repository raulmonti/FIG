//==============================================================================
//
//  ConfidenceInterval.cpp
//
//  Copyleft 2016-
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
#include <cmath>   // sqrt(), exp(), erf(), M_constants...
#include <cassert>
// C++
#include <limits>   // std::numeric_limits<>::quiet_NaN
#include <ostream>
#include <iomanip>  // std:setprecision()
// External code
#include <gsl_cdf.h>  // gsl_cdf_{ugaussian,tdist}_Pinv()
#include <gsl_sys.h>  // gsl_finite(), gsl_nan()
#include <gsl_errno.h>
// FIG
#include <ConfidenceInterval.h>
#include <FigException.h>


namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

/**
 * @brief   Inverse error function
 * @details Check out the definition in the
 *          <a href="http://en.wikipedia.org/wiki/Error_function#Inverse_functions">
 *          wiki</a>. This implementation in particular was taken from
 *          <a href="http://stackoverflow.com/a/5975483">this SO answer</a>.
 * @return  Erf inverse applied to 'y', or 'NaN' on error
 */
double erf_inv(const double& y)
{
	static const double y0(0.7);
	static const double a[4] = { 0.886226899, -1.645349621,  0.914624893, -0.140543331};
	static const double b[4] = {-2.118377725,  1.442710462, -0.329097515,  0.012229801};
	static const double c[4] = {-1.970840454, -1.624906493,  3.429567803,  1.641345311};
	static const double d[2] = { 3.543889200,  1.637067800};
	double x, z;

//	NOTE: conditiouns below would make this value of x never to be read
//	if (-1.0 > y || 1.0 < y)  // Argument out of range
//		x = std::numeric_limits<double>::quiet_NaN();

	if (1.0 == std::abs(y)) {
		// We're not in for extremes (go ask Billy Joel)
		x = std::numeric_limits<double>::quiet_NaN();

	} else if (y < -y0) {
		z = sqrt(-log((1.0+y)/2.0));
		x = -(((c[3]*z+c[2])*z+c[1]) * z + c[0]) / ((d[1]*z+d[0]) * z + 1.0);

	} else {
		if (y < y0) {
			z = y*y;
			x = y*(((a[3]*z+a[2])*z+a[1])*z+a[0])/((((b[3]*z+b[3])*z+b[1])*z+b[0])*z+1.0);
		} else {
			z = sqrt(-log((1.0-y)/2.0));
			x = (((c[3]*z+c[2])*z+c[1])*z+c[0])/((d[1]*z+d[0])*z+1.0);
		}
		// Polish x to full accuracy
		x = x - (erf(x) - y) / (M_2_SQRTPI * exp(-x*x));
		x = x - (erf(x) - y) / (M_2_SQRTPI * exp(-x*x));
	}

	return x;
}

} // namespace   // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

bool ConfidenceInterval::value_simulations_;  // defined only to copy the doc!
bool ConfidenceInterval::time_simulations_;   // defined only to copy the doc!


ConfidenceInterval::ConfidenceInterval(const std::string& thename,
									   double confidence,
									   double precision,
									   bool dynamicPrecision,
									   bool neverStop) :
    name(thename),
	errorMargin(precision/2.0),
	percent(dynamicPrecision),
	confidence(confidence),
	alwaysInvalid(neverStop),
	numSamples_(0),
	estimate_(0.0),
	prevEstimate_(0.0),
	variance_(std::numeric_limits<double>::infinity()),
	quantile(confidence_quantile(confidence)),
    halfWidth_(std::numeric_limits<double>::infinity()),
    statOversample_(1.0),
    varCorrection_(1.0)
{
	assert(gsl_finite(quantile));
	assert(std::isfinite(quantile));
	if (0.0 >= precision)
		throw_FigException("a CI requires precision > 0.0");
//	if (percent && 1.0 <= precision)
//		throw_FigException("dynamic precision must ∈ (0.0, 1.0)");
	if (0.0 >= confidence || 1.0 <= confidence)
		throw_FigException("a CI requires confidence coefficient ∈ (0.0, 1.0)");
	// Turn off error messages from GSL
	gsl_set_error_handler_off();
}


void
ConfidenceInterval::set_statistical_oversampling(const double& statOversamp)
{
	if (1.0 > statOversamp)
		throw_FigException("the statistical oversampling factor should scale "
		                   "*up* the minimum # of experimental samples needed");
	statOversample_ = statOversamp;
}


void
ConfidenceInterval::set_variance_correction(const double& varCorrection)
{
	if (0.0 > varCorrection)
		throw_FigException("the variance correction factor must be positive");
	varCorrection_ = varCorrection;
}


bool
ConfidenceInterval::is_valid(bool safeguard) const noexcept
{
	// NOTE: variable definition forced due to issue #17
	//       [git.cs.famaf.unc.edu.ar/dsg/fig/issues/17]
	const bool finished =
		   !alwaysInvalid && 0.0 < estimate_ && estimate_ < 1.0 &&
		   min_samples_covered(safeguard) &&
		   halfWidth_ < errorMargin * (percent ? estimate_ : 1.0);
		   // the interval's "sample" half width is compared against
		   // the "theoretical" error margin
	return finished;
}


double
ConfidenceInterval::precision() const noexcept
{
	return 2.0 * errorMargin * (percent ? estimate_ : 1.0);
}


double
ConfidenceInterval::lower_limit() const
{
	return std::min(1.0, std::max(0.0, estimate_ - precision()/2.0));
}


double
ConfidenceInterval::lower_limit(const double& confco) const
{
	return std::min(1.0, std::max(0.0, estimate_ - precision(confco)/2.0));
}


double
ConfidenceInterval::upper_limit() const
{
	return std::max(0.0, std::min(1.0, estimate_ + precision()/2.0));
}


double
ConfidenceInterval::upper_limit(const double& confco) const
{
	return std::max(0.0, std::min(1.0, estimate_ + precision(confco)/2.0));
}


void
ConfidenceInterval::reset(bool fullReset) noexcept
{
    numSamples_ = 0;
    estimate_ = 0.0;
	prevEstimate_ = 0.0;
	variance_ = std::numeric_limits<decltype(variance_)>::infinity();
	halfWidth_ = std::numeric_limits<decltype(halfWidth_)>::infinity();
	if (fullReset) {
		statOversample_ = 1.0;
		varCorrection_ = 1.0;
	}
}


void ConfidenceInterval::print(std::ostream& out,
                               bool printScientific,
                               int printPrecision)
{
	if (printScientific)
		out << std::setprecision(3) << std::scientific;
	else
		out << std::setprecision(printPrecision);
	out << "Estimate: " << point_estimate()
	    << " (var="  << estimation_variance()
	    << ",prec=" << precision(confidence) << ")";
#if __cplusplus >= 201103L
	out << std::defaultfloat;
#else
	out << std::setprecision(6) << std::fixed;
#endif
}


double
ConfidenceInterval::confidence_quantile(const double& cc,
										const unsigned long& N)
{
#ifndef NDEBUG
	if (0.0 >= cc || 1.0 <= cc)
		throw_FigException("requires confidence coefficient ∈ (0.0, 1.0)");
#endif
	double significance(0.5*(1.0+cc));  // == 1-(1-cc)/2
	double quantile;
	// Following usually runs once, but Murphy has showed his face around here
	do {
//		double quantile = gsl_cdf_ugaussian_Pinv(significance);  // easy way
		quantile = gsl_cdf_tdist_Pinv(significance,              // correct way
									  std::max(N-1.0, 10.0));

		// ^^^ for the ctor of the CI, before estimations begin,
		//     we emulate 10 degrees of freedom which symbolise 11 samples
		significance += 0.000001;
	} while (!gsl_finite(quantile)    &&
			 !std::isfinite(quantile) &&
			  significance < 0.50001*(1.0+cc));

	if ( ! (std::isfinite(quantile) && gsl_finite(quantile)) ) {
		// GSL T-distribution failed, try using Gaussian
		quantile = gsl_cdf_ugaussian_Pinv(0.5*(1.0+cc));
		if ( ! (std::isfinite(quantile) && gsl_finite(quantile)) )
			throw_FigException("error computing confidence quantile");
	}
	return quantile;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
