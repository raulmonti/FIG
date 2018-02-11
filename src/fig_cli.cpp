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
#include <regex>
#include <string>
#include <fstream>
#include <algorithm>  // std::all_of()
// External code
#include <CmdLine.h>
#include <ValueArg.h>
#include <SwitchArg.h>
#include <ValuesConstraint.h>
#include <UnlabeledValueArg.h>
// FIG
#include <fig_cli.h>
#include <FigLog.h>
#include <FigConfig.h>
#include <FigException.h>
#include <Clock.h>
#include <ModelSuite.h>
#include <ImportanceFunctionConcrete.h>  // interpret_post_processing()
#include <MultiDoubleArg.h>
#include <NumericConstraint.h>
#include <TimeConstraint.h>
#include <FigVersionVisitor.h>


using namespace TCLAP;
using std::to_string;
using std::string;
using fig::figTechLog;


namespace fig_cli  // // // // // // // // // // // // // // // // // // // //
{

// Objects offered to configure the operation of FIG  /////////////////////////

string modelFile;
string propertiesFile;
string engineName;
fig::JaniTranny janiSpec;
fig::ImpFunSpec impFunSpec("no_name", "no_strategy");
string thrTechnique;
std::set< unsigned > globalEfforts;
std::list< fig::StoppingConditions > estBounds;
std::chrono::seconds simsTimeout;
string rngType;
size_t rngSeed;
bool forceOperation;
bool confluenceCheck;
double failProbDFT;

} // namespace fig_cli   // // // // // // // // // // // // // // // // // //




namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

using namespace fig_cli;
using fig::ImportanceValue;


/// Short version message, requested with --version
const std::string versionStrShort("FIG tool " + std::string(fig_VERSION_STR)
								 + " (" + std::string(fig_BUILD_TYPE) + ")");

/// Long version message, requested with -v/--version-full
const std::string versionStrLong(
	"FIG tool version " + std::string(fig_VERSION_STR) + "\n\n"
	"Build:       " + std::string(fig_CURRENT_BUILD) + "\n"
	"Compiler:    " + std::string(fig_COMPILER_USED) + "\n"
	"Default RNG: " + (
		(0 == std::strncmp("mt64", fig::Clock::DEFAULT_RNG,  6ul))
			? std::string("C++ STL's 64-bit Mersenne-Twister (") : (
		(0 == std::strncmp("pcg32", fig::Clock::DEFAULT_RNG, 6ul))
			? std::string("PCG-family 32-bit generator (")       : (
		(0 == std::strncmp("pcg64", fig::Clock::DEFAULT_RNG, 6ul))
			? std::string("PCG-family 64-bit generator (")       : (
			std::string("Unknown! (") ) ) ) )
#ifdef RANDOM_RNG_SEED
		+ "randomized seeding"
#elif defined PCG_RNG
		+ "seed: " + std::to_string(0xCAFEF00DD15EA5E5ull)  // PCG default seed
#else
		+ "seed: " + std::to_string(std::mt19937_64::default_seed)
#endif
	+ ")\n");


// TCLAP parameter holders and stuff  /////////////////////////////////////////

