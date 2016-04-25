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


// C
#include <cstdlib>  // std::strtoul()
// C++
#include <set>
#include <list>
#include <string>
// External code
#include <CmdLine.h>
#include <ValueArg.h>
#include <SwitchArg.h>
#include <ValuesConstraint.h>
#include <UnlabeledValueArg.h>
// FIG
#include <fig_cli.h>
#include <FigConfig.h>
#include <FigException.h>
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
fig::ImpFunSpec impFunSpec("noName", "noStrategy");
string thrTechnique;
std::set< unsigned > splittings;
std::list< fig::StoppingConditions > estBounds;

} // namespace fig_cli



namespace
{

using namespace fig_cli;


// TCLAP parameter holders and stuff  /////////////////////////////////////////

CmdLine cmd_("\nSample usage:\n"
			 "~$ ./fig models/tandem.{sa,pp} --auto-coupled --stop-time 5 m\n"
			 "Use an automatically computed importance function built on the "
			 "model's fully coupled state space, performing a 5 minutes "
			 "estimation which will employ the RESTART simulation engine "
			 "(default) for splitting 2 (default) and the hybrid thresholds "
			 "building technique, i.e. \"hyb\" (default)\n"
			 "~$ ./fig models/tandem.{sa,pp} --flat -e nosplit --stop-time 1 h\n"
			 "Use a flat importance function to perform a 1 hour standard "
			 "Monte Carlo simulation (i.e. no splitting)\n"
			 "~$ ./fig models/tandem.{sa,pp} --adhoc \"10*q2+q1\" -t ams \\"
			 "            --stop-conf 0.9 0.2\n"
			 "Use the importance function \"10*q2+q1\" defined ad hoc by the "
			 "user, with the RESTART simulation engine (default) for splitting "
			 "2 (default), employing the Adaptive Multilevel Splitting "
			 "thresholds building technique, i.e. \"ams\", estimating until "
			 "the relative precision achieved for a 90% confidence interval "
			 "equals 20% of the value estimated for each property.\n"
			 "~$ ./fig models/tandem.{sa,pp} --auto-split \"+\" -t hyb  \\"
			 "       --stop-conf .8 .4 --stop-time 1 h --stop-conf .95 .1    \\"
			 "       --splitting 2,3,5,9,11 -e restart\n"
			 "Use an automatically computed importance function modularly "
			 "built, viz. on every module state space, simulating with the "
			 "RESTART engine for the splitting values explicitly specified, "
			 "using the hybrid thresholds building technique, i.e. \"hyb\", "
			 "estimating the value of each property for this configuration "
			 "and for each one of the three stopping conditions.",
			 ' ',
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
	"t", "thresholds",
	"Technique to use for building the importance thresholds. "
	"Default is \"" + thrTechDefault + "\"",
	false, thrTechDefault,
	&thrTechConstraints);

// Importance function specifications
SwitchArg ifunFlat(
	"", "flat",
	"Use a flat importance function, i.e. consider all states in the model "
	"equally important. Information is kept symbolically as an algebraic "
	"expression, thus using very little memory. Notice the flat function is "
	"incompatible with RESTART-like simulation engines.");
SwitchArg ifunFlatCoupled(
	"", "flat-coupled",
	"Use a flat importance function, i.e. consider all states in the model "
	"equally important. Information is stored in a single, very big vector "
	"the size of the coupled model's concrete state space, which may be huge. "
	"Notice the flat function is incompatible with RESTART-like simulation "
	"engines.");
SwitchArg ifunFlatSplit(
	"", "flat-split",
	"Use a flat importance function, i.e. consider all states in the model "
	"equally important. Information is stored in several, relatively small "
	"vectors, one per module. Notice the flat function is incompatible with "
	"RESTART-like simulation engines.");
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
	"(relative to the estimate) to reach. This is a multi-argument, "
	"meaning you can define as many of them as you wish, "
	"e.g. \"--stop-conf 0.8 0.4 --stop-conf 0.95 0.1\"",
	false,
	&ccConstraint, &precConstraint);
NumericConstraint<long> timeLapseConstraint(
	[](const long& timeLapse) { return timeLapse > 0l; },
	"positive_time_lapse");
ValuesConstraint<char> timeUnitConstraint(std::vector<char>({'s', 'm', 'h', 'd'}));
MultiDoubleArg< long, char > timeCriteria(
	"", "stop-time",
	"Add a stopping condition for estimations based on a (wall clock) "
	"execution time length, e.g. \"45 m\". Can specify seconds (s), "
	"minutes (m), hours (h) or days (d). This is a multi-argument, "
	"meaning you can define as many of them as you wish, "
	"e.g. \"--stop-time 30 s --stop-time 10 m\"",
	false,
	&timeLapseConstraint, &timeUnitConstraint);
std::vector< Arg* > stopCondSpecs = {
	&confidenceCriteria,
	&timeCriteria
};

// Splitting values to test
ValueArg<string> splittings_(
	"s", "splitting",
	"Define splitting values to try out with RESTART-like simulation engines, "
	"specified as a comma-separated list of integral values greater than '1'",
	false, "2",
	"comma-separated-split-values");


// Helper routines  ///////////////////////////////////////////////////////////

