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
#include <chrono>
#include <ctime>    // std::ctime()
#include <cassert>
#include <cstdlib>  // std::strtoul()
#include <sys/stat.h>

#include <fig.h>

typedef std::set< std::string > NamesList;
typedef std::set< std::tuple<double,double,bool> > StopCond;

static void print_intro();
static void check_arguments(const int& argc, const char** argv);
static bool file_exists(const std::string& filepath);
static void build_model(const char* modelFilePath, const char* propsFilePath);
static void set_global_splitting(const char* splitsPerThreshold);



int main(int argc, char** argv)
{
	//  Intro  // // // // // // // // // // // // // // // // // //
	print_intro();
	check_arguments(argc, const_cast<const char**>(argv));

	//  Compile model and properties   // // // // // // // // // //
	build_model(argv[1], argv[2]);
	auto model = fig::ModelSuite::get_instance();
	if (!model.sealed()) {
		std::cerr << "ERROR: failed to build the model.\n";
		exit(EXIT_FAILURE);
	} else {
		std::cout << std::endl;
		if (argc > 3)
			set_global_splitting(argv[3]);
	}
	const size_t propertyIndex(0ul);

	//  Estimation goals   // // // // // // // // // // // // // //
	const fig::StoppingConditions timeSpans(std::set<size_t>({25ul,90ul}));
	const double confidence(0.80);
	const double precision(0.4);
	const fig::StoppingConditions stopCriterion(StopCond({std::make_tuple(
			confidence, precision, true)}));
	std::shared_ptr< fig::SimulationEngine > engine(nullptr);

	//  Standard Monte Carlo     // // // // // // // // // // // //
	const std::string flatIfunName("algebraic");
	model.build_importance_function_flat(flatIfunName, propertyIndex);
	model.build_thresholds("smc", flatIfunName);
	engine = model.prepare_simulation_engine("nosplit", flatIfunName);
	model.estimate(propertyIndex, *engine, timeSpans);
	//model.estimate(propertyIndex, *engine, stopCriterion);
	engine = nullptr;

	//  RESTART with algebraic ad hoc   // // // // // // // // //
	const std::string adhocIfunName("algebraic");
//	model.build_importance_function_adhoc(adhocIfunName, propertyIndex, "q2", NamesList({"q2"}), true);
	model.build_importance_function_adhoc(adhocIfunName, propertyIndex, "q3", NamesList({"q3"}), true);
//	model.build_importance_function_adhoc(adhocIfunName, propertyIndex, "buf", NamesList({"buf"}), true);
	model.build_thresholds("smc", adhocIfunName);
	engine = model.prepare_simulation_engine("restart", adhocIfunName);
	//model.estimate(propertyIndex, *engine, timeSpans);
	model.estimate(propertyIndex, *engine, stopCriterion);
	engine = nullptr;

	//  RESTART with automatic coupled   // // // // // // // // //
	const std::string cAutoIfunName("concrete_coupled");
	model.build_importance_function_auto(cAutoIfunName, propertyIndex);
	model.build_thresholds("smc", cAutoIfunName);
	engine = model.prepare_simulation_engine("restart", cAutoIfunName);
	//model.estimate(propertyIndex, *engine, timeSpans);
	model.estimate(propertyIndex, *engine, stopCriterion);
	engine = nullptr;

	//  RESTART with automatic split  // // // // // // // // // //
	const std::string sAutoIfunName("concrete_split");
	model.build_importance_function_auto(sAutoIfunName, propertyIndex, "+", true);
	model.build_thresholds("smc", sAutoIfunName);
	engine = model.prepare_simulation_engine("restart", sAutoIfunName);
	//model.estimate(propertyIndex, *engine, timeSpans);
	model.estimate(propertyIndex, *engine, stopCriterion);
	engine = nullptr;

	//  Free memory  // // // // // // // // // // // // // // // //
	model.release_resources();

	return EXIT_SUCCESS;
}


