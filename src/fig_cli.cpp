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
#include <MultiDoubleArg.h>


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

CmdLine cmd_("Sample usage:\n\n ~$ wget http://c.xkcd.com/random/comic/", ' ',
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

// Thresholds builder
ValuesConstraint<string> thrTechConstraints(fig::ModelSuite::available_threshold_techniques());
const string thrTechDefault("hyb");
ValueArg<string> thrTechnique_(
		"t", "thresholdsTechnique",
		"Technique to use for building the importance thresholds. "
		"Default is \"" + thrTechDefault + "\"",
		false, thrTechDefault,
		&thrTechConstraints);

// Importance function specifications
SwitchArg ifunFlat(
		"", "flat",
		"Use a flat importance function, i.e. consider all states in the model "
		"equally important. Information is kept symbolically as an algebraic "
		"expression, thus using very little memory.");
SwitchArg ifunFlatCoupled(
		"", "flat-coupled",
		"Use a flat importance function, i.e. consider all states in the model "
		"equally important. Information is stored in a single, very big vector "
		"the size of the coupled model's concrete state space, which may be huge.");
SwitchArg ifunFlatSplit(
		"", "flat-split",
		"Use a flat importance function, i.e. consider all states in the model "
		"equally important. Information is stored in several, relatively small "
		"vectors, one per module.");
SwitchArg ifunAutoCoupled(
		"", "auto-coupled",
		"Use an automatically computed \"coupled\" importance function, "
		"i.e. store information for the coupled model. This stores "
		"in memory a vector the size of the coupled model's concrete state "
		"space, which may be huge.");
ValueArg<string> ifunAutoSplit(
		"", "auto-split",
		"Use an automatically computed \"split\" importance function, "
		"i.e. store information separately for each module. This stores in "
		"memory one small vector per module, and then uses the algebraic "
		"expression provided to merge these \"split\" importance values.",
		false, "",
		"merge_fun");
ValueArg<string> ifunAdhoc(
		"", "adhoc",
		"Use an ad hoc importance function, i.e. assign importance to the "
		"states using an user-provided algebraic function on them. "
		"Information is kept symbolically as an algebraic expression, "
		"thus using very little memory.",
		false, "",
		"adhoc_fun");
ValueArg<string> ifunAdhocCoupled(
		"", "adhoc-coupled",
		"Use an ad hoc importance function, i.e. assign importance to the "
		"states using an user-provided algebraic function on them. "
		"Information is stored in a single, very big vector the size of the "
		"coupled model's concrete state space, which may be huge.",
		false, "",
		"adhoc_fun");
std::vector< Arg* > impFunSpecs = {
	&ifunFlat,
	&ifunFlatCoupled,
	&ifunFlatSplit,
	&ifunAutoCoupled,
	&ifunAutoSplit,
	&ifunAdhoc,
	&ifunAdhocCoupled
};

// Stopping conditions (aka estimation bounds)
MultiDoubleArg< double, double > confidenceCriterion(
		"", "stop-conf",
		"Add a stopping condition for estimations based on a confidence "
		"criterion, i.e. a confidence coefficient and precision to reach.",
		true,
		"pair(confidence_coefficient,precision)");
//MultiDoubleArg< size_t, char > timeCriterion(
//		"", "--stop-time"
//		/** @todo TODO implement */);


// Main parsing routine  //////////////////////////////////////////////////////

bool parse_arguments(const int& argc, const char** argv, bool fatalError)
{
	try {
		// Add all arguments and options defined
		cmd_.add(modelFile_);
		cmd_.add(propertiesFile_);
		cmd_.add(engineName_);
		cmd_.add(thrTechnique_);
		cmd_.xorAdd(impFunSpecs);
		cmd_.add(confidenceCriterion);
//		cmd_.add(timeCriterion());
		/// @todo TODO implement the rest

		// Parse the command line
		cmd_.parse(argc, argv);

		// Fill the globally offered objects
		modelFile      = modelFile_.getValue();
		propertiesFile = propertiesFile_.getValue();
		engineName     = engineName_.getValue();
		thrTechnique   = thrTechnique_.getValue();
		if (ifunFlat.isSet()) {
			impFunName = "algebraic";
			impFunStrategy = "flat";
		} else if (ifunFlatCoupled.isSet()) {
			impFunName = "concrete_coupled";
			impFunStrategy = "flat";
		} else if (ifunFlatSplit.isSet()) {
			impFunName = "concrete_split";
			impFunStrategy = "flat";
		} else if (ifunAutoCoupled.isSet()) {
			impFunName = "concrete_coupled";
			impFunStrategy = "auto";
		} else if (ifunAutoSplit.isSet()) {
			impFunName = "concrete_split";
			impFunStrategy = "auto";
			impFunDetails = ifunAutoSplit.getValue();
		} else if (ifunAdhoc.isSet()) {
			impFunName = "algebraic";
			impFunStrategy = "adhoc";
			impFunDetails = ifunAdhoc.getValue();
		} else if (ifunAdhocCoupled.isSet()) {
			impFunName = "concrete_coupled";
			impFunStrategy = "adhoc";
			impFunDetails = ifunAdhocCoupled.getValue();
		} else {
			std::cerr << "ERROR: must specify an importance function.\n\n";
			std::cerr << "For complete USAGE and HELP type:\n";
			std::cerr << "   " << argv[0] << " --help\n\n";
			if (fatalError)
				exit(EXIT_FAILURE);
			else
				return false;
		}
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