/// Parse ad hoc ImportanceFunction specification details:
/// an user-defined algebraic expression, and optionally also
/// the minimum and maximum value the ImportanceFunction can take.
std::tuple<string, fig::ImportanceValue, fig::ImportanceValue>
parse_ifun_details(const std::string& details)
{
	fig::ImportanceValue min(0u), max(0u);
	char* err(nullptr);

	// Divide fields (split by semicolons)
	auto strValues = split(details, ';');
	for (auto& str: strValues)
		delete_substring(str, "\"");  // erase unescaped quotation marks

	// The algebraic expression must be defined, though it's not interpreted here
	assert(strValues.size() > 0ul);

	// The extreme values are optional
	if (strValues.size() > 1ul) {
		min = std::strtoul(strValues[1].data(), &err, 10);
		if (nullptr != err && err[0] != '\0') {
			// Mimic TCLAP 'parsing error' message style
			std::cerr << "PARSE ERROR: When parsing the algebraic expression \""
					  << details << "\"\n";
			std::cerr << "             After the first semicolon, the function's "
					  << "min value should've been provided.\n\n";
			return std::tuple<string,fig::ImportanceValue,fig::ImportanceValue>();
		}
	}
	if (strValues.size() > 2ul) {
		max = std::strtoul(strValues[2].data(), &err, 10);
		if (nullptr != err && err[0] != '\0') {
			// Mimic TCLAP 'parsing error' message style
			std::cerr << "PARSE ERROR: When parsing the algebraic expression \""
					  << details << "\"\n";
			std::cerr << "             After the second semicolon, the function's "
					  << "max value should've been provided.\n\n";
			return std::tuple<string,fig::ImportanceValue,fig::ImportanceValue>();
		}
	}

	assert(min <= max);
	return std::make_tuple(strValues[0], min, max);
}


/// Check the ImportanceFunction specification parsed from the command line
/// into the TCLAP holders. Use it to fill in the global information offered
/// to FIG for estimation (viz: 'impFunSpec')
/// @return Whether the information could be successfully retrieved
bool
get_ifun_specification()
{
	if (ifunFlat.isSet()) {
		new(&impFunSpec) fig::ImpFunSpec("algebraic", "flat");

	} else if (ifunFlatCoupled.isSet()) {
		new(&impFunSpec) fig::ImpFunSpec("concrete_coupled", "flat");

	} else if (ifunFlatSplit.isSet()) {
		new(&impFunSpec) fig::ImpFunSpec("concrete_split", "flat");

	} else if (ifunAutoCoupled.isSet()) {
		new(&impFunSpec) fig::ImpFunSpec("concrete_coupled", "auto");

	} else if (ifunAutoSplit.isSet()) {
		const string mergeFun =
				std::get<0>(parse_ifun_details(ifunAutoSplit.getValue()));
		if (mergeFun.empty())
			return false;  // something went wrong
		new(&impFunSpec) fig::ImpFunSpec("concrete_split", "auto", mergeFun);

	} else if (ifunAdhoc.isSet()) {
		const auto details = parse_ifun_details(ifunAdhoc.getValue());
		if (std::get<0>(details).empty())
			return false;  // something went wrong
		new(&impFunSpec) fig::ImpFunSpec("algebraic", "adhoc",
										 std::get<0>(details),
										 std::get<1>(details),
										 std::get<2>(details));

	} else if (ifunAdhocCoupled.isSet()) {
		const auto details = parse_ifun_details(ifunAdhocCoupled.getValue());
		if (std::get<0>(details).empty())
			return false;  // something went wrong
		new(&impFunSpec) fig::ImpFunSpec("concrete_coupled", "adhoc",
										 std::get<0>(details),
										 std::get<1>(details),
										 std::get<2>(details));
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
	if (splittings_.isSet()) {
		auto strValues = split(splittings_.getValue(), ',');
		char* err(nullptr);
		for (const auto& strValue: strValues) {
			unsigned value = std::strtoul(strValue.data(), &err, 10);
			if (nullptr == err || err[0] == '\0') {
				splittings.emplace(value);
			} else {
				// Mimic TCLAP 'parsing error' message style
				std::cerr << "PARSE ERROR: Argument: (--"
						  << splittings_.getName() << ")\n";
				std::cerr << "             Invalid value given \""
						  << err << "\"\n\n";
				return false;
			}
		}
	} else {
		// If we set a default for "splittings_" we shouldn't be here...
		// anyway, use a single default splitting value if none was specified
		splittings.emplace(2u);
	}
	return true;
}

} // namespace



namespace fig_cli
{

// Main parsing routine  //////////////////////////////////////////////////////

bool
parse_arguments(const int& argc, const char** argv, bool fatalError)
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
		if (!get_ifun_specification()) {
			std::cerr << "ERROR: must specify an importance function.\n\n";
			std::cerr << "For complete USAGE and HELP type:\n";
			std::cerr << "   " << argv[0] << " --help\n\n";
			if (fatalError)
				exit(EXIT_FAILURE);
			else
				return false;
		}
		if (!get_stopping_conditions()) {
			std::cerr << "ERROR: must specify at least one stopping condition ";
			std::cerr << "(aka estimation bound).\n\n";
			std::cerr << "For complete USAGE and HELP type:\n";
			std::cerr << "   " << argv[0] << " --help\n\n";
			if (fatalError)
				exit(EXIT_FAILURE);
			else
				return false;
		}
		if (!get_splitting_values()) {
			std::cerr << "ERROR: splitting values must be specified as a "
						 "comma-sperated list of positive integral values. "
						 "There should be no spaces in this list.\n\n";
			std::cerr << "For complete USAGE and HELP type:\n";
			std::cerr << "   " << argv[0] << " --help\n\n";
			if (fatalError)
				exit(EXIT_FAILURE);
			else
				return false;
		}

	} catch (ArgException& e) {
		throw_FigException(std::string("command line parsing failed "
						   "unexpectedly: ").append(e.what()));
	}

	return true;
}

} // namespace fig_cli

