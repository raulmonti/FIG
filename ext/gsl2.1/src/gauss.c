/* cdf/gauss.c
 *
 * Copyright (C) 2002, 2004 Jason H. Stover.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
 * Computes the cumulative distribution function for the Gaussian
 * distribution using a rational function approximation.  The
 * computation is for the standard Normal distribution, i.e., mean 0
 * and standard deviation 1. If you want to compute Pr(X < t) for a
 * Gaussian random variable X with non-zero mean m and standard
 * deviation sd not equal to 1, find gsl_cdf_ugaussian ((t-m)/sd).
 * This approximation is accurate to at least double precision. The
 * accuracy was verified with a pari-gp script.  The largest error
 * found was about 1.4E-20. The coefficients were derived by Cody.
 *
 * References:
 *
 * W.J. Cody. "Rational Chebyshev Approximations for the Error
 * Function," Mathematics of Computation, v23 n107 1969, 631-637.
 *
 * W. Fraser, J.F Hart. "On the Computation of Rational Approximations
 * to Continuous Functions," Communications of the ACM, v5 1962.
 *
 * W.J. Kennedy Jr., J.E. Gentle. "Statistical Computing." Marcel Dekker. 1980.
 * 
 *  
 */

#include <gsl_config.h>
#include <math.h>
#include <gsl_math.h>
#include <gsl_cdf.h>

#ifndef M_1_SQRT2PI
#define M_1_SQRT2PI (M_2_SQRTPI * M_SQRT1_2 / 2.0)
#endif

#define SQRT32 (4.0 * M_SQRT2)

/*
 * IEEE double precision dependent constants.
 *
 * GAUSS_EPSILON: Smallest positive value such that 
 *                gsl_cdf_gaussian(x) > 0.5.
 * GAUSS_XUPPER: Largest value x such that gsl_cdf_gaussian(x) < 1.0.
 * GAUSS_XLOWER: Smallest value x such that gsl_cdf_gaussian(x) > 0.0.
 */

#define GAUSS_EPSILON  (GSL_DBL_EPSILON / 2)
#define GAUSS_XUPPER (8.572)
#define GAUSS_XLOWER (-37.519)

#define GAUSS_SCALE (16.0)

static double
get_del (double x, double rational)
{
  double xsq = 0.0;
  double del = 0.0;
  double result = 0.0;

  xsq = floor (x * GAUSS_SCALE) / GAUSS_SCALE;
  del = (x - xsq) * (x + xsq);
  del *= 0.5;

  result = exp (-0.5 * xsq * xsq) * exp (-1.0 * del) * rational;

  return result;
}

/*
 * Normal cdf for fabs(x) < 0.66291
 */
static double
gauss_small (const double x)
{
  unsigned int i;
  double result = 0.0;
  double xsq;
  double xnum;
  double xden;

  const double a[5] = {
    2.2352520354606839287,
    161.02823106855587881,
    1067.6894854603709582,
    18154.981253343561249,
    0.065682337918207449113
  };
  const double b[4] = {
    47.20258190468824187,
    976.09855173777669322,
    10260.932208618978205,
    45507.789335026729956
  };

  xsq = x * x;
  xnum = a[4] * xsq;
  xden = xsq;

  for (i = 0; i < 3; i++)
    {
      xnum = (xnum + a[i]) * xsq;
      xden = (xden + b[i]) * xsq;
    }

  result = x * (xnum + a[3]) / (xden + b[3]);

  return result;
}

/*
 * Normal cdf for 0.66291 < fabs(x) < sqrt(32).
 */
static double
gauss_medium (const double x)
{
  unsigned int i;
  double temp = 0.0;
  double result = 0.0;
  double xnum;
  double xden;
  double absx;

  const double c[9] = {
    0.39894151208813466764,
    8.8831497943883759412,
    93.506656132177855979,
    597.27027639480026226,
    2494.5375852903726711,
    6848.1904505362823326,
    11602.651437647350124,
    9842.7148383839780218,
    1.0765576773720192317e-8
  };
  const double d[8] = {
    22.266688044328115691,
    235.38790178262499861,
    1519.377599407554805,
    6485.558298266760755,
    18615.571640885098091,
    34900.952721145977266,
    38912.003286093271411,
    19685.429676859990727
  };

  absx = fabs (x);

  xnum = c[8] * absx;
  xden = absx;

  for (i = 0; i < 7; i++)
    {
      xnum = (xnum + c[i]) * absx;
      xden = (xden + d[i]) * absx;
    }

  temp = (xnum + c[7]) / (xden + d[7]);

  result = get_del (x, temp);

  return result;
}