CmdLine cmd_("\nSample usage:\n"
			 "~$ fig models/tandem.{sa,pp} --amono --stop-time 5m\n"
			 "Use an automatically computed \"monolithic\" importance function "
             "built on the global state space of the model, running a 5 minutes "
			 "estimation which will employ the RESTART simulation engine "
             "(default) for global effort (aka splitting for RESTART) == 2 "
             "(default) and the hybrid thresholds building technique "
             "i.e. \"hyb\" (default)\n"
             "~$ fig models/tandem.{sa,pp} --flat -e nosplit            \\"
             "       --stop-conf 0.8 0.4 --timeout 1h\n"
			 "Use a flat importance function to perform a standard Monte Carlo "
			 "simulation (i.e. no splitting), which will run until either "
			 "the relative precision achieved for an 80% confidence interval "
			 "equals 40% of the value estimated for each property, or 1 hour "
			 "of wall clock time has passed, whichever happens first.\n"
             "~$ fig models/tandem.{sa,pp} --adhoc \"10*q2+q1\" -t ams    \\"
             "            -e sfe --stop-conf 0.9 0.2\n"
			 "Use the importance function \"10*q2+q1\" defined ad hoc by the "
             "user, with the (simple) Fixed Effort simulation engine for global "
             "effort 1024 (default for that engine), employing the Adaptive "
             "Multilevel Splitting thresholds building technique (\"ams\"), "
             "estimating until the relative precision achieved "
             "for a 90% confidence interval equals 20% of the value "
             "estimated for each property.\n"
             "~$ fig models/tandem.sa --adhoc \"10*q2+q1\" -t es          \\"
             "            -e restart --stop-conf 0.9 0.2\n"
             "Same as before but using RESTART with the Expected Success "
             "thresholds building technique (\"es\"), which automatically "
             "selects the best effort for each level and thus requires no "
             "specification of global effort values.\n"
             "~$ fig models/tandem.sa --acomp \"+\" -t hyb                \\"
			 "       --stop-conf .8 .4 --stop-time 1h --stop-conf .95 .1       \\"
             "       --global-effort 2,3,5,9,15 -e restart --timeout 30m\n"
			 "Use an automatically computed \"compositional\" importance "
			 "function, built modularly on every module's state space, "
             "simulating with the RESTART engine for the given global effort "
             "values (aka splitting values for RESTART), using the hybrid "
             "thresholds building technique (\"hyb\"), estimating the value "
             "of each property for this configuration and for each one "
             "of the three stopping conditions, or until the wall-clock "
             "timeout is reached in each case.",
			 ' ', versionStrShort);

// IOSA model file path
UnlabeledValueArg<string> modelFile_(
	"modelFile",
	"Path to the file with the IOSA/JANI model",
	true, "",
	"modelFile");

// Properties file path
UnlabeledValueArg<string> propertiesFile_(
	"propertiesFile",
	"Path to the file with the properties to estimate",
	false, "",
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

// Translation from/to JANI specification format
SwitchArg JANIimport_(
	"", "from-jani",
	"Don't estimate; create IOSA model file from JANI-spec model file.");
SwitchArg JANIexport_(
	"", "to-jani",
	"Don't estimate; create JANI-spec model file from IOSA model file.");

// Importance function specifications
SwitchArg ifunFlat(
	"", "flat",
	"Use a flat importance function, i.e. consider all states in the model "
	"equally important. Information is kept symbolically as an algebraic "
	"expression, thus using very little memory. Notice the flat function is "
	"only compatible with the no-split simulation engine.");
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
ValueArg<string> ifunAutoCompositional(
	"", "acomp",
	"Use an automatically computed \"compositional\" importance function, "
	"i.e. store information separately for each module. This stores in "
	"memory one vector per module, and then uses the algebraic expression "
	"provided to \"compose\" the global importance from these.",
	false, "",
	"composition_fun");
std::vector< Arg* > impFunSpecs = {
	&ifunFlat,
	&ifunAdhoc,
	// &ifunAdhocCoupled
	&ifunAutoMonolithic,
	&ifunAutoCompositional,
};

// Importance function post-processing
ValuesConstraint<string> postProcConstraints(
	fig::ModelSuite::available_importance_post_processings());
NumericConstraint<float> postProcArg(
	[](const float&){ return true; }, "value");
MultiDoubleArg< string, float > impPostProc(
	"", "post-process",
	"Specify a post-processing to apply to the importance computed, e.g. "
	"\"--post-process exp 2.0\" to exponentiate all values using '2' as "
	"the base. Only applicable to --amono and --acomp importance functions.",
	false, &postProcConstraints, &postProcArg);

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
	false, &ccConstraint, &precConstraint);
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

// Simulations timeout
ValueArg<string> timeout(
	"", "timeout",
	"Running time limit: if set, any simulation will be soft-interrupted "
	"after running for this (wall clock) time. If the stopping condition for "
	"a simulation is also time based, the lowest of the two values will apply.",
	false, "", &timeDurationConstraint);

// Splitting values to test
ValueArg<string> globalEfforts_(
    "g", "global-effort",
    "Global effort values to use with Importance Splitting simulation engines; "
    "this must be a comma-separated list of integral values greater than 1",
	false, "2",
    "comma-separated-integral-values");

// RNG to use
ValuesConstraint<string> RNGConstraints(
	fig::ModelSuite::available_RNGs());
ValueArg<string> rngType_(
	"r", "rng",
	"Specify the pseudo Random Number Generator (aka RNG) to use for "
	"sampling the time values of clocks",
	false, fig::Clock::DEFAULT_RNG, &RNGConstraints);

