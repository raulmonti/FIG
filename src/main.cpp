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
#include <sstream>
#include <fstream>
#include <string>
#include <list>
// C
#include <cassert>
#include <cstring>  // strtod(), strtol()
#include <sys/stat.h>
// FIG
#include <fig.h>
#include <fig_cli.h>
#include <string_utils.h>  // trim()


//  Helper functions headers  //////////////////////////////////////////////////

static void print_intro();
//static bool check_arguments(const int& argc, const char** argv);
static bool file_exists(const std::string& filepath);
static void build_model(const std::string& modelFilePath, const std::string& propsFilePath);
//static fig::StoppingConditions parse_estimation_bounds(const char* confCo, const char* prec);
//static std::set<unsigned> parse_splitting_values(const char* values);


//  Configuration of the estimation run  ///////////////////////////////////////

using fig_cli::modelFile;
using fig_cli::propertiesFile;
using fig_cli::engineName;
using fig_cli::impFunName;
using fig_cli::impFunStrategy;
using fig_cli::impFunDetails;
using fig_cli::thrTechnique;
using fig_cli::splittings;
using fig_cli::estBound;


//  Main stuff  ////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	// Intro and invocation check
	print_intro();
	fig_cli::parse_arguments(argc, const_cast<const char**>(argv));  // exit on error
//	const bool details = check_arguments(argc, const_cast<const char**>(argv));
//	const std::string modelFile(argv[1]);
//	const std::string propertiesFile(argv[2]);
//	const std::string engineName(argv[3]);
//	const std::string impFunName(argv[4]);
//	const std::string impFunStrategy(argv[5]);
//	const std::string impFunDetails(details ? delete_substring(argv[6],"\"") : "");
//	const std::string thrTechnique(details ? argv[7] : argv[6]);
//	const fig::StoppingConditions estBound =  // just for now...
//			parse_estimation_bounds(details ? argv[8] : argv[7],
//									details ? argv[9] : argv[8]);
//	auto splittings = parse_splitting_values(argv[argc-1]);  // just for now
//					  // Last invocation parameter should've been a string
//					  // formatted "2 5 18..." specifying the splitting values

	// Compile model and properties files
	build_model(modelFile, propertiesFile);
	auto model = fig::ModelSuite::get_instance();
	if (!model.sealed()) {
		fig::ModelSuite::log("ERROR: failed to build the model.\n");
		exit(EXIT_FAILURE);
	}

	// Estimate using requested configuration
	model.process_batch(engineName,
						impFunName,
						std::make_pair(impFunStrategy, impFunDetails),
						thrTechnique,
						std::list<fig::StoppingConditions>({estBound}),
						splittings);

	// Free memory
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
bool check_arguments(const int& argc, const char** argv)
{
	const auto& npos = std::string::npos;
	auto log = fig::ModelSuite::log;
	auto main_log = fig::ModelSuite::main_log;
	auto tech_log = fig::ModelSuite::tech_log;
	static const std::string help("--help");
	static const std::string engine("engine");
	static const std::string ifun("ifun");
	static const std::string thr("thresholds");
	static const std::string usage(std::string("Usage: ") + argv[0] +
								   " <modelFilePath>"
								   " <propertiesFilePath>"
								   " <engineName>"
								   " <impFunName>"
								   " <impFunStrategy>"
								   " <thrTechnique>"
								   " <confidenceCoefficient>" // just for now...
								   " <relativePrecision>"     // just for now...
								   " \"<splittingValues>\"\n");
	static const std::string helpUsage("\nCall with \"" + help + " <opt>\" "
									   "for <opt> in {" + engine + ", " + ifun +
									   ", " + thr + "} to see detailed options "
									   "regarding the simulation engine, "
									   "importance function or thresholds "
									   "building respectively.\n");

	// Show it, show what they did
	tech_log("Invocation command:");
	for (int i = 0 ; i < argc ; i++)
		tech_log(std::string(" ") + argv[i]);
	tech_log("\n\n");

	// Check whether the invocation was for a help query
	if (argc > 1 && std::string(argv[1]).find(help) != npos) {
		if (argc < 3) {
			// general usage query
			main_log(usage);
			main_log(helpUsage);
		} else if (std::string(argv[2]).find(engine) != npos) {
			// simulation engine specification query
			throw_FigException("TODO: write this help message");
		} else if (std::string(argv[2]).find(ifun) != npos) {
			// importance function specification query
			throw_FigException("TODO: write this help message");
		} else if (std::string(argv[2]).find(thr) != npos) {
			// thresholds building specification query
			throw_FigException("TODO: write this help message");
		} else {
			// invalid query
			main_log("ERROR: incorrect FIG invocation.\n");
			tech_log(std::string("invalid help query option \"") + argv[2] + "\"");
		}
		exit(EXIT_SUCCESS);
	} else if (argc < 7 + 2 + 1 /* ConfInt specification, just fr now */) {
		main_log("ERROR: incorrect FIG invocation, too few parameters.\n");
		tech_log("\n" + usage + helpUsage);
		exit(EXIT_FAILURE);
	}

	// Tell whether there should be an extra parameter with details regarding
	// the importance function, e.g. a user defined ad hoc function
	const bool ifunDetails = trim(argv[4]) == "concrete_split" ||
							 trim(argv[5]) == "adhoc";
	if (ifunDetails && argc != 8 + 2 + 1 /* splitting values */) {
		if (trim(argv[4]) == "concrete_split") {
			main_log("ERROR: incorrect detailed FIG invocation.\n");
			tech_log("ERROR: incorrect detailed FIG invocation, "
					 "concrete_split importance function requires a "
					 "\"merge function\" passed as extra parameter. "
					 "See --help " + ifun + "\n\n" + usage);
		} else if (trim(argv[5]) == "adhoc") {
			main_log("ERROR: incorrect detailed FIG invocation.\n");
			tech_log("ERROR: incorrect detailed FIG invocation, "
					 "adhoc importance assessment strategy requires an "
					 "\"ad hoc mathematical expression\" passed as extra parameter. "
					 "See --help " + ifun + "\n\n" + usage);
		} else {
			log("ERROR: incorrect detailed FIG invocation.\n");
			tech_log("\n" + usage + helpUsage);
		}
		exit(EXIT_FAILURE);
	} else if (!ifunDetails && argc != 7 + 2 + 1 /* splitting values */) {
		log("ERROR: incorrect FIG invocation.\n");
		tech_log("\n" + usage + helpUsage);
		exit(EXIT_FAILURE);
	}

	return ifunDetails;
}



