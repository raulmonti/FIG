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
#include <cassert>
#include <sys/stat.h>

#include <fig.h>

// using std::make_tuple;
// typedef std::set< std::string > NamesList;
// typedef std::set< std::tuple<double,double,bool> > StopCond;

static void print_intro();
static void check_arguments(const int& argc, const char** argv);
static bool file_exists(const std::string& filepath);
static void build_model(const char* modelFilePath, const char* propsFilePath);



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
	}
	const size_t propertyIndex(0ul);  // check only first defined property


    /** TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO **/
    /** HERE WE SHOULD SIMULATE AND DO ALL THE STUFF CARLOS KNOWS ABOUT. **/

    std::cerr << "\nWell don't just stare, DO SOMETHING!\n\n";


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
	const std::string usage(std::string("Usage: ").append(argv[0])
							.append(" <modelFilePath> <propertiesFilePath>\n"));
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
