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


// C++
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
// C
#include <cassert>
#include <sys/stat.h>
// FIG
#include <fig.h>
#include <fig_cli.h>


//  Helper functions headers  //////////////////////////////////////////////////

static void print_intro(const int &argc, const char **argv);
static bool file_exists(const std::string& filepath);
static void build_model(const std::string& modelFilePath,
						const std::string& propsFilePath);


//  Configuration of the estimation run  ///////////////////////////////////////

using fig_cli::modelFile;
using fig_cli::propertiesFile;
using fig_cli::engineName;
using fig_cli::impFunSpec;
using fig_cli::thrTechnique;
using fig_cli::splittings;
using fig_cli::estBounds;


//  Main stuff  ////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	auto log(fig::ModelSuite::log);
	auto tech_log(fig::ModelSuite::tech_log);
	auto const_argv(const_cast<const char**>(argv));
	const std::string FIG_ERROR("ERROR: FIG failed to");

	// Greeting and command line parsing
	try {
		print_intro(argc, const_argv);
		fig_cli::parse_arguments(argc, const_argv);  // exit on error
	} catch (fig::FigException& e) {
		log(FIG_ERROR + " parse the command line.\n\n");
		tech_log("Parse error message: " + e.msg() + "\n");
		exit(EXIT_FAILURE);
	}

	// Compile model and properties files
	try {
		build_model(modelFile, propertiesFile);
	} catch (fig::FigException& e) {
		log(FIG_ERROR + " compile the model/properties file.\n\n");
		tech_log("Parse error message: " + e.msg() + "\n");
		exit(EXIT_FAILURE);
	}
	auto model = fig::ModelSuite::get_instance();
	if (!model.sealed()) {
		log(FIG_ERROR + " build the model.\n\n");
		exit(EXIT_FAILURE);
	} else {
		tech_log("Model and properties files successfully compiled.\n");
	}

	// Estimate using requested configuration
	try {
		model.process_batch(engineName,
							impFunSpec,
							thrTechnique,
							estBounds,
							splittings);
		model.release_resources();
	} catch (fig::FigException& e) {
		log(FIG_ERROR + " perform estimations.\n\n");
		tech_log("Error message: " + e.msg() + "\n");
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}


//  Helper functions implementations  //////////////////////////////////////////

void print_intro(const int& argc, const char** argv)
{
	auto main_log = fig::ModelSuite::main_log;
	auto tech_log = fig::ModelSuite::tech_log;
	using std::to_string;
	const std::time_t now = std::chrono::system_clock::to_time_t(
								std::chrono::system_clock::now());
	main_log("\n");
	main_log(" ~~~~~~~~~ \n");
	main_log("  · FIG ·  \n");
	main_log(" ~~~~~~~~~ \n");
	main_log("           \n");
	main_log(" This is the Finite Improbability Generator.\n");
	main_log(" Version: "+to_string(fig_VERSION_MAJOR)+"."+to_string(fig_VERSION_MINOR)+"\n");
	main_log(" Build:   " fig_CURRENT_BUILD "\n");
	main_log(" Authors: Budde, Carlos E. <cbudde@famaf.unc.edu.ar>\n");
	main_log("          Monti, Raúl E.   <raulmonti88@gmail.com>\n");
	main_log("\n");

	tech_log(std::string("\nFIG tool invoked on ") + std::ctime(&now) + "\n");
	tech_log("Invocation command:");
	for (int i = 0 ; i < argc ; i++)
		tech_log(std::string(" ") + argv[i]);
	tech_log("\n\n");
}


bool file_exists(const std::string& filepath)
{
	struct stat buffer;
	return (stat(filepath.c_str(), &buffer) == 0);
}


void build_model(const std::string& modelFilePath, const std::string& propsFilePath)
{
	fig::ModelSuite::log("Model file: " + modelFilePath);
	if (!file_exists(modelFilePath)) {
		fig::ModelSuite::log(" *** Error: file not found! ***\n");
		exit(EXIT_FAILURE);
	}
	fig::ModelSuite::log("\nProperties file: " + propsFilePath);
	if (!file_exists(propsFilePath)) {
		fig::ModelSuite::log(" *** Error: file not found! ***\n");
		exit(EXIT_FAILURE);
	}
	fig::ModelSuite::log("\n\n");

	Parser parser;
	Verifier verifier;
	Precompiler precompiler;

	std::ifstream mfin(modelFilePath, ios::binary);
	std::stringstream ss;
	ss << mfin.rdbuf();

	// Parse the file with the model description
	parser.parse(&ss);
	ss.str("");ss.clear();  // clear ss contents
	ss << precompiler.pre_compile(GLOBAL_MODEL_AST, GLOBAL_PARSING_CONTEXT);
	parser.parse(&ss);
	verifier.verify(GLOBAL_MODEL_AST, GLOBAL_PARSING_CONTEXT);

	// Parse the file with the properties to check
	std::ifstream pfin(propsFilePath, ios::binary);
	ss.str("");ss.clear();  // clear ss contents
	ss << pfin.rdbuf();
	parser.parseProperties(&ss);
	ss.str("");ss.clear();  // clear ss contents
	ss << precompiler.pre_compile_props();
	parser.parseProperties(&ss);

	// Compile into simulation model
	fig::CompileModel(GLOBAL_MODEL_AST, GLOBAL_PARSING_CONTEXT);
}
