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
#include <type_traits>
// FIG
//#include <fig.h>  // we won't be using the parser yet
#include <ILabel.h>
#include <OLabel.h>
#include <ModelSuite.h>
#include <ModuleInstance.h>
#include <PropertyTransient.h>
#include <StoppingConditions.h>
#include <SimulationEngineNosplit.h>
#include <ImportanceFunctionConcreteCoupled.h>


int main()
{
	// Types names
	using fig::Clock;
	using fig::ILabel;
	using fig::OLabel;
	using fig::Precondition;
	using fig::Postcondition;
	using fig::ModuleInstance;

	typedef fig::VariableDefinition<fig::STATE_INTERNAL_TYPE> VarDef;
	typedef fig::State<fig::STATE_INTERNAL_TYPE>              State;
	typedef std::list< std::string >                          NamesList;

	// Reusable variables
	fig::ModelSuite& model = fig::ModelSuite::get_instance();
	std::shared_ptr< ModuleInstance > module(nullptr);

	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	 *                                                             *
	 *  System to test: tandem queue                               *
	 *                                                             *
	 *  'arr'  tells a new package arrives at q1                   *
	 *  'pass' tells a package passes from q1 to q2                *
	 *  'exit' tells a package exits q2                            *
	 *                                                             *
	 *  Module Queue1                                              *
	 *                                                             *
	 *      int q1 : [0..9] = 1                                    *
	 *      clock clkArr  : Uniform(0,4)                           *
	 *      clock clkPass : Normal(2,1)                            *
	 *                                                             *
	 *      [arr!]  q1 == 0   @ clkArr  --> q1++ {clkArr, clkPass} *
	 *      [arr!] 0 < q1 < 9 @ clkArr  --> q1++ {clkArr}          *
	 *      [arr!]  q1 == 9   @ clkArr  -->      {clkArr}          *
	 *      [pass!] q1 >  1   @ clkPass --> q1-- {clkPass}         *
	 *      [pass!] q1 == 1   @ clkPass --> q1-- {}                *
	 *                                                             *
	 *  EndModule                                                  *
	 *                                                             *
	 *  Module Queue2                                              *
	 *                                                             *
	 *      int q2 : [0..5] = 0                                    *
	 *      clock clkExit : Exponential(3)                         *
	 *                                                             *
	 *      [pass?] q2 == 0           --> q2++ {clkExit}           *
	 *      [pass?] 0 < q2 < 5        --> q2++ {}                  *
	 *      [pass?] q2 == 5           -->      {}  KABOOOM!        *
	 *      [exit!] q2 >  1 @ clkExit --> q2-- {clkExit}           *
	 *      [exit!] q2 == 1 @ clkExit --> q2-- {}                  *
	 *                                                             *
	 *  EndModule                                                  *
	 *                                                             *
	 *  Initial clocks: {clkArr,clkPass} in Queue1                 *
	 *  Prob( q1+q2 > 0 U q2 == 5 ) ?                              *
	 *                                                             *
	 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

	// Module "Queue1"
	std::cout << "Building the first module" << std::endl;
	State vars1 = std::vector< VarDef >({std::make_tuple("q1", 0, 9, 1)});
	std::vector< Clock > clocks1 = {{"clkArr",  "uniformAB", {{0.0, 4.0}}},
									{"clkPass", "normalMV",  {{2.0, 1.0}}}};
	module = std::make_shared< ModuleInstance >("Queue1", vars1, clocks1);
	// [arr!] q1 == 0 @ clkArr --> q1++ {clkArr, clkPass}
	module->add_transition(
		OLabel("arr"),
		"clkArr",
		Precondition("q1 == 0", NamesList({"q1"})),
		Postcondition("1", NamesList(), NamesList({"q1"})),
		NamesList({"clkArr", "clkPass"}));
	// [arr!] 0 < q1 < 9 @ clkArr --> q1++ {clkArr}
	module->add_transition(
		OLabel("arr"),
		"clkArr",
		Precondition("q1 < 9", NamesList({"q1"})),
		Postcondition("q1+1", NamesList({"q1"}), NamesList({"q1"})),
		NamesList({"clkArr"}));
	// [arr!] q1 == 9 @ clkArr --> {clkArr}
	module->add_transition(
		OLabel("arr"),
		"clkArr",
		Precondition("q1 == 9", NamesList({"q1"})),
		Postcondition("", NamesList(), NamesList()),
		NamesList({"clkArr"}));
	// [pass!] q1 > 1 @ clkPass --> q1-- {clkPass}
	module->add_transition(
		OLabel("pass"),
		"clkPass",
		Precondition("q1 > 1", NamesList({"q1"})),
		Postcondition("q1-1", NamesList({"q1"}), NamesList({"q1"})),
		NamesList({"clkPass"}));
	// [pass!] q1 == 1 @ clkPass --> q1=0 {}
	module->add_transition(
		OLabel("pass"),
		"clkPass",
		Precondition("q1 == 1", NamesList({"q1"})),
		Postcondition("0", NamesList(), NamesList({"q1"})),
		NamesList());
	model.add_module(module);
	assert(nullptr == module);

	// Module "Queue2"
	std::cout << "Building the second module" << std::endl;
	State vars2 = std::vector< VarDef >({std::make_tuple("q2", 0, 5, 0)});
	std::vector< Clock > clocks2 = {{"clkExit", "exponential", {{3.0}}}};
	module = std::make_shared< ModuleInstance >("Queue2", vars2, clocks2);
	// [pass?] q2 == 0 --> q2++ {clkExit}
	module->add_transition(
		ILabel("pass"),
		"",
		Precondition("q2 == 0", NamesList({"q2"})),
		Postcondition("1", NamesList(), NamesList({"q2"})),
		NamesList({"clkExit"}));
	// [pass?] 0 < q2 < 5 --> q2++ {}
	module->add_transition(
		ILabel("pass"),
		"",
		Precondition("0 < q2 && q2 < 5", NamesList({"q2"})),
		Postcondition("q2+1", NamesList({"q2"}), NamesList({"q2"})),
		NamesList());
	// [pass?] q2 == 5 --> {}  "rare event"
	module->add_transition(
		ILabel("pass"),
		"",
		Precondition("q2 == 5", NamesList({"q2"})),
		Postcondition("", NamesList(), NamesList()),
		NamesList());
	// [exit!] q2 > 1 @ clkExit --> q2-- {clkExit}
	module->add_transition(
		OLabel("exit"),
		"clkExit",
		Precondition("q2 > 1", NamesList({"q2"})),
		Postcondition("q2-1", NamesList({"q2"}), NamesList({"q2"})),
		NamesList({"clkExit"}));
	// [exit!] q2 == 1 @ clkExit --> q2-- {}
	module->add_transition(
		OLabel("exit"),
		"clkExit",
		Precondition("q2 == 1", NamesList({"q2"})),
		Postcondition("0", NamesList(), NamesList({"q2"})),
		NamesList({"clkExit"}));
	model.add_module(module);
	assert(nullptr == module);

	// Property
	std::cout << "Building the property" << std::endl;
	auto property_ptr(std::make_shared< fig::PropertyTransient >(
			"q1+q2 == 0", NamesList({"q1","q2"}),  // stopping condition
			"q2 == 5", NamesList({"q2"})  // goal
	));
	model.add_property(property_ptr);
	assert(nullptr != property_ptr);

	// Simulation preliminaries
	model.seal(NamesList({"clkArr", "clkPass"}));  // initial clocks' names
	std::cout << "Building an importance function" << std::endl;
	auto ifun_ptr = std::make_shared< fig::ImportanceFunctionConcreteCoupled >();
	assert(nullptr != ifun_ptr);
	ifun_ptr->assess_importance(*model.modules_network(), *property_ptr, "flat");
	std::cout << "Building a simulation engine" << std::endl;
	auto engine_ptr = std::make_shared< fig::SimulationEngineNosplit >(
						  model.modules_network());
	assert(nullptr != engine_ptr);
	engine_ptr->bind(ifun_ptr);
	std::cout << "Building simulation bounds" << std::endl;
	fig::StoppingConditions stop_by_value;
	stop_by_value.add_confidence_criterion(0.8, 0.2, true);
	assert(stop_by_value.is_confidence_criteria());
	fig::StoppingConditions stop_by_time;
	stop_by_time.add_time_budget(30ul);
	assert(stop_by_time.is_time_budgets());
	
	// Simulation
	std::cout << "Single, warm up simulation... "; std::cout.flush();
	double estimate = engine_ptr->simulate(*property_ptr, 1u);
	std::cout << "resulted in the estimate " << estimate << std::endl;
	std::cout << "Second single simulation... "; std::cout.flush();
	estimate = engine_ptr->simulate(*property_ptr, 1u);
	std::cout << "resulted in the estimate " << estimate << std::endl;
	std::cout << "Simulating until desired accuracy is reached" << std::endl;
	model.estimate(*property_ptr, *engine_ptr, stop_by_value);
	std::cout << "Simulating for certain fixed amount of time" << std::endl;
	model.estimate(*property_ptr, *engine_ptr, stop_by_time);

	// Cleanup
	engine_ptr->unbind();
	ifun_ptr->clear();

	return 0;
}
