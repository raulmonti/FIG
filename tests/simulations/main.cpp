//==============================================================================
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

// C/C++
#include <vector>
#include <string>
#include <cassert>
// FIG
//#include <fig.h>  // we won't be using the parser yet
#include <FigException.h>
#include <ModelSuite.h>
#include <TraialPool.h>

using namespace fig;


int main()
{
	/* * * * * * * * * * * * * * * * * * * * * * * * * * *
	 *                                                   *
	 *  System to test: tandem queue                     *
	 *                                                   *
	 *  'arr'  tells a new package arrives at q1         *
	 *  'pass' tells a package passes from q1 to q2      *
	 *  'exit' tells a package exits q2                  *
	 *  Initial clocks: {clkArr,clkPass} in Queue1       *
	 *                                                   *
	 *  Module Queue1                                    *
	 *                                                   *
	 *      int q1 : [0..10] = 1                         *
	 *      clock clkArr  : Uniform(0,2)                 *
	 *      clock clkPass : Exponential(5)               *
	 *                                                   *
	 *      [arr!]  q1 < 10 @ clkArr  --> q1++ {clkArr}  *
	 *      [pass!] q1 > 0  @ clkPass --> q1-- {clkPass} *
	 *                                                   *
	 *  EndModule                                        *
	 *                                                   *
	 *  Module Queue2                                    *
	 *                                                   *
	 *      int q2 : [0..8] = 0                          *
	 *      clock clkExit : Normal(4,1)                  *
	 *                                                   *
	 *      [pass?] q2 < 8           --> q2++ {}         *
	 *      [pass?] q2 == 8          --> q2=8 {}         *
	 *      [exit!] q2 > 0 @ clkExit --> q2-- {clkExit}  *
	 *                                                   *
	 *  EndModule                                        *
	 *                                                   *
	 *  Prob( q1+q2 > 0 U q2 == 8 ) ?                    *
	 *                                                   *
	 * * * * * * * * * * * * * * * * * * * * * * * * * * */

	throw_FigException("TODO: this test!");

	return 0;
}