// User-specified seed for RNG
ValueArg<string> rngSeed_(
	"", "rng-seed",
	"Specify the (numeric) seed of the RNG; may also specify \"random\" "
	"to use randomized seeding in all simulation runs",
	false, "", "random/<digit>+");

// Ignore not-IOSA-compliance warnings
SwitchArg forceOperation_(
	"f", "force",
	"Force FIG operation disregarding any warning of the model not being IOSA-"
	"compliant. Depending on the user command, this may force estimation of "
	"the properties values, or translation to the JANI Specification format.");

// Confluence-checking request
SwitchArg confluenceCheck_(
        "c", "confluence",
        "Run algorithm to check confluence of committed actions.");

// For models that come from a Dynamic Faul Tree specification (e.g. GALILEO),
// the user may specify the the probability of fail before repair,
// e.g. of increasing one lvl of importance
ValueArg<double> failProbDFT_(
    "", "dft",
    "For models that come from Dynamic Fault Tree descriptions, specify "
    "a *rough and unified* probability of observing a fail before a repair."
    "Call with value 0.0 for an automatic choice.",
    false, -1.0, "probFail ∈ (0,1)");


// Helper routines  ///////////////////////////////////////////////////////////

/// Add parsing for long version info requests,
/// just like TCLAP's default version printing.
/// @note The switches to request full version info are "-v" and "--version-full"
void
add_full_version_parsing(CmdLine& cl)
{
	auto v = new FigVersionVisitor(fig::figMainLog, versionStrLong);
	SwitchArg* vers = new SwitchArg("v","version-full", "Displays full "
									"version information and exits.",
									false, v);
	cl.add( vers );

//	// These member functions are protected so we can't use them
//	cl.deleteOnExit(vers);
//	cl.deleteOnExit(v);
//	// To avoid the leak we could use smart pointers like below,
//	// but TCLAP's CmdLine can't work with them (piece of filthy shit)
//	auto v = std::make_shared<FigVersionVisitor>(fig::figMainLog, versionStrLong);
//	auto vers(std::make_shared<SwitchArg>(
//				"v", "version-full",
//				"Displays full version information and exits.",
//				false, v));
}

