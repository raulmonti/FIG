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
// FIG
#include <MathExpression.h>


/**
 * Functions offered to the end user for mathematical expressions
 */
namespace
{

/// Minimum between 2 values
template< typename _T >
inline _T min2(_T x1, _T x2)
{
	return x1 < x2 ? x1 : x2;
}

/// Minimum between 3 values
template< typename _T >
inline _T min3(_T x1, _T x2, _T x3)
{
}

	/// @todo: TODO implement generic version with std::min()    <algorithm>
	///        and parameter pack expansion

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
		/*
		 *  TODO: bind all offered functions over variables
		 *        Notice MuParser already has a few: http://muparser.beltoforion.de/
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
