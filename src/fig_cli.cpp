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
#include <cctype>   // std::isdigit()
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
#include <TimeConstraint.h>


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
std::chrono::seconds globalTO;

} // namespace fig_cli



namespace
{

using namespace fig_cli;
using fig::ImportanceValue;


/// FIG's full --version message
const std::string versionStr(
	"FIG tool " + std::string(fig_VERSION_STR) + "\n"
	"Build: " + std::string(fig_CURRENT_BUILD) + " with "
#if   !defined RANDOM_RNG_SEED && !defined PCG_RNG
	"MT RNG (seed: " + to_string(fig::Clock::rng_seed()) + ")"
#elif !defined RANDOM_RNG_SEED &&  defined PCG_RNG
	"PCG RNG (seed:" + to_string(fig::Clock::rng_seed()) + ")"
#elif !defined PCG_RNG
	"MT RNG (random seed)");
#else
	"PCG RNG (random seed)");
#endif
);


// TCLAP parameter holders and stuff  /////////////////////////////////////////

CmdLine cmd_("\nSample usage:\n"
			 "~$ ./fig models/tandem.{sa,pp} --amono --stop-time 5m\n"
			 "Use an automatically computed \"monolithic\" importance function "
			 "built on the global model's state space, performing a 5 minutes "
			 "estimation which will employ the RESTART simulation engine "
			 "(default) for splitting 2 (default) and the hybrid thresholds "
			 "building technique, i.e. \"hyb\" (default)\n"
			 "~$ ./fig models/tandem.{sa,pp} --flat -e nosplit          \\"
			 "       --stop-conf 0.8 0.4 --global-timeout 1h\n"
			 "Use a flat importance function to perform a standard Monte Carlo "
			 "simulation (i.e. no splitting), which will run until either "
			 "the relative precision achieved for an 80% confidence interval "
			 "equals 40% of the value estimated for each property, or 1 hour "
			 "of wall clock time has passed, whichever happens first.\n"
			 "~$ ./fig models/tandem.{sa,pp} --adhoc \"10*q2+q1\" -t ams  \\"
			 "       --stop-conf 0.9 0.2\n"
			 "Use the importance function \"10*q2+q1\" defined ad hoc by the "
			 "user, with the RESTART simulation engine (default) for splitting "
			 "2 (default), employing the Adaptive Multilevel Splitting "
			 "thresholds building technique, i.e. \"ams\", estimating until "
			 "the relative precision achieved for a 90% confidence interval "
			 "equals 20% of the value estimated for each property.\n"
			 "~$ ./fig models/tandem.{sa,pp} --acomp \"+\" -t hyb         \\"
			 "       --stop-conf .8 .4 --stop-time 1h --stop-conf .95 .1       \\"
			 "       --splitting 2,3,5,9,15 -e restart --global-timeout 30m\n"
			 "Use an automatically computed \"compositional\" importance "
			 "function, built modularly on every module's state space, "
			 "simulating with the RESTART engine for the splitting values "
			 "explicitly specified, using the hybrid thresholds building "
			 "technique, i.e. \"hyb\", estimating the value of each property "
			 "for this configuration and for each one of the three stopping "
			 "conditions, or until the the global wall-clock timeout is "
			 "reached in each case.",
			 ' ', versionStr);

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
//SwitchArg ifunFlatCoupled(
//	"", "flat-coupled",
//	"Use a flat importance function, i.e. consider all states in the model "
//	"equally important. Information is stored in a single, very big vector "
//	"the size of the coupled model's concrete state space, which may be huge. "
//	"Notice the flat function is incompatible with RESTART-like simulation "
//	"engines.");
//SwitchArg ifunFlatSplit(
//	"", "flat-split",
//	"Use a flat importance function, i.e. consider all states in the model "
//	"equally important. Information is stored in several, relatively small "
//	"vectors, one per module. Notice the flat function is incompatible with "
//	"RESTART-like simulation engines.");
ValueArg<string> ifunAdhoc(
	"", "adhoc",
	"Use an ad hoc importance function, i.e. assign importance to the "
	"states using an user-provided algebraic function on them. "
	"Information is kept symbolically as an algebraic expression, "
	"thus using very little memory.",
	false, "",
	"adhoc_fun");
