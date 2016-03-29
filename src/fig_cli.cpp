//==============================================================================
//
//  fig_cli.cpp
//
//  Copyleft 2016-
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


// C++
#include <string>
// External code
#include <CmdLine.h>
// FIG
#include <fig_cli.h>
#include <FigConfig.h>


using namespace TCLAP;
using std::to_string;
using std::string;

namespace fig_cli
{

// Objects offered to configure the estimation runs  //////////////////////////

string modelFile;
string propertiesFile;
string engineName;
string impFunName;
string impFunStrategy;
string impFunDetails;
string thrTechnique;
std::set< unsigned > splittings;
fig::StoppingConditions estBound;


// TCLAP parameter holders and stuff  /////////////////////////////////////////

CmdLine cmd_("You too have a nice day good sir", ' ',
			 to_string(fig_VERSION_MAJOR)+"."+to_string(fig_VERSION_MINOR));

UnlabeledValueArg<string> modelFile_(
		"modelFile",
		"Path to the SA model file to study",
		true, "",
		"modelFile");

UnlabeledValueArg<string> propertiesFile_(
		"propertiesFile",
		"Path to the file with the properties whose value is to be estimated",
		true, "",
		"propertiesFile");


// Main parsing routine  //////////////////////////////////////////////////////

bool parse_arguments(const int& argc, const char** argv, bool fatalError)
{
	try {
		// Add all arguments and options defined
		cmd_.add(modelFile_);
		cmd_.add(propertiesFile_);
		/// @todo TODO implement the rest

		// Parse the command line
		cmd_.parse(argc, argv);

		// Fill the globally offered objects
		modelFile = modelFile_.getValue();
		propertiesFile = propertiesFile_.getValue();
		/// @todo TODO implement the rest

	} catch (ArgException& e) {
		std::cerr << "ERROR: " << e.what() << "\n";
		if (fatalError)
			exit(EXIT_FAILURE);
		else
			return false;
	}

	return true;
}


} // namespace fig