/*
 * Normal cdf for 
 * {sqrt(32) < x < GAUSS_XUPPER} union { GAUSS_XLOWER < x < -sqrt(32) }.
 */
static double
gauss_large (const double x)
{
  int i;
  double result;
  double xsq;
  double temp;
  double xnum;
  double xden;
  double absx;

  const double p[6] = {
    0.21589853405795699,
    0.1274011611602473639,
    0.022235277870649807,
    0.001421619193227893466,
    2.9112874951168792e-5,
    0.02307344176494017303
  };
  const double q[5] = {
    1.28426009614491121,
    0.468238212480865118,
    0.0659881378689285515,
    0.00378239633202758244,
    7.29751555083966205e-5
  };

  absx = fabs (x);
  xsq = 1.0 / (x * x);
  xnum = p[5] * xsq;
  xden = xsq;

  for (i = 0; i < 4; i++)
    {
      xnum = (xnum + p[i]) * xsq;
      xden = (xden + q[i]) * xsq;
    }

  temp = xsq * (xnum + p[4]) / (xden + q[4]);
  temp = (M_1_SQRT2PI - temp) / absx;

  result = get_del (x, temp);

  return result;
}

double
gsl_cdf_ugaussian_P (const double x)
{
  double result;
  double absx = fabs (x);

  if (absx < GAUSS_EPSILON)
    {
      result = 0.5;
      return result;
    }
  else if (absx < 0.66291)
    {
      result = 0.5 + gauss_small (x);
      return result;
    }
  else if (absx < SQRT32)
    {
      result = gauss_medium (x);

      if (x > 0.0)
        {
          result = 1.0 - result;
        }

      return result;
    }
  else if (x > GAUSS_XUPPER)
    {
      result = 1.0;
      return result;
    }
  else if (x < GAUSS_XLOWER)
    {
      result = 0.0;
      return result;
    }
  else
    {
      result = gauss_large (x);

      if (x > 0.0)
        {
          result = 1.0 - result;
        }
    }

  return result;
}

double
gsl_cdf_ugaussian_Q (const double x)
{
  double result;
  double absx = fabs (x);

  if (absx < GAUSS_EPSILON)
    {
      result = 0.5;
      return result;
    }
  else if (absx < 0.66291)
    {
      result = gauss_small (x);

      if (x < 0.0)
        {
          result = fabs (result) + 0.5;
        }
      else
        {
          result = 0.5 - result;
        }

      return result;
    }
  else if (absx < SQRT32)
    {
      result = gauss_medium (x);

      if (x < 0.0)
        {
          result = 1.0 - result;
        }

      return result;
    }
  else if (x > -(GAUSS_XLOWER))
    {
      result = 0.0;
      return result;
    }
  else if (x < -(GAUSS_XUPPER))
    {
      result = 1.0;
      return result;
    }
  else
    {
      result = gauss_large (x);

      if (x < 0.0)
        {
          result = 1.0 - result;
        }

    }

  return result;
}

double
gsl_cdf_gaussian_P (const double x, const double sigma)
{
  return gsl_cdf_ugaussian_P (x / sigma);
}

double
gsl_cdf_gaussian_Q (const double x, const double sigma)
{
  return gsl_cdf_ugaussian_Q (x / sigma);
}



