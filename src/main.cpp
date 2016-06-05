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
#include <iomanip>  // std::setprecision()
#include <fstream>
#include <sstream>
#include <string>
#include <list>
// C
#include <omp.h>
#include <cassert>
#include <sys/stat.h>
// FIG
#include <fig.h>
#include <fig_cli.h>
#include <string_utils.h>


//  Helper functions headers  //////////////////////////////////////////////////

static bool print_intro(const int& argc, const char** argv);
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
using fig_cli::globalTO;


//  Main stuff  ////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	auto log(fig::ModelSuite::log);
	auto tech_log(fig::ModelSuite::tech_log);
	auto const_argv(const_cast<const char**>(argv));
	const std::string FIG_ERROR("ERROR: FIG failed to");

	// "Greetings, human!" and command line parsing
	try {
		const bool versionQuery = print_intro(argc, const_argv);
		fig_cli::parse_arguments(argc, const_argv);  // exit on error
		if (versionQuery)
			exit(EXIT_SUCCESS);
	} catch (fig::FigException& e) {
		log(FIG_ERROR + " parse the command line.\n\n");
		tech_log("Error message: " + e.msg() + "\n");
		exit(EXIT_FAILURE);
	} catch (std::exception& e) {
		log("UNEXPECTED " + FIG_ERROR + " parse the command line.\n\n");
		tech_log(std::string("Error message: ") + e.what() + "\n");
		exit(EXIT_FAILURE);
	}

	// Compile model and properties files
	try {
		double start = omp_get_wtime();
		build_model(modelFile, propertiesFile);
		std::stringstream ss; ss << "Model building time: " << std::fixed;
		ss << std::setprecision(2) << omp_get_wtime()-start << " s\n\n";
		tech_log(ss.str());
	} catch (fig::FigException& e) {
		log(FIG_ERROR + " compile the model/properties file.\n\n");
		tech_log("Error message: " + e.msg() + "\n");
		exit(EXIT_FAILURE);
	} catch (std::exception& e) {
		log("UNEXPECTED " + FIG_ERROR + " compile the model/properties file.\n\n");
		tech_log(std::string("Error message: ") + e.what() + "\n");
		exit(EXIT_FAILURE);
	}

	// Estimate using requested configuration
	try {
		auto model = fig::ModelSuite::get_instance();
		model.set_global_timeout(globalTO);
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
	} catch (std::exception& e) {
		log("UNEXPECTED " + FIG_ERROR + " perform estimations.\n\n");
		tech_log(std::string("Error message: ") + e.what() + "\n");
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}


//  Helper functions implementations  //////////////////////////////////////////

bool print_intro(const int& argc, const char** argv)
{
	auto main_log = fig::ModelSuite::main_log;
	auto tech_log = fig::ModelSuite::tech_log;
	using std::to_string;
	const std::time_t now = std::chrono::system_clock::to_time_t(
								std::chrono::system_clock::now());

	// First check if this is a version query and we should omit the greeting
	if (argc == 2 && (trim(argv[1]) == "-v" || trim(argv[1]) == "--version"))
		return true;

	// Print the big fat greeting the user deserves
	main_log("\n");
	main_log(" ~~~~~~~~~ \n");
	main_log("  · FIG ·  \n");
	main_log(" ~~~~~~~~~ \n");
	main_log("           \n");
	main_log(" This is the Finite Improbability Generator.\n");
	main_log(" Version: " + std::string(fig_VERSION_STR) + "\n");
	main_log(" Build:   ");
	if (is_substring_ci(fig_CURRENT_BUILD, "release"))
		main_log("Release ");
	else
		main_log("Debug ");
#ifndef PCG_RNG
	main_log("(Mersenne-Twister RNG)\n");
#else
	main_log("(PCG family RNG)\n");
#endif
	main_log(" Authors: Budde, Carlos E. <cbudde@famaf.unc.edu.ar>\n");
	main_log("          Monti, Raúl E.   <raulmonti88@gmail.com>\n");
	main_log("\n");

	// Print additional technical info if this is more than a query
	if (argc > 1 && trim(argv[1]) != "-h" && trim(argv[1]) != "--help") {
		tech_log(std::string("\nFIG tool invoked on ") + std::ctime(&now));
		tech_log("Build: " fig_CURRENT_BUILD "\n");
		tech_log("64-bit RNG: ");
#ifndef PCG_RNG
		tech_log("STL's Mersenne-Twister ");
#else
		tech_log("Builtin PCG ");
#endif
#ifndef RANDOM_RNG_SEED
		tech_log("(seed: " + std::to_string(fig::Clock::rng_seed()) + ")\n\n");
#else
		tech_log("(seeded from system's random device)\n\n");
#endif
		tech_log("Invocation command:");
		for (int i = 0 ; i < argc ; i++)
			tech_log(std::string(" ") + argv[i]);
		tech_log("\n\n");
	}

	return false;
}


bool file_exists(const std::string& filepath)
{
	struct stat buffer;
	return (stat(filepath.c_str(), &buffer) == 0);
}


void build_model(const std::string& modelFilePath, const std::string& propsFilePath)
{
	auto log = fig::ModelSuite::log;
	auto tech_log = fig::ModelSuite::tech_log;

	log("Model file: " + modelFilePath);
	if (!file_exists(modelFilePath)) {
		log(" *** Error: file not found! ***\n");
		exit(EXIT_FAILURE);
	}
	log("\nProperties file: " + propsFilePath);
	if (!file_exists(propsFilePath)) {
		log(" *** Error: file not found! ***\n");
		exit(EXIT_FAILURE);
	}
	log("\n\n");

	Parser parser;
	Precompiler precompiler;

	std::ifstream mfin(modelFilePath, ios::binary);
	std::stringstream ss;
	ss << mfin.rdbuf();

	// Parse the file with the model description
	parser.parse(&ss);
	ss.str("");ss.clear();  // clear ss contents
	ss << precompiler.pre_compile( GLOBAL_MODEL_AST
                                 , GLOBAL_PARSING_CONTEXT
                                 , parser.get_lexemes());
	parser.parse(&ss);

	// Check if the model is small enough for IOSA-compliance verification...
	const size_t NTRANS_UBOUND(1ul<<7ul);  // arbitrary af
	bool verifyModel(true);
	for (const AST* module: GLOBAL_MODEL_AST->get_all_ast(parser::_MODULE)) {
		if (NTRANS_UBOUND < module->get_all_ast(parser::_TRANSITION).size()) {
			verifyModel = false;
			break;
		}
	}
	if (verifyModel) {
		// ...it is! Verify the model satisfies all IOSA conditions
		Verifier verifier;
		verifier.verify(GLOBAL_MODEL_AST, GLOBAL_PARSING_CONTEXT);
	} else {
		// ...some module is too big: inform the user and skip verification
		tech_log("Skipping model's IOSA-compliance verification since some "
				 "module has more than " + std::to_string(NTRANS_UBOUND) +
				 " transitions.\n");
	}

	// Parse the file with the properties to check
	std::ifstream pfin(propsFilePath, ios::binary);
	ss.str("");ss.clear();  // clear ss contents
	ss << pfin.rdbuf();
	parser.parseProperties(&ss);
	ss.str("");ss.clear();  // clear ss contents
	ss << precompiler.pre_compile_props( parser.get_lexemes()
                                       , GLOBAL_CONST_TABLE);
	parser.parseProperties(&ss);

	// Compile into simulation model
	fig::CompileModel(GLOBAL_MODEL_AST, GLOBAL_PARSING_CONTEXT);

	tech_log("Model and properties files successfully compiled.\n");
}
