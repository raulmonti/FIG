//==============================================================================
//
//  MathExpression.cpp
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


// C++
#include <iostream>
#include <algorithm>  // std::min<>(), std::max<>()
#include <initializer_list>
// C
#include <cstdarg>  // va_start, va_arg
// FIG
#include <MathExpression.h>


/**
 * Functions offered to the end user for mathematical expressions
 */
namespace
{

/// @todo: TODO extend MuParser library to accept functions with either
///        initializer lists as min<> and max<> below (try that first),
///        or std::vector<>, or else with variadic arguments.

/// Minimum between N values
template< typename _T >
inline _T min(std::initializer_list<_T> parameters)
{
	return std::min<_T>(parameters);
}

/// Maximum between N values
template< typename _T >
inline _T max(std::initializer_list<_T> parameters)
{
	return std::max<_T>(parameters);
}

/// Minimum between 2 values
template< typename _T >
inline _T min2(_T a, _T b)
{
	return std::min<_T>(a, b);
}

/// Maximum between 2 values
template< typename _T >
inline _T max2(_T a, _T b)
{
	return std::max<_T>(a, b);
}

} // namespace


namespace fig
{

using std::cerr;
using std::endl;

void MathExpression::parse_our_expression()
{
	assert(!exprStr_.empty());
	try {
		expr_.SetExpr(exprStr_);

		expr_.DefineFun("min", min2<STATE_INTERNAL_TYPE>);
		expr_.DefineFun("max", max2<STATE_INTERNAL_TYPE>);
		/*
		 *  TODO: bind all offered functions over variables
		 *        Notice MuParser already has a few: http://muparser.beltoforion.de/
		 *        But some are only available for floating point internal types
		expr.DefineFun("MySqr", MySqr);
		expr.DefineFun("Uni01", Uni01);
		...
		*/
	} catch (mu::Parser::exception_type &e) {
		cerr << "Failed parsing expression" << endl;
		cerr << "    message:  " << e.GetMsg()   << endl;
		cerr << "    formula:  " << e.GetExpr()  << endl;
		cerr << "    token:    " << e.GetToken() << endl;
		cerr << "    position: " << e.GetPos()   << endl;
		cerr << "    errc:     " << e.GetCode()  << endl;
		throw FigException("bad mathematical expression");
	}
}

} // namespace fig