//ValueArg<string> ifunAdhocCoupled(
//	"", "adhoc-coupled",
//	"Use an ad hoc importance function, i.e. assign importance to the "
//	"states using an user-provided algebraic function on them. "
//	"Information is stored in a single, very big vector the size of the "
//	"coupled model's concrete state space, which may be huge.",
//	false, "",
//	"adhoc_fun");
SwitchArg ifunAutoMonolithic(
	"", "amono",
	"Use an automatically computed \"monolithic\" importance function, "
	"i.e. store information for the global, parallely composed model. "
	"This stores in memory a vector the size of the global model's "
	"concrete state space, which may be huge.");
SwitchArg ifunAutoMonolithicExponential(
	"", "amono-exp",
	"Just like \"--amono\" but store the exponentiation of the "
	"automatically computed importance values.");
ValueArg<string> ifunAutoCompositional(
	"", "acomp",
	"Use an automatically computed \"compositional\" importance function, "
	"i.e. store information separately for each module. This stores in "
	"memory one vector per module, and then uses the algebraic expression "
	"provided to \"compose\" the global importance from these.",
	false, "",
	"composition_fun");
ValueArg<string> ifunAutoCompositionalExponential(
	"", "acomp-exp",
	"Just like \"--acomp\" but store in each module the exponentiation "
	"of its automatically computed importance.",
	false, "",
	"composition_fun");
std::vector< Arg* > impFunSpecs = {
	&ifunFlat,
//	&ifunFlatCoupled,
//	&ifunFlatSplit,
	&ifunAdhoc,
//	&ifunAdhocCoupled
	&ifunAutoMonolithic,
	&ifunAutoMonolithicExponential,
	&ifunAutoCompositional,
	&ifunAutoCompositionalExponential,
};

// Stopping conditions (aka estimation bounds)
NumericConstraint<float> ccConstraint(
	[](const float& cc) { return 0.0f < cc && cc < 1.0f; },
	"confCo ∈ (0,1)");
NumericConstraint<float> precConstraint(
	[](const float& prec) { return 0.0f < prec; },
	"relPrec ∈ (0,1)");
MultiDoubleArg< float, float > confidenceCriteria(
	"", "stop-conf",
	"Add a stopping condition for estimations based on a confidence "
	"criterion, i.e. a \"confidence coefficient\" and a \"precision\" "
	"(relative to the estimate) to reach. This is a multi-argument, "
	"meaning you can define as many of them as you wish, "
	"e.g. \"--stop-conf 0.8 0.4 --stop-conf 0.95 0.1\"",
	false,
	&ccConstraint, &precConstraint);
TimeConstraint<string> timeDurationConstraint;
MultiArg<string> timeCriteria(
	"", "stop-time",
	"Add a stopping condition for estimations based on a (wall clock) "
	"execution time length, e.g. \"45m\". Can specify seconds (s), "
	"minutes (m), hours (h) or days (d). Default is seconds, i.e. 's' is "
	"assumed if no suffix is specified. This is a multi-argument, "
	"meaning you can define as many of them as you wish, "
	"e.g. \"--stop-time 30 --stop-time 10m\"",
	false, &timeDurationConstraint);
std::vector< Arg* > stopCondSpecs = {
	&confidenceCriteria,
	&timeCriteria
};

// Global timeout (affects any simulation launched)
ValueArg<string> globalTimeout(
	"", "global-timeout",
	"Global time limit: if set, any simulation will be soft-interrupted after "
	"running for this (wall clock) time. If the stopping condition for a "
	"simulation is also time based, the lowest of the two values will apply.",
	false, "",
	&timeDurationConstraint);

