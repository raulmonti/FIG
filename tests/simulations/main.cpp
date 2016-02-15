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
#include <ILabel.h>
#include <OLabel.h>
#include <ModelSuite.h>
#include <ModuleInstance.h>
#include <PropertyTransient.h>
#include <StoppingConditions.h>
#include <SimulationEngineNosplit.h>


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
	 *      int q1 : [0..7] = 1                                    *
	 *      clock clkArr  : Normal(9,1)                            *
	 *      clock clkPass : Uniform(0,5)                           *
	 *                                                             *
	 *      [arr!]  q1 == 0   @ clkArr  --> q1++ {clkArr, clkPass} *
	 *      [arr!] 0 < q1 < 7 @ clkArr  --> q1++ {clkArr}          *
	 *      [arr!]  q1 == 7   @ clkArr  -->      {clkArr}          *
	 *      [pass!] q1 >  1   @ clkPass --> q1-- {clkPass}         *
	 *      [pass!] q1 == 1   @ clkPass --> q1-- {}                *
	 *                                                             *
	 *  EndModule                                                  *
	 *                                                             *
	 *  Module Queue2                                              *
	 *                                                             *
	 *      int q2 : [0..5] = 0                                    *
	 *      clock clkExit : Exponential(0.18)                      *
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
	State vars1 = std::vector< VarDef >({std::make_tuple("q1", 0, 7, 1)});
	std::vector< Clock > clocks1 = {{"clkArr",  "normalMV",  {{9.0, 1.0}}},
									{"clkPass", "uniformAB", {{0.0, 5.0}}}};
	module = std::make_shared< ModuleInstance >("Queue1", vars1, clocks1);
	// [arr!] q1 == 0 @ clkArr --> q1++ {clkArr, clkPass}
	module->add_transition(
		OLabel("arr"),
		"clkArr",
		Precondition("q1 == 0", NamesList({"q1"})),
		Postcondition("1", NamesList(), NamesList({"q1"})),
		NamesList({"clkArr", "clkPass"}));
	// [arr!] 0 < q1 < 7 @ clkArr --> q1++ {clkArr}
	module->add_transition(
		OLabel("arr"),
		"clkArr",
		Precondition("0 < q1 && q1 < 7", NamesList({"q1"})),
		Postcondition("q1+1", NamesList({"q1"}), NamesList({"q1"})),
		NamesList({"clkArr"}));
	// [arr!] q1 == 7 @ clkArr --> {clkArr}
	module->add_transition(
		OLabel("arr"),
		"clkArr",
		Precondition("q1 == 7", NamesList({"q1"})),
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
	std::vector< Clock > clocks2 = {{"clkExit", "exponential", {{0.18}}}};
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
	model.seal(NamesList({"clkArr", "clkPass"}));
	std::cout << "Building the importance functions" << std::endl;
	const std::string flatIfunName("algebraic");
	const std::string autoIfunName("concrete_coupled");
	model.build_importance_function_adhoc("algebraic", *property_ptr, "q2", NamesList({"q2"}));
//	model.build_importance_function_flat(flatIfunName, *property_ptr);
	model.build_importance_function_auto(autoIfunName, *property_ptr);
	std::cout << "Building its thresholds" << std::endl;
	model.build_thresholds("ams", flatIfunName);
	model.build_thresholds("ams", autoIfunName);

	// Time simulations
	std::cout << "\nSimulating for fixed time spans\n" << std::endl;
	fig::StoppingConditions stop_by_time;
	stop_by_time.add_time_budget(2ul);
	stop_by_time.add_time_budget(7ul);
	std::cout << "...with \"flat\" ifun and \"nosplit\" engine...\n"
			  << std::endl;
	auto engine_ptr = model.prepare_simulation_engine("nosplit", flatIfunName);
	assert(engine_ptr->bound());
	model.estimate(*property_ptr, *engine_ptr, stop_by_time);
	std::cout << "...with \"auto\" ifun and \"restart\" engine...\n"
			  << std::endl;
	engine_ptr = model.prepare_simulation_engine("restart", autoIfunName);
	assert(engine_ptr->bound());
	model.estimate(*property_ptr, *engine_ptr, stop_by_time);

	// Value simulations
	std::cout << "Simulating until desired accuracy is reached\n" << std::endl;
	fig::StoppingConditions stop_by_value;
	stop_by_value.add_confidence_criterion(0.7, 0.2, true);
	stop_by_value.add_confidence_criterion(0.9, 0.1, true);
	std::cout << "...with \"flat\" ifun and \"nosplit\" engine...\n"
			  << std::endl;
	engine_ptr = model.prepare_simulation_engine("nosplit", flatIfunName);
	assert(engine_ptr->bound());
	model.estimate(*property_ptr, *engine_ptr, stop_by_value);
	std::cout << "...with \"auto\" ifun and \"restart\" engine...\n"
			  << std::endl;
	engine_ptr = model.prepare_simulation_engine("restart", autoIfunName);
	assert(engine_ptr->bound());
	model.estimate(*property_ptr, *engine_ptr, stop_by_value);

	// Cleanup
	std::cout << "\nReleasing resources" << std::endl;
	model.release_resources(flatIfunName, "nosplit");
	model.release_resources(autoIfunName, "restart");

	return 0;
}
