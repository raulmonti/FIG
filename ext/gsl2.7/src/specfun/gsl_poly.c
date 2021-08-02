/* gsl_poly.c
 *
 * Copyright (C) 1996, 1997, 1998, 1999, 2000, 2004, 2007 Brian Gough
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

#include <stdlib.h>
#include <gsl_poly.h>

//	#ifdef HAVE_INLINE
//	INLINE_FUN
double
gsl_poly_eval(const double c[], const int len, const double x)
{
  int i;
  double ans = c[len-1];
  for(i=len-1; i>0; i--) ans = c[i-1] + x * ans;
  return ans;
}

//	INLINE_FUN
gsl_complex
gsl_poly_complex_eval(const double c[], const int len, const gsl_complex z)
{
  int i;
  gsl_complex ans;
  GSL_SET_COMPLEX (&ans, c[len-1], 0.0);
  for(i=len-1; i>0; i--) {
	/* The following three lines are equivalent to
	   ans = gsl_complex_add_real (gsl_complex_mul (z, ans), c[i-1]);
	   but faster */
	double tmp = c[i-1] + GSL_REAL (z) * GSL_REAL (ans) - GSL_IMAG (z) * GSL_IMAG (ans);
	GSL_SET_IMAG (&ans, GSL_IMAG (z) * GSL_REAL (ans) + GSL_REAL (z) * GSL_IMAG (ans));
	GSL_SET_REAL (&ans, tmp);
  }
  return ans;
}

//	INLINE_FUN
gsl_complex
gsl_complex_poly_complex_eval(const gsl_complex c[], const int len, const gsl_complex z)
{
  int i;
  gsl_complex ans = c[len-1];
  for(i=len-1; i>0; i--) {
	/* The following three lines are equivalent to
	   ans = gsl_complex_add (c[i-1], gsl_complex_mul (x, ans));
	   but faster */
	double tmp = GSL_REAL (c[i-1]) + GSL_REAL (z) * GSL_REAL (ans) - GSL_IMAG (z) * GSL_IMAG (ans);
	GSL_SET_IMAG (&ans, GSL_IMAG (c[i-1]) + GSL_IMAG (z) * GSL_REAL (ans) + GSL_REAL (z) * GSL_IMAG (ans));
	GSL_SET_REAL (&ans, tmp);
  }
  return ans;
}

//	INLINE_FUN
double
gsl_poly_dd_eval(const double dd[], const double xa[], const size_t size, const double x)
{
  size_t i;
  double y = dd[size - 1];
  for (i = size - 1; i--;) y = dd[i] + (x - xa[i]) * y;
  return y;
}

//	#endif /* HAVE_INLINE */