// Splitting values to test
ValueArg<string> splittings_(
	"s", "splitting",
	"Define splitting values to try out with RESTART-like simulation engines, "
	"specified as a comma-separated list of integral values greater than '1'",
	false, "2",
	"comma-separated-split-values");


// Helper routines  ///////////////////////////////////////////////////////////

/// User-defined information for the importance function
typedef std::tuple<string,             // adhoc/composition function expression
				   ImportanceValue,    // user-defined min value
				   ImportanceValue,    // user-defined max value
				   ImportanceValue>    // user-defined neutral element
	UserDefImpFun;


/// Parse ad hoc ImportanceFunction specification details:
/// an user-defined algebraic expression, and optionally also
/// the minimum and maximum value the ImportanceFunction can take.
UserDefImpFun
parse_ifun_details(const std::string& details)
{
	ImportanceValue min(0u), max(0u), neutralElement(0u);
	char* err(nullptr);

	// Divide fields (split by semicolons)
	auto strValues = split(details, ';');
	for (auto& str: strValues) {
		// erase unescaped quotation marks
		delete_substring(str, "\"");
		delete_substring(str, "\'");
	}

	// The algebraic expression must be defined, though it's not interpreted here
	assert(strValues.size() > 0ul);

	// The extreme values and neutral element are optional
	if (strValues.size() > 1ul) {
		min = std::strtoul(strValues[1].data(), &err, 10);
		if (nullptr != err && err[0] != '\0') {
			// Mimic TCLAP's 'parsing error' message style
			std::cerr << "PARSE ERROR: When parsing the algebraic expression \""
					  << details << "\"\n";
			std::cerr << "             After the first semicolon, the function's "
					  << "min value should've been provided.\n\n";
			return UserDefImpFun();
		}
	}
	if (strValues.size() > 2ul) {
		max = std::strtoul(strValues[2].data(), &err, 10);
		if (nullptr != err && err[0] != '\0') {
			// Mimic TCLAP's 'parsing error' message style
			std::cerr << "PARSE ERROR: When parsing the algebraic expression \""
					  << details << "\"\n";
			std::cerr << "             After the second semicolon, the function's "
					  << "max value should've been provided.\n\n";
			return UserDefImpFun();
		}
	}
	if (strValues.size() > 3ul) {
		neutralElement = std::strtoul(strValues[3].data(), &err, 10);
		if (nullptr != err && err[0] != '\0') {
			// Mimic TCLAP's 'parsing error' message style
			std::cerr << "PARSE ERROR: When parsing the algebraic expression \""
					  << details << "\"\n";
			std::cerr << "             After the third semicolon, the function's "
					  << "neutral element should've been provided.\n\n";
			return UserDefImpFun();
		}
	}

	assert(min <= max);
	return std::make_tuple(strValues[0], min, max, neutralElement);
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

//	} else if (ifunFlatCoupled.isSet()) {
//		new(&impFunSpec) fig::ImpFunSpec("concrete_coupled", "flat");
//
//	} else if (ifunFlatSplit.isSet()) {
//		new(&impFunSpec) fig::ImpFunSpec("concrete_split", "flat");

	} else if (ifunAdhoc.isSet()) {
		const auto details = parse_ifun_details(ifunAdhoc.getValue());
		if (std::get<0>(details).empty())
			return false;  // no expression? something went wrong
		new(&impFunSpec) fig::ImpFunSpec("algebraic", "adhoc",
										 std::get<0>(details),   // expr
										 "",                     // postProc
										 std::get<1>(details),   // min
										 std::get<2>(details));  // max

//	} else if (ifunAdhocCoupled.isSet()) {
//		const auto details = parse_ifun_details(ifunAdhocCoupled.getValue());
//		if (std::get<0>(details).empty())
//			return false;  // no expression? something went wrong
//		new(&impFunSpec) fig::ImpFunSpec("concrete_coupled", "adhoc",
//										 std::get<0>(details),   // expr
//										 std::get<1>(details),   // min
//										 std::get<2>(details));  // max

	} else if (ifunAutoMonolithic.isSet()) {
		new(&impFunSpec) fig::ImpFunSpec("concrete_coupled", "auto", "", "");

	} else if (ifunAutoMonolithicExponential.isSet()) {
		new(&impFunSpec) fig::ImpFunSpec("concrete_coupled", "auto", "", "exp");

	} else if (ifunAutoCompositional.isSet() ||
			   ifunAutoCompositionalExponential.isSet()) {
		std::string postProc;
		UserDefImpFun details;
		if (ifunAutoCompositional.isSet()) {
			postProc = "";
			details = parse_ifun_details(ifunAutoCompositional.getValue());
		} else {
			postProc = "exp";
			details = parse_ifun_details(ifunAutoCompositionalExponential.getValue());
		}
		if (std::get<0>(details).empty())
			return false;  // no expression? something went wrong
		new(&impFunSpec) fig::ImpFunSpec("concrete_split", "auto",
										 std::get<0>(details),   // expr
										 postProc,
										 std::get<1>(details),   // min
										 std::get<2>(details),   // max
										 std::get<3>(details));  // neutral elem
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
		for (string timeCrit: timeCriteria.getValue()) {
			char *err(nullptr), timeUnit(timeCrit.back());
			if (!std::isalpha(timeUnit))
				timeUnit = 's';  // by default interpret seconds
			else
				timeCrit.resize(timeCrit.length()-1);
			const size_t FACTOR(timeUnit == 's' ? 1ul     :
								timeUnit == 'm' ? 60ul    :
								timeUnit == 'h' ? 3600ul  :
								timeUnit == 'd' ? 86400ul : 0ul);
			const size_t timeLen = std::strtoul(timeCrit.data(), &err, 10);
			assert (nullptr == err || err[0] != '\0');
			stopCond.add_time_budget(timeLen * FACTOR);
		}
		estBounds.emplace(estBounds.begin(), stopCond);
	}
	return true;
}


