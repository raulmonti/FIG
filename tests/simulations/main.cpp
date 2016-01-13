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
#include <list>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <cassert>
// FIG
//#include <fig.h>  // we won't be using the parser yet
#include <ModelSuite.h>
#include <TraialPool.h>
#include <ILabel.h>
#include <OLabel.h>
#include <FigException.h>


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

	// Types names
	using fig::Clock;
	using fig::ILabel;
	using fig::OLabel;
	using fig::Precondition;
	using fig::Postcondition;
	using fig::Transition;
	using fig::ModuleInstance;
	using fig::ModelSuite;
	typedef fig::VariableDefinition<fig::STATE_INTERNAL_TYPE> VarDef;
	typedef fig::State<fig::STATE_INTERNAL_TYPE>              State;
	typedef std::list< std::string >                          NamesList;

	// Reusable variables
	ModelSuite& model = fig::ModelSuite::get_instance();
	std::shared_ptr< ModuleInstance > module(nullptr);

	// Model construction
	std::cout << "Building the first module" << std::endl;
	State vars1 = std::vector< VarDef >({std::make_tuple("q1",0,10,1)});
	std::vector< Clock > clocks1 = {{"clkArr",  "uniformAB",   {{0.0, 2.0}}},
									{"clkPass", "exponential", {{5.0}}}};
	module = std::make_shared< ModuleInstance >("Queue1", vars1, clocks1);
	module->add_transition(
		OLabel("arr"),
		"clkArr",
		Precondition("q1 < 10", NamesList({"q1"})),
		Postcondition("q1+1", NamesList({"q1"}), NamesList({"q1"})),
		NamesList({"clkArr"}));
	module->add_transition(
		OLabel("pass"),
		"clkPass",
		Precondition("q1 > 0", NamesList({"q1"})),
		Postcondition("q1-1", NamesList({"q1"}), NamesList({"q1"})),
		NamesList({"clkPass"}));
	model.add_module(module);

	throw_FigException("TODO: finish this test!");

	return 0;
}