/* randist/gauss.c
 * 
 * Copyright (C) 1996, 1997, 1998, 1999, 2000, 2006, 2007 James Theiler, Brian Gough
 * Copyright (C) 2006 Charles Karney
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <gsl_config.h>
#include <math.h>
#include <gsl_math.h>
#include <gsl_rng.h>
#include <gsl_randist.h>

/* Of the two methods provided below, I think the Polar method is more
 * efficient, but only when you are actually producing two random
 * deviates.  We don't produce two, because then we'd have to save one
 * in a static variable for the next call, and that would screws up
 * re-entrant or threaded code, so we only produce one.  This makes
 * the Ratio method suddenly more appealing.
 *
 * [Added by Charles Karney] We use Leva's implementation of the Ratio
 * method which avoids calling log() nearly all the time and makes the
 * Ratio method faster than the Polar method (when it produces just one
 * result per call).  Timing per call (gcc -O2 on 866MHz Pentium,
 * average over 10^8 calls)
 *
 *   Polar method: 660 ns
 *   Ratio method: 368 ns
 *
 */

/* Polar (Box-Mueller) method; See Knuth v2, 3rd ed, p122 */

double
gsl_ran_gaussian (const gsl_rng * r, const double sigma)
{
  double x, y, r2;

  do
    {
      /* choose x,y in uniform square (-1,-1) to (+1,+1) */
      x = -1 + 2 * gsl_rng_uniform_pos (r);
      y = -1 + 2 * gsl_rng_uniform_pos (r);

      /* see if it is in the unit circle */
      r2 = x * x + y * y;
    }
  while (r2 > 1.0 || r2 == 0);

  /* Box-Muller transform */
  return sigma * y * sqrt (-2.0 * log (r2) / r2);
}

/* Ratio method (Kinderman-Monahan); see Knuth v2, 3rd ed, p130.
 * K+M, ACM Trans Math Software 3 (1977) 257-260.
 *
 * [Added by Charles Karney] This is an implementation of Leva's
 * modifications to the original K+M method; see:
 * J. L. Leva, ACM Trans Math Software 18 (1992) 449-453 and 454-455. */

double
gsl_ran_gaussian_ratio_method (const gsl_rng * r, const double sigma)
{
  double u, v, x, y, Q;
  const double s = 0.449871;    /* Constants from Leva */
  const double t = -0.386595;
  const double a = 0.19600;
  const double b = 0.25472;
  const double r1 = 0.27597;
  const double r2 = 0.27846;

  do                            /* This loop is executed 1.369 times on average  */
    {
      /* Generate a point P = (u, v) uniform in a rectangle enclosing
         the K+M region v^2 <= - 4 u^2 log(u). */

      /* u in (0, 1] to avoid singularity at u = 0 */
      u = 1 - gsl_rng_uniform (r);

      /* v is in the asymmetric interval [-0.5, 0.5).  However v = -0.5
         is rejected in the last part of the while clause.  The
         resulting normal deviate is strictly symmetric about 0
         (provided that v is symmetric once v = -0.5 is excluded). */
      v = gsl_rng_uniform (r) - 0.5;

      /* Constant 1.7156 > sqrt(8/e) (for accuracy); but not by too
         much (for efficiency). */
      v *= 1.7156;

      /* Compute Leva's quadratic form Q */
      x = u - s;
      y = fabs (v) - t;
      Q = x * x + y * (a * y - b * x);

      /* Accept P if Q < r1 (Leva) */
      /* Reject P if Q > r2 (Leva) */
      /* Accept if v^2 <= -4 u^2 log(u) (K+M) */
      /* This final test is executed 0.012 times on average. */
    }
  while (Q >= r1 && (Q > r2 || v * v > -4 * u * u * log (u)));

  return sigma * (v / u);       /* Return slope */
}

double
gsl_ran_gaussian_pdf (const double x, const double sigma)
{
  double u = x / fabs (sigma);
  double p = (1 / (sqrt (2 * M_PI) * fabs (sigma))) * exp (-u * u / 2);
  return p;
}

double
gsl_ran_ugaussian (const gsl_rng * r)
{
  return gsl_ran_gaussian (r, 1.0);
}

double
gsl_ran_ugaussian_ratio_method (const gsl_rng * r)
{
  return gsl_ran_gaussian_ratio_method (r, 1.0);
}

double
gsl_ran_ugaussian_pdf (const double x)
{
  return gsl_ran_gaussian_pdf (x, 1.0);
}

