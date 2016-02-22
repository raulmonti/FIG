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


int main(int argc, char** argv)
{
	print_intro(std::cout);

    if (argc <= 1) {
		std::cerr << "ERROR: must call with the name of the file\n"
					 "       with the model described in IOSA syntax.\n";
		exit(EXIT_FAILURE);
	}

    Parser      parser      = Parser();
    Verifier    verifier    = Verifier();
    Precompiler precompiler = Precompiler();

	// Read the model
	std::ifstream fin(argv[1], ios::binary);
	std::stringstream ss;
    ss << fin.rdbuf();
	// Parse the model
	std::pair<AST*, parsingContext> pp = parser.parse(& ss);
	if (pp.first) {
		try {
			std::stringstream pss;
			// Solve constants (precompile)
            pss << precompiler.pre_compile(pp.first,pp.second);
            delete pp.first;
			// Parse again with solved constants
            pp = parser.parse(&pss);
			// Verify IOSA compliance and other stuff
            verifier.verify(pp.first,pp.second);
			// Compile to a simulation model
			fig::CompileModel(pp.first,pp.second);
		} catch (fig::FigException &e) {
            delete pp.first;   
            throw e;
        }
    }
    /** TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO **/
    /** HERE WE SHOULD SIMULATE AND DO ALL THE STUFF CARLOS KNOWS ABOUT. **/

	auto model = fig::ModelSuite::get_instance();
	assert(model.sealed());

    /* Free the parsed model */
    delete pp.first;
	return 0;
}


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
