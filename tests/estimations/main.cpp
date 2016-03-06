//==============================================================================
//
//  main.cpp
//
//  Copyleft 2016-
//  Authors:
//  - Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
//  - Raul E. Monti <raulmonti88@gmail.com> (Universidad Nacional de Córdoba)
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


#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <tuple>
#include <set>
#include <cassert>

#include <fig.h>

typedef std::set< std::string > NamesList;
typedef std::set< std::tuple<double,double,bool> > StopCond;

static void print_test_intro(std::ostream& out);
static void check_dummy_arguments(const int& argc, const char** argv);
static void build_model(const char* modelFilePath, const char* propsFilePath);



int main(int argc, char** argv)
{
	//  Intro  // // // // // // // // // // // // // // // // // //
	print_test_intro(std::cout);
	check_dummy_arguments(argc, const_cast<const char**>(argv));

	//  Compile model and properties   // // // // // // // // // //
	build_model("models/atm_queue.sa", "models/atm_queue.pp");
//	build_model("models/tandem_queue.sa", "models/tandem_queue.pp");
	auto model = fig::ModelSuite::get_instance();
	if (!model.sealed()) {
		std::cerr << "ERROR: failed to build the model.\n";
		exit(EXIT_FAILURE);
	} else {
		std::cout << std::endl;
	}
	const size_t propertyIndex(0ul);

	//  Estimation goals   // // // // // // // // // // // // // //
	const double confidence(0.95);
	const double precision(0.4);
	const fig::StoppingConditions stopCriterion(StopCond({std::make_tuple(
			confidence, precision, true)}));
	const fig::StoppingConditions timeSpan(std::set<size_t>({60ul}));
	std::shared_ptr< fig::SimulationEngine > engine(nullptr);

//	//  Standard Monte Carlo     // // // // // // // // // // // //
//	const std::string flatIfunName("algebraic");
//	model.build_importance_function_flat(flatIfunName, propertyIndex);
//	model.build_thresholds("ams", flatIfunName);
//	engine = model.prepare_simulation_engine("nosplit", flatIfunName);
//	model.estimate(propertyIndex, *engine, timeSpan);
//	//model.estimate(propertyIndex, *engine, stopCriterion);
//	engine = nullptr;

	//  RESTART with algebraic ad hoc (q2) // // // // // // // //
	const std::string adhocIfunName("algebraic");
//	model.build_importance_function_adhoc(adhocIfunName, propertyIndex, "q2", NamesList({"q2"}), true);
	model.build_importance_function_adhoc(adhocIfunName, propertyIndex, "buf", NamesList({"buf"}), true);
	model.build_thresholds("ams", adhocIfunName);
	engine = model.prepare_simulation_engine("restart", adhocIfunName);
	//model.estimate(propertyIndex, *engine, timeSpan);
	model.estimate(propertyIndex, *engine, stopCriterion);
	engine = nullptr;

//	//  RESTART with automatic coupled   // // // // // // // // //
//	const std::string cAutoIfunName("concrete_coupled");
//	model.build_importance_function_auto(cAutoIfunName, propertyIndex);
//	model.build_thresholds("ams", cAutoIfunName);
//	engine = model.prepare_simulation_engine("restart", cAutoIfunName);
//	//model.estimate(propertyIndex, *engine, timeSpan);
//	model.estimate(propertyIndex, *engine, stopCriterion);
//	engine = nullptr;

	//  RESTART with automatic split  // // // // // // // // // //
	const std::string sAutoIfunName("concrete_split");
	model.build_importance_function_auto(sAutoIfunName, propertyIndex, "+", true);
	model.build_thresholds("ams", sAutoIfunName);
	engine = model.prepare_simulation_engine("restart", sAutoIfunName);
	std::dynamic_pointer_cast<fig::SimulationEngineRestart>(engine)->
			set_splits_per_threshold(5);
	model.estimate(propertyIndex, *engine, timeSpan);
	model.estimate(propertyIndex, *engine, stopCriterion);
	engine = nullptr;

	//  Free memory  // // // // // // // // // // // // // // // //
	model.release_resources();

	return EXIT_SUCCESS;
}


// ///////////////////////////////////////////////////////////////////////////
void print_test_intro(std::ostream& out)
{
	out << std::endl;
	out << " ~~~~~~~~~ \n";
	out << "  · FIG ·  \n";
	out << " ~~~~~~~~~ \n";
	out << "           \n";
	out << " This is the Finite Improbability Generator.\n";
	out << " Version: " << fig_VERSION_MAJOR << "." << fig_VERSION_MINOR << "\n";
	out << " Authors: Budde, Carlos E. <cbudde@famaf.unc.edu.ar>\n";
	out << "          Monti, Raúl E.   <raulmonti88@gmail.com>\n";
	out << "           \n";
	out << " This is a test deviced for estimations checking;\n";
	out << " it automatically runs with the files models/tandem_queue.{sa,pp}\n";
	out << std::endl;
}


// ///////////////////////////////////////////////////////////////////////////
void check_dummy_arguments(const int& argc, const char** argv)
{
	const std::string help("--help");
	const std::string usage(std::string("Usage: ").append(argv[0])
							.append(" <modelFilePath> <propertiesFilePath>\n"));
	if (argc < 3 && argc > 1 && help == argv[1]) {
		std::cerr << usage << std::endl;
		exit(EXIT_SUCCESS);
//	} else if (argc < 3) {
//		std::cerr << "ERROR: FIG invoked with too few parameters.\n";
//		std::cerr << usage << std::endl;
//		exit(EXIT_FAILURE);
	}
}


// ///////////////////////////////////////////////////////////////////////////
void build_model(const char* modelFilePath, const char* propsFilePath)
{

	tout << "Model file: " << modelFilePath << endl;
	tout << "Properties: " << propsFilePath << endl;

	Parser parser;
	Verifier verifier;
	Precompiler precompiler;

    std::ifstream mfin(modelFilePath, ios::binary);
    std::stringstream ss;
    ss << mfin.rdbuf();

    // Parse the file with the model description
    parser.parse(&ss);
    ss.clear();
    ss << precompiler.pre_compile(GLOBAL_MODEL_AST,GLOBAL_PARSING_CONTEXT);
    parser.parse(&ss);
    verifier.verify(GLOBAL_MODEL_AST,GLOBAL_PARSING_CONTEXT);

    // Parse the file with the properties to check
    std::ifstream pfin(propsFilePath, ios::binary);
    ss.clear();
    ss << pfin.rdbuf();
    parser.parseProperties(&ss);

    // Compile everything into simulation model
	fig::CompileModel(GLOBAL_MODEL_AST, GLOBAL_PARSING_CONTEXT);
}
