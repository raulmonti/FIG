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
#include <ModelSuite.h>


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

CmdLine cmd_("http://c.xkcd.com/random/comic/", ' ',
			 to_string(fig_VERSION_MAJOR)+"."+to_string(fig_VERSION_MINOR));

// Model file path
UnlabeledValueArg<string> modelFile_(
		"modelFile",
		"Path to the SA model file to study",
		true, "",
		"modelFile");

// Properties file path
UnlabeledValueArg<string> propertiesFile_(
		"propertiesFile",
		"Path to the file with the properties to estimate",
		true, "",
		"propertiesFile");

// Simulation engine
ValuesConstraint<string> engineConstraints(fig::ModelSuite::available_simulators());
const string engineDefault("restart");
ValueArg<string> engineName_(
		"e", "engine",
		"Name of the simulation engine to use for estimations. "
		"Default is \"" + engineDefault + "\"",
		false, engineDefault,
		&engineConstraints);

// Importance function specifications
SwitchArg ifunFlat(
		"", "flat",
		"Use a flat importance function, i.e. consider all states in the model "
		"equally important. Information is kept symbolically as an algebraic "
		"expression, thus using very little memory.");
SwitchArg ifunFlatCoupled(
		"", "flat-coupled",
		"Use a flat importance function, i.e. consider all states in the model "
		"equally important, storing this (null) information in a vector the "
		"size of the concrete state space of the coupled model. This may use "
		"a huge amount of memory");
SwitchArg ifunFlatSplit(
		"", "flat-split",
		"Use a flat importance function, i.e. consider all states in the model "
		"equally important, storing this (null) information in several vectors, "
		"one per module.");
SwitchArg ifunAutoCoupled(
		"", "auto-coupled",
		"Use an automatically computed \"concrete_coupled\" importance "
		"function, i.e. store information for the coupled model state space. "
		"This may use a huge amount of memory.");
ValueArg<string> ifunAutoSplit(
		"", "auto-split",
		"Use an automatically computed \"concrete_split\" importance function "
		"(i.e. stores information separately for each module), employing the "
		"provided algebraic expression to merge the modules' importance. "
		"Default merge function is to add all modules' importance.",
		false, "+",
		"merge_fun");
std::vector< Arg* > impFunSpecs = {
	&ifunFlat,
	&ifunFlatCoupled,
	&ifunFlatSplit,
	&ifunAutoCoupled,
	&ifunAutoSplit
};

//// Importance function
//ValuesConstraint<string> impFunConstraints(fig::ModelSuite::available_importance_functions());
//const string impFunDefault("concrete_coupled");
//ValueArg<string> impFunName_(
//		"f", "importanceFunction",
//		"Name of the importance function to use for estimations. "
//		"Default is \"" + impFunDefault + "\"",
//		false, impFunDefault,
//		&impFunConstraints);
//
//// Importance assessment strategy
//ValuesConstraint<string> impStratConstraints(fig::ModelSuite::available_importance_strategies());
//const string impStratDefault("auto");
//ValueArg<string> impFunStrategy_(
//		"s", "importanceStrategy",
//		"Strategy to use for importance assessment of the model states. "
//		"Default is \"" + impStratDefault + "\"",
//		false, impStratDefault,
//		&impStratConstraints);
//
//// Importance function ad hoc assessment expression
//ValueArg<string> impFunAdhoc_(
//		"", "adhoc",
//		"Ad hoc algebraic expression to use for importance assessment "
//		"of the model states. ",
//		false, "");

// Thresholds builder
ValuesConstraint<string> thrTechConstraints(fig::ModelSuite::available_threshold_techniques());
const string thrTechDefault("hyb");
ValueArg<string> thrTechnique_(
		"t", "thresholdsTechnique",
		"Technique to use for building the importance thresholds. "
		"Default is \"" + thrTechDefault + "\"",
		false, thrTechDefault,
		&thrTechConstraints);



// Main parsing routine  //////////////////////////////////////////////////////

bool parse_arguments(const int& argc, const char** argv, bool fatalError)
{
	try {
		// Add all arguments and options defined
		cmd_.add(modelFile_);
		cmd_.add(propertiesFile_);
		cmd_.add(engineName_);
		cmd_.xorAdd(impFunSpecs);
//		cmd_.add(impFunName_);
//		cmd_.add(impFunStrategy_);
		cmd_.add(thrTechnique_);
		/// @todo TODO implement the rest

		// Parse the command line
		cmd_.parse(argc, argv);

		// Fill the globally offered objects
		modelFile      = modelFile_.getValue();
		propertiesFile = propertiesFile_.getValue();
		engineName     = engineName_.getValue();
//		impFunName     = impFunName_.getValue();
//		impFunStrategy = impFunStrategy_.getValue();
		thrTechnique   = thrTechnique_.getValue();
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