// ///////////////////////////////////////////////////////////////////////////
void print_intro()
{
	auto log = fig::ModelSuite::main_log;
	using std::to_string;
	log("\n");
	log(" ~~~~~~~~~ \n");
	log("  · FIG ·  \n");
	log(" ~~~~~~~~~ \n");
	log("           \n");
	log(" This is the Finite Improbability Generator.\n");
	log(" Version: "+to_string(fig_VERSION_MAJOR)+"."+to_string(fig_VERSION_MINOR)+"\n");
	log(" Authors: Budde, Carlos E. <cbudde@famaf.unc.edu.ar>\n");
	log("          Monti, Raúl E.   <raulmonti88@gmail.com>\n");
	log("\n");
	std::time_t now = std::chrono::system_clock::to_time_t(
						  std::chrono::system_clock::now());
	fig::ModelSuite::tech_log("\nFIG tool invoked on ");
	fig::ModelSuite::tech_log(std::ctime(&now));
	fig::ModelSuite::tech_log("\n");
}


// ///////////////////////////////////////////////////////////////////////////
void check_arguments(const int& argc, const char** argv)
{
	const std::string help("--help");
	const std::string usage(std::string("Usage: ") + argv[0] +
			" <modelFilePath> <propertiesFilePath> [<splitting>]\n");
	if (argc < 3 && argc > 1 && help == argv[1]) {
		std::cerr << usage << std::endl;
		exit(EXIT_SUCCESS);
	} else if (argc < 3) {
		std::cerr << "ERROR: FIG invoked with too few parameters.\n";
		std::cerr << usage << std::endl;
		exit(EXIT_FAILURE);
	}
}


// ///////////////////////////////////////////////////////////////////////////
bool file_exists(const std::string& filepath)
{
	struct stat buffer;
	return (stat(filepath.c_str(), &buffer) == 0);
}


// ///////////////////////////////////////////////////////////////////////////
void build_model(const char* modelFilePath, const char* propsFilePath)
{
	// Look for specified files
	fig::ModelSuite::log(std::string("Model file: ") + modelFilePath);
	if (!file_exists(modelFilePath)) {
		fig::ModelSuite::log(" *** Error: file not found! ***\n");
		exit(EXIT_FAILURE);
	}
	fig::ModelSuite::log(std::string("\nProperties: ") + propsFilePath);
	if (!file_exists(propsFilePath)) {
		fig::ModelSuite::log(" *** Error: file not found! ***\n");
		exit(EXIT_FAILURE);
	}
	fig::ModelSuite::log("\n");

	// Open files as streams
	Parser parser;
	Verifier verifier;
	Precompiler precompiler;

    std::ifstream mfin(modelFilePath, ios::binary);
    std::stringstream ss;
    ss << mfin.rdbuf();

    // Parse the file with the model description
    parser.parse(&ss);
	ss = std::stringstream();
	ss << precompiler.pre_compile(GLOBAL_MODEL_AST,GLOBAL_PARSING_CONTEXT);
    parser.parse(&ss);
    verifier.verify(GLOBAL_MODEL_AST,GLOBAL_PARSING_CONTEXT);

    // Parse the file with the properties to check
    std::ifstream pfin(propsFilePath, ios::binary);
	ss = std::stringstream();
	ss << pfin.rdbuf();
	parser.parseProperties(&ss);
	ss = std::stringstream();
	ss << precompiler.pre_compile_props();
	parser.parseProperties(&ss);

	// Compile into simulation model
	fig::CompileModel(GLOBAL_MODEL_AST, GLOBAL_PARSING_CONTEXT);
}


// ///////////////////////////////////////////////////////////////////////////
void set_global_splitting(const char* splitsPerThreshold)
{
	char* err;
	unsigned long spt = std::strtoul(splitsPerThreshold, &err, 10);
	if ('\0' != *err) {
		fig::ModelSuite::log(std::string(" *** Error: bad splitting factor ")
							 + "specified: \"" + splitsPerThreshold + "\" ***\n");
		exit(EXIT_FAILURE);
	}
	fig::ModelSuite::tech_log(std::string("Specified global splitting factor")
							  + " = " + splitsPerThreshold + "\n");
	fig::ModelSuite::get_instance().set_splitting(spt);
}