// ///////////////////////////////////////////////////////////////////////////
bool file_exists(const std::string& filepath)
{
	struct stat buffer;
	return (stat(filepath.c_str(), &buffer) == 0);
}



// ///////////////////////////////////////////////////////////////////////////
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
	ss << precompiler.pre_compile(GLOBAL_MODEL_AST,GLOBAL_PARSING_CONTEXT);
	parser.parse(&ss);
	verifier.verify(GLOBAL_MODEL_AST,GLOBAL_PARSING_CONTEXT);

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



// ///////////////////////////////////////////////////////////////////////////
static fig::StoppingConditions parse_estimation_bounds(const char* confCo,
													   const char* prec)
{
	char* err(nullptr);
	const double confidenceCoefficient = strtod(confCo, &err);
	if (nullptr != err && '\0' != err[0])
		throw_FigException(std::string("bad confidence coefficient specified \"")
						   + confCo + "\", offending characters: " + err);
	const double precision = strtod(prec, &err);
	if (nullptr != err && '\0' != err[0])
		throw_FigException(std::string("bad precision specified \"")
						   + prec + "\", offending characters: " + err);
	std::set<std::tuple<double,double,bool>> stopCond = {
		std::make_tuple(confidenceCoefficient, precision, true) };
	return fig::StoppingConditions(stopCond);
}



// ///////////////////////////////////////////////////////////////////////////
static std::set<unsigned> parse_splitting_values(const char *values)
{
	std::set<unsigned> splittingValues;  // ordered set, this is intentional
	std::string splittings(values);
	char* err(nullptr);

	replace_substring(splittings, "\"", "");
	while (!splittings.empty()) {
		const long l = std::strtol(splittings.c_str(), &err, 10);
		splittings = nullptr == err ? "" : trim(err);
		if (!splittings.empty() && !std::isdigit(splittings.c_str()[0]))
			throw_FigException("invalid splitting values: \"" + splittings + "\"");
		splittingValues.emplace(static_cast<unsigned>(l));
	}

	return splittingValues;
}
