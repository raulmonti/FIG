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
#include <set>
#include <list>
#include <string>
// External code
#include <CmdLine.h>
// FIG
#include <fig_cli.h>
#include <FigConfig.h>
#include <ModelSuite.h>
#include <MultiDoubleArg.h>
#include <NumericConstraint.h>


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
std::list< fig::StoppingConditions > estBounds;

} // namespace fig_cli



namespace
{

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
NumericConstraint<float> ccConstraint(
	[](const float& cc) { return 0.0f < cc && cc < 1.0f; },
	"confidence_coefficient ∈ (0,1)");
NumericConstraint<float> precConstraint(
	[](const float& prec) { return 0.0f < prec; },
	"positive_relative_precision");
MultiDoubleArg< float, float > confidenceCriteria(
	"", "stop-conf",
	"Add a stopping condition for estimations based on a confidence "
	"criterion, i.e. a \"confidence coefficient\" and a \"precision\" "
	"(relative to the estimate) to reach.",
	false,
	&ccConstraint, &precConstraint);
NumericConstraint<long> timeLapseConstraint(
	[](const long& timeLapse) { return timeLapse > 0l; },
	"positive_time_lapse");
//const std::vector<string> timeUnits({"s", "m", "h", "d"});
ValuesConstraint<char> timeUnitConstraint(std::vector<char>({'s', 'm', 'h', 'd'}));
MultiDoubleArg< long, char > timeCriteria(
	"", "stop-time",
	"Add a stopping condition for estimations based on a (wall clock) "
	"execution time length, e.g. \"45 m\". Can specify seconds (s), "
	"minutes (m), hours (h) or days (d).",
	false,
	&timeLapseConstraint, &timeUnitConstraint);
std::vector< Arg* > stopCondSpecs = {
	&confidenceCriteria,
	&timeCriteria
};

// Splitting values to test
ValueArg<string> splittings_(
	"", "splitting",
	"Define splitting values to try out with RESTART-like simulation engines",
	false,
	"comma-separated list of positive integral values");


// Helper routines  ///////////////////////////////////////////////////////////

/// Check the ImportanceFunction specification parsed from the command line
/// into the TCLAP holders. Use it to fill in the global information offered to
/// FIG for estimation (viz: 'impFunName', 'impFunStrategy' and 'impFunDetails')
/// @return Whether the information could be successfully retrieved
bool
get_ifun_specification()
{
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
		return false;
	}
	return true;
}


/// Check the StoppingCondition specifications parsed from the command line
/// into the TCLAP holders. Use them to fill in the global information offered
/// to FIG for estimation (viz: the 'estBounds' list)
/// @return Whether the information could be successfully retrieved
bool
get_stopping_conditions()
{
	if (!confidenceCriteria.isSet() && !timeCriteria.isSet())
		return false;
	if (confidenceCriteria.isSet()) {
		fig::StoppingConditions stopCond;
		for (const auto& confCrit: confidenceCriteria.getValues())
			stopCond.add_confidence_criterion(confCrit.first, confCrit.second, true);
		estBounds.emplace(estBounds.begin(), stopCond);
	}
	if (timeCriteria.isSet()) {
		fig::StoppingConditions stopCond;
		for (const auto& timeCrit: timeCriteria.getValues()) {
			const size_t FACTOR( timeCrit.second == 's' ? 1ul     :
								 timeCrit.second == 'm' ? 60ul    :
								 timeCrit.second == 'h' ? 3600ul  :
								 timeCrit.second == 'd' ? 86400ul : 0ul );
			stopCond.add_time_budget(timeCrit.first*FACTOR);
		}
		estBounds.emplace(estBounds.begin(), stopCond);
	}
	return true;
}


/// Check the splitting values specifications parsed from the command line
/// into the TCLAP holders. Use them to fill in the global information offered
/// to FIG for estimation (viz: the 'splittings' set)
/// @return Whether the information could be successfully retrieved
bool
get_splitting_values()
{
	if (!splittings_.isSet())
		return true;  // a single default splitting value is used
	auto strValues = split(splittings_.getValue(), ',');

	/// @todo TODO finish up

	return true;
}

} // namespace



namespace fig_cli
{

// Main parsing routine  //////////////////////////////////////////////////////

bool parse_arguments(const int& argc, const char** argv, bool fatalError)
{
	try {
		// Add all defined arguments and options to TCLAP's command line parser
		cmd_.add(modelFile_);
		cmd_.add(propertiesFile_);
		cmd_.add(engineName_);
		cmd_.add(thrTechnique_);
		cmd_.orAdd(stopCondSpecs);
		cmd_.xorAdd(impFunSpecs);
		cmd_.add(splittings_);

		// Parse the command line input
		cmd_.parse(argc, argv);

		// Fill the globally offered objects
		modelFile      = modelFile_.getValue();
		propertiesFile = propertiesFile_.getValue();
		engineName     = engineName_.getValue();
		thrTechnique   = thrTechnique_.getValue();
		bool ifunDefined = get_ifun_specification();
		if (!ifunDefined) {
			std::cerr << "ERROR: must specify an importance function.\n\n";
			std::cerr << "For complete USAGE and HELP type:\n";
			std::cerr << "   " << argv[0] << " --help\n\n";
			if (fatalError)
				exit(EXIT_FAILURE);
			else
				return false;
		}
		bool stopCondDefined = get_stopping_conditions();
		if (!stopCondDefined) {
			std::cerr << "ERROR: must specify at least one stopping condition ";
			std::cerr << "(aka estimation bound).\n\n";
			std::cerr << "For complete USAGE and HELP type:\n";
			std::cerr << "   " << argv[0] << " --help\n\n";
			if (fatalError)
				exit(EXIT_FAILURE);
			else
				return false;
		}
		bool splittingsDefined = get_splitting_values();
		if (!splittingsDefined) {
			std::cerr << "ERROR: splitting values must be given as a comma-"
						 "sperated list of positive integral values. "
						 "There should be no spaces in this list.";
			std::cerr << "(aka estimation bound).\n\n";
			std::cerr << "For complete USAGE and HELP type:\n";
			std::cerr << "   " << argv[0] << " --help\n\n";
			if (fatalError)
				exit(EXIT_FAILURE);
			else
				return false;
		}

	} catch (ArgException& e) {
		std::cerr << "ERROR: " << e.what() << "\n";
		if (fatalError)
			exit(EXIT_FAILURE);
		else
			return false;
	}

	return true;
}

} // namespace fig_cli