/// Check for global timeout specification and set it for simulations
/// time-bounding if found.
/// @return Whether the information could be successfully retrieved
bool
get_global_timeout()
{
	if (globalTimeout.isSet()) {
		string timeout(globalTimeout.getValue());
		char *err(nullptr), timeUnit(timeout.back());
		if (!std::isalpha(timeUnit))
			timeUnit = 's';  // by default interpret seconds
		else
			timeout.resize(timeout.length()-1);
		const size_t FACTOR(timeUnit == 's' ? 1ul     :
							timeUnit == 'm' ? 60ul    :
							timeUnit == 'h' ? 3600ul  :
							timeUnit == 'd' ? 86400ul : 0ul);
		const size_t timeLen = std::strtoul(timeout.data(), &err, 10);
		assert (nullptr == err || err[0] != '\0');
		globalTO = std::chrono::seconds(timeLen * FACTOR);
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
		char* err(nullptr);
		for (auto strValue: split(splittings_.getValue(), ',')) {
			unsigned value = std::strtoul(strValue.data(), &err, 10);
			if (nullptr == err || err[0] == '\0') {
				splittings.emplace(value);
			} else {
				// Mimic TCLAP's 'parsing error' message style
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
	// Called with no arguments? Print briefest help and exit gracefully
	if (argc==1) {
		// Mimic TCLAP's messages style
		std::cerr << "For complete USAGE and HELP type:\n";
		std::cerr << "   " << argv[0] << " --help\n\n";
		exit(EXIT_SUCCESS);
	}

	try {
		// Add all defined arguments and options to TCLAP's command line parser
		cmd_.add(modelFile_);
		cmd_.add(propertiesFile_);
		cmd_.add(engineName_);
		cmd_.add(thrTechnique_);
		cmd_.orAdd(stopCondSpecs);
		cmd_.xorAdd(impFunSpecs);
		cmd_.add(globalTimeout);
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
		if (!get_global_timeout()) {
			std::cerr << "ERROR: something failed while parsing the global ";
			std::cerr << "timeout specification.\n\n";
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