/// Check for any JANI specification parsed from the command line into the
/// TCLAP holders. Use it to fill in the global information offered to FIG
/// for JANI interaction (viz: 'janiSpec')
/// @return Whether the information could be successfully retrieved
bool
get_jani_spec()
{
	using std::regex;
	using std::regex_match;
	using std::regex_replace;
	using std::make_pair;

	// Determine JANI interaction
	bool fromJANI(JANIimport_.getValue()), toJANI(JANIexport_.getValue());
	janiSpec.translateOnly = fromJANI || toJANI;
	if (janiSpec.translateOnly) {
		janiSpec.janiInteraction = true;
		janiSpec.translateDirection = fromJANI ? fig::JaniTranny::FROM_JANI
											   : fig::JaniTranny::TO_JANI;
	} else if (regex_match(modelFile_.getValue(), regex("^.*\\.jani$"))) {
		// .jani extension: suspicious enough to assume JANI
		janiSpec.janiInteraction = true;
		janiSpec.translateDirection = fig::JaniTranny::FROM_JANI;
	} else {
		// Superficial check for JANI-like content in the input model file
		std::ifstream modelFile(modelFile_.getValue());
		if (!modelFile.good()) {
			// something fishy's going on but none of our business
			janiSpec.janiInteraction = false;
			return true;
		}
		const size_t NLINES(100ul);
		size_t linesRead(0ul);
		string line;
		std::vector< std::pair<regex,bool> > matches(3);
		matches[0] = make_pair(regex("\r*.*jani-version.*\n*\r*"), false);
		matches[1] = make_pair(regex("\r*.*name.*\n*\r*"), false);
		matches[2] = make_pair(regex("\r*.*type.*\n*\r*"), false);
		auto check_match = [] (const std::pair<regex,bool>& p)
						   { return p.second; };
		while (std::getline(modelFile, line) && linesRead++ < NLINES) {
			for (auto& match: matches)
				if (regex_match(line, match.first))
					match.second = true;
			if (std::all_of(begin(matches), end(matches), check_match))
				break;  // all matches were found
		}
		// Does it look JANI enough?
		janiSpec.janiInteraction = std::all_of(begin(matches), end(matches), check_match);
		if (janiSpec.janiInteraction)
			janiSpec.translateDirection = fig::JaniTranny::FROM_JANI;
		else if (matches[0].second) {  // found "jani-version" !?
			figTechLog << "JANI [ERROR] failed parsing JANI input model file\n";
			return false;
		}
	}

	// If there's JANI interaction, specify details in janiSpec
	if (janiSpec.translateDirection == fig::JaniTranny::FROM_JANI) {
		auto janiFile = modelFile_.getValue();
		const regex janiExt("(^.*)\\.jani$");
		janiSpec.modelFileJANI = janiFile;
		janiSpec.propsFileIOSA = "";
		janiSpec.modelFileIOSA = (regex_match(janiFile, janiExt))
								 ? regex_replace(janiFile, janiExt, "$1.sa")
								 : janiFile + ".sa";
	} else if (janiSpec.translateDirection == fig::JaniTranny::TO_JANI) {
		auto iosaFile = modelFile_.getValue();
		const regex iosaExt("(^.*)\\.(iosa|sa)$");
		janiSpec.modelFileIOSA = iosaFile;
		janiSpec.propsFileIOSA = propertiesFile_.getValue();
		janiSpec.modelFileJANI = (regex_match(iosaFile, iosaExt))
								 ? regex_replace(iosaFile, iosaExt, "$1.jani")
								 : iosaFile + ".jani";
		if (regex_match(iosaFile, iosaExt))
			janiSpec.modelFileJANI = regex_replace(iosaFile, iosaExt, "$1.jani");
		else
			janiSpec.modelFileJANI = iosaFile + ".jani";
	}

	return true;
}


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
			figTechLog << "PARSE [ERROR] When parsing the algebraic expression \""
					  << details << "\"\n";
			figTechLog << "             After the first semicolon, the function's "
					  << "min value should've been provided.\n\n";
			return UserDefImpFun();
		}
	}
	if (strValues.size() > 2ul) {
		max = std::strtoul(strValues[2].data(), &err, 10);
		if (nullptr != err && err[0] != '\0') {
			// Mimic TCLAP's 'parsing error' message style
			figTechLog << "PARSE [ERROR] When parsing the algebraic expression \""
					  << details << "\"\n";
			figTechLog << "             After the second semicolon, the function's "
					  << "max value should've been provided.\n\n";
			return UserDefImpFun();
		}
	}
	if (strValues.size() > 3ul) {
		neutralElement = std::strtoul(strValues[3].data(), &err, 10);
		if (nullptr != err && err[0] != '\0') {
			// Mimic TCLAP's 'parsing error' message style
			figTechLog << "PARSE [ERROR] When parsing the algebraic expression \""
					  << details << "\"\n";
			figTechLog << "             After the third semicolon, the function's "
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
	fig::PostProcessing postProc;
	if (impPostProc.isSet()) {
		if (impPostProc.getValues().size() > 1) {
			// Mimic TCLAP's 'parsing error' message style
			figTechLog << "PARSE [ERROR] Argument: (" << impPostProc.getName()
					  << ")\n             Can only be defined once!\n";
			return false;
		} else {
			assert(impPostProc.getValues().size() == 1ul);
			postProc = fig::ImportanceFunctionConcrete::interpret_post_processing(
						   impPostProc.getValues()[0ul]);
		}
	}

	if (ifunFlat.isSet()) {
		new(&impFunSpec) fig::ImpFunSpec("algebraic", "flat");

	} else if (ifunAdhoc.isSet()) {
		const auto details = parse_ifun_details(ifunAdhoc.getValue());
		if (std::get<0>(details).empty())
			return false;  // no expression? something went wrong
		new(&impFunSpec) fig::ImpFunSpec("algebraic", "adhoc",
										 std::get<0>(details),   // expr
										 postProc,               // post-process
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
		new(&impFunSpec) fig::ImpFunSpec("concrete_coupled", "auto", "", postProc);

	} else if (ifunAutoCompositional.isSet()) {
		UserDefImpFun details = parse_ifun_details(ifunAutoCompositional.getValue());
		if (std::get<0>(details).empty())
			return false;  // no expression? something went wrong
		new(&impFunSpec) fig::ImpFunSpec("concrete_split", "auto",
										 std::get<0>(details),   // expr
										 postProc,               // post-process
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
			assert (nullptr == err || err[0] == '\0');
			stopCond.add_time_budget(timeLen * FACTOR);
		}
		estBounds.emplace(estBounds.begin(), stopCond);
	}
	return true;
}


/// Check for timeout specification and set it for simulations
/// time-bounding if found.
/// @return Whether the information could be successfully retrieved
bool
get_timeout()
{
	if (timeout.isSet()) {
		string TO(timeout.getValue());
		char *err(nullptr), timeUnit(TO.back());
		if (!std::isalpha(timeUnit))
			timeUnit = 's';  // by default interpret seconds
		else
			TO.resize(TO.length()-1);
		const size_t FACTOR(timeUnit == 's' ? 1ul     :
							timeUnit == 'm' ? 60ul    :
							timeUnit == 'h' ? 3600ul  :
							timeUnit == 'd' ? 86400ul : 0ul);
		const size_t timeLen = std::strtoul(TO.data(), &err, 10);
		assert (nullptr == err || err[0] == '\0');
		simsTimeout = std::chrono::seconds(timeLen * FACTOR);
	}
	return true;
}


/// Check for customizations in the RNG mechanisms.
/// @return Whether the information could be successfully retrieved
bool
get_rng_specs()
{
	rngType = rngType_.getValue();
	if (rngSeed_.isSet()) {
		const string& seed(rngSeed_.getValue());
		if (std::isalpha(seed.back())) {
			if (0 == seed.compare("random"))
				rngSeed = 0ul;
			else {
				figTechLog << "[ERROR] Invalid RNG seed: " << seed << std::endl;
				return false;
			}
		} else {
			char *err(nullptr);
			rngSeed = std::strtoul(seed.data(), &err, 10);
			if (nullptr != err && err[0] != '\0') {
				figTechLog << "[ERROR] Invalid RNG seed: " << seed << std::endl;
				return false;
			} else if (0ul == rngSeed) {
				figTechLog << "[WARNING] RNG seed == 0 is interpreted as "
				           << "randomized seeding. Is that what you want?\n\n";
			}
		}
	} else if (fig::Clock::rng_seed_is_random()) {
		rngSeed = 0ul;
	} else {
		rngSeed = fig::Clock::rng_seed();
	}
	return true;
}


/// Check the global effort values specifications parsed from the command line
/// into the TCLAP holders. Use them to fill in the global information offered
/// to FIG for estimation (viz: the 'globalEffort' set)
/// @return Whether the information could be successfully retrieved
bool
get_global_effort_values()
{
	if (globalEfforts_.isSet()) {
		char* err(nullptr);
		for (auto strValue: split(globalEfforts_.getValue(), ',')) {
			unsigned value = std::strtoul(strValue.data(), &err, 10);
			if (nullptr == err || err[0] == '\0') {
				globalEfforts.emplace(value);
			} else {
				// Mimic TCLAP's 'parsing error' message style
				figTechLog << "PARSE [ERROR] Argument: (--"
				          << globalEfforts_.getName() << ")\n";
				figTechLog << "             Invalid value given \""
						  << err << "\"\n\n";
				return false;
			}
		}
	} else {
		globalEfforts.emplace(0u);  // per-engine default effort selection
	}
	return true;
}

} // namespace  // // // // // // // // // // // // // // // // // // // // //




namespace fig_cli  // // // // // // // // // // // // // // // // // // // //
{

// Main parsing routine  //////////////////////////////////////////////////////

bool
parse_arguments(const int& argc, const char** argv, bool fatalError)
{
	// Called with no arguments? Print briefest help and exit gracefully
	if (argc==1) {
		// Mimic TCLAP's messages style
		figTechLog << "For complete USAGE and HELP type:\n";
		figTechLog << "   " << argv[0] << " --help\n\n";
		exit(EXIT_SUCCESS);
	}

	try {
		// Add all defined arguments and options to TCLAP's command line parser
		add_full_version_parsing(cmd_);
		cmd_.add(modelFile_);
		cmd_.add(propertiesFile_);
		auto ifun_or_JANI(impFunSpecs);  // jani spec XOR ifun
		ifun_or_JANI.emplace_back(&JANIimport_);
		ifun_or_JANI.emplace_back(&JANIexport_);
		cmd_.xorAdd(ifun_or_JANI);
		cmd_.add(engineName_);
		cmd_.add(thrTechnique_);
		cmd_.add(confidenceCriteria);
		cmd_.add(timeCriteria);
		cmd_.add(impPostProc);
		cmd_.add(timeout);
		cmd_.add(globalEfforts_);
		cmd_.add(rngType_);
		cmd_.add(rngSeed_);
		cmd_.add(forceOperation_);
		cmd_.add(confluenceCheck_);
		cmd_.add(failProbDFT_);

		// Parse the command line input
		cmd_.parse(argc, argv);

		// Fill the globally offered objects
		modelFile       = modelFile_.getValue();
		propertiesFile  = propertiesFile_.getValue();
		engineName      = engineName_.getValue();
		thrTechnique    = thrTechnique_.getValue();
		forceOperation  = forceOperation_.getValue();
		confluenceCheck = confluenceCheck_.getValue();
		failProbDFT     = failProbDFT_.getValue();
		if (!get_jani_spec()) {
			figTechLog << "[ERROR] Failed parsing the JANI-spec commands.\n\n";
			figTechLog << "For complete USAGE and HELP type:\n";
			figTechLog << "   " << argv[0] << " --help\n\n";
			goto exit_with_failure;
		} else if (janiSpec.janiInteraction && janiSpec.translateOnly) {
			// avoid further parsing
			goto exit_with_success;
		}
		if (!get_global_effort_values()) {
			figTechLog << "[ERROR] Global effort values must be specified as a "
			             "comma-sperated list of positive integral values. "
			             "There should be no spaces in this list.\n\n";
			figTechLog << "For complete USAGE and HELP type:\n";
			figTechLog << "   " << argv[0] << " --help\n\n";
			goto exit_with_failure;
		}
		if (!get_ifun_specification()) {
			figTechLog << "[ERROR] Must specify an importance function.\n\n";
			figTechLog << "For complete USAGE and HELP type:\n";
			figTechLog << "   " << argv[0] << " --help\n\n";
			goto exit_with_failure;
		} else if ("flat" == impFunSpec.strategy) {
			// make amendments to set the only possible "flat" configuration
			engineName = "nosplit";
			if (globalEfforts.size() > 1ul)
				figTechLog << "[WARNING] Global effort is incompatible with a "
				             "\"flat\" importance function, ignoring values.\n";
			std::set< unsigned >({1u}).swap(globalEfforts);
		} else if (0.0 <= failProbDFT
		           && (impFunSpec.name.find("split") == std::string::npos
		               || "auto" != impFunSpec.strategy)) {
			figTechLog << "[ERROR] Special DFT processing using importance "
			              "splitting requires the compositional importance "
			              "function (i.e. acomp).\n\n";
			figTechLog << "For complete USAGE and HELP type:\n";
			figTechLog << "   " << argv[0] << " --help\n\n";
			goto exit_with_failure;
		}
		if (!get_stopping_conditions()) {
			figTechLog << "[ERROR] Must specify at least one stopping condition ";
			figTechLog << "(--stop-conf|--stop-time).\n\n";
			figTechLog << "For complete USAGE and HELP type:\n";
			figTechLog << "   " << argv[0] << " --help\n\n";
			goto exit_with_failure;
		}
		if (!get_timeout()) {
			figTechLog << "[ERROR] Something failed while parsing the ";
			figTechLog << "timeout specification.\n\n";
			figTechLog << "For complete USAGE and HELP type:\n";
			figTechLog << "   " << argv[0] << " --help\n\n";
			goto exit_with_failure;
		}
		if (!get_rng_specs()) {
			figTechLog << "[ERROR] Something failed while parsing the ";
			figTechLog << "RNG specifications.\n\n";
			figTechLog << "For complete USAGE and HELP type:\n";
			figTechLog << "   " << argv[0] << " --help\n\n";
			goto exit_with_failure;
		}

	} catch (ArgException& e) {
		throw_FigException(std::string("command line parsing failed "
						   "unexpectedly: ").append(e.what()));
	}

	exit_with_success:
		return true;

	exit_with_failure:
		if (fatalError)
			exit(EXIT_FAILURE);
		else
			return false;
}

} // namespace fig_cli  // // // // // // // // // // // // // // // // // // //

