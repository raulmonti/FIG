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
#include <cmath>   // sqrt(), exp(), erf()
// C++
#include <limits>  // std::numeric_limits<>::quiet_NaN
// FIG
#include <ConfidenceInterval.h>
#include <FigException.h>


namespace
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

	if (-1.0 > y || 1.0 < y)  // Argument out of range
		x = std::numeric_limits<double>::quiet_NaN();

	if (1.0 == std::abs(y)) {
		// We're not in for extremes, ask Billy Joel for that
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
		x = x - (erf(x) - y) / (2.0/sqrt(M_PI) * exp(-x*x));
		x = x - (erf(x) - y) / (2.0/sqrt(M_PI) * exp(-x*x));
	}

	return x;
}

/**
 * @brief Quantile function for the standard normal distribution.
 * @details Check out the definition in the
 *          <a href="http://en.wikipedia.org/wiki/Probit#Computation">
 *          wiki</a>.
 * @return Standard normal inverse CDF of 'y', or 'NaN' on error
 */
double probit(const double& y)
{
	if (-1.0 > y || 1.0 < y)
		return std::numeric_limits<double>::quiet_NaN();
	else
		return sqrt(2.0) * erf_inv(2.0 * y - 1.0);
}

} // namespace



namespace fig
{

bool ConfidenceInterval::value_simulations_;  // defined only to copy the doc!
bool ConfidenceInterval::time_simulations_;   // defined only to copy the doc!


ConfidenceInterval::ConfidenceInterval(const std::string& thename,
									   double confidence,
									   double precision,
									   bool dynamicPrecision) :
    name(thename),
	errorMargin(precision/2.0),
	percent(dynamicPrecision),
	confidence(confidence),
	quantile(ConfidenceInterval::confidence_quantile(confidence)),
	numSamples_(0),
	estimate_(0.0),
	variance_(std::numeric_limits<double>::infinity()),
	halfWidth_(std::numeric_limits<double>::infinity()),
	statOversample_(1.0),
	varCorrection_(1.0)
{
	if (0.0 >= precision)
		throw_FigException("requires precision > 0.0");
//	if (percent && 1.0 <= precision)
//		throw_FigException("dynamic precision must ∈ (0.0, 1.0)");
	if (0.0 >= confidence || 1.0 <= confidence)
		throw_FigException("requires confidence coefficient ∈ (0.0, 1.0)");
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
ConfidenceInterval::is_valid() const noexcept
{
	return min_samples_covered() &&
		   0.0 <= estimate_ && estimate_ <= 1.0 &&
		   halfWidth_ < errorMargin * (percent ? estimate_ : 1.0);
}


double
ConfidenceInterval::precision() const noexcept
{
    return 2.0 * errorMargin * (percent ? estimate_ : 1.0);
}


double
ConfidenceInterval::lower_limit() const
{
    return std::max(0.0, estimate_ - precision()/2.0);
}


double
ConfidenceInterval::lower_limit(const double& confco) const
{
    return std::max(0.0, estimate_ - precision(confco)/2.0);
}


double
ConfidenceInterval::upper_limit() const
{
    return std::min(1.0, estimate_ + precision()/2.0);
}


double
ConfidenceInterval::upper_limit(const double& confco) const
{
    return std::min(1.0, estimate_ + precision(confco)/2.0);
}


void
ConfidenceInterval::reset() noexcept
{
    numSamples_ = 0;
    estimate_ = 0.0;
    variance_ = std::numeric_limits<double>::infinity();
    halfWidth_ = std::numeric_limits<double>::infinity();
    statOversample_ = 1.0;
    varCorrection_ = 1.0;
}


double
ConfidenceInterval::confidence_quantile(const double& cc)
{
	double quantile = probit((1.0+cc)/2.0);
	if (std::isnan(quantile) || std::isinf(quantile))
		throw_FigException("error computing confidence quantile");
	return quantile;
}

} // namespace fig
