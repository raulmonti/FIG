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

#include <fig.h>

static void print_intro(std::ostream& out);
static void check_arguments(const int& argc, const char** argv);
static void build_model(const char* modelFilePath, const char* propsFilePath);



int main(int argc, char** argv)
{
	//  Intro  // // // // // // // // // // // // // // // // // //
	print_intro(std::cout);
	check_arguments(argc, const_cast<const char**>(argv));

	//  Compile model and properties   // // // // // // // // // //
	build_model(argv[1], argv[2]);
	auto model = fig::ModelSuite::get_instance();
	if (!model.sealed()) {
		std::cerr << "ERROR: failed to build the model.\n";
		exit(EXIT_FAILURE);
	}


    /** TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO **/
    /** HERE WE SHOULD SIMULATE AND DO ALL THE STUFF CARLOS KNOWS ABOUT. **/

    std::cerr << "\nWell don't just stare, DO SOMETHING!\n\n";


	//  Free memory  // // // // // // // // // // // // // // // //
	model.release_resources();

	return EXIT_SUCCESS;
}


// ///////////////////////////////////////////////////////////////////////////
void print_intro(std::ostream& out)
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
	out << std::endl;
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
void build_model(const char* modelFilePath, const char* propsFilePath)
{

    tout << "Model file: "      << modelFilePath << endl;
    tout << "Properties file: " << propsFilePath << endl;

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
