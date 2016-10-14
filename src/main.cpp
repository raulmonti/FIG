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


// C++
#include <iostream>
#include <iomanip>  // std::setprecision()
#include <fstream>
#include <sstream>
#include <string>
#include <list>
// C
#include <omp.h>
#include <cassert>
#include <sys/stat.h>
// FIG
#include <fig.h>
#include <fig_cli.h>
#include <string_utils.h>
#include <ModelBuilder.h>
#include <ModelReductor.h>
#include <ModelPrinter.h>
#include <ModelVerifier.h>
#include <ConfluenceChecker.h>
#include <JANI_translator.h>
#include <ImportanceFunction.h>


//  Helper functions headers  //////////////////////////////////////////////////

static bool print_intro(const int& argc, const char** argv);
static bool file_exists(const std::string& filepath);
static void interact_with_JANI();
static void compile_model(bool modelAlreadyBuilt);


//  Configuration of the estimation run  ///////////////////////////////////////

using fig_cli::confluenceCheck;
using fig_cli::forceOperation;
using fig_cli::janiSpec;
using fig_cli::modelFile;
using fig_cli::propertiesFile;
using fig_cli::engineName;
using fig_cli::impFunSpec;
using fig_cli::thrTechnique;
using fig_cli::splittings;
using fig_cli::estBounds;
using fig_cli::simsTimeout;


//  Main stuff  ////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	auto log(fig::ModelSuite::log);
	auto tech_log(fig::ModelSuite::tech_log);
	auto const_argv(const_cast<const char**>(argv));
	const std::string FIG_ERROR("[ERROR] FIG failed to");
	shared_ptr<ModelAST> modelAST = nullptr;

	// "Greetings, human!" and command line parsing
	try {
		const bool versionQuery = print_intro(argc, const_argv);
		fig_cli::parse_arguments(argc, const_argv);  // exit on error
		if (versionQuery)
			goto exit_point;  // we're done
	} catch (fig::FigException& e) {
		log(FIG_ERROR + " parse the command line.\n\n");
		tech_log("Error message: " + e.msg() + "\n");
		exit(EXIT_FAILURE);
	} catch (std::exception& e) {
		log("UNEXPECTED " + FIG_ERROR + " parse the command line.\n\n");
		tech_log(std::string("Error message: ") + e.what() + "\n");
		exit(EXIT_FAILURE);
	}

	// Check for JANI interaction directives
	try {
		if (janiSpec.janiInteraction) {
			double start = omp_get_wtime();
			interact_with_JANI();
			std::stringstream ss; ss << "JANI translation time: " << std::fixed;
			ss << std::setprecision(2) << omp_get_wtime()-start << " s\n\n";
			tech_log(ss.str());
			if (janiSpec.translateOnly)
				goto exit_point;  // we're done
		}
	} catch (fig::FigException& e) {
		log(FIG_ERROR + " communicate with the JANI Specification format.\n\n");
		tech_log("Error message: " + e.msg() + "\n");
		exit(EXIT_FAILURE);
	} catch (std::exception& e) {
		log("UNEXPECTED " + FIG_ERROR + " communicate with the JANI Specification format.\n\n");
		tech_log(std::string("Error message: ") + e.what() + "\n");
		exit(EXIT_FAILURE);
	}

	// Compile IOSA model and properties to check
	try {
		double start = omp_get_wtime();
        compile_model(janiSpec.janiInteraction);  // JANI interaction => model was built
		std::stringstream ss; ss << "Model building time: " << std::fixed;
		ss << std::setprecision(2) << omp_get_wtime()-start << " s\n\n";
		tech_log(ss.str());
	} catch (fig::FigException& e) {
		log(FIG_ERROR + " compile the model/properties file.\n\n");
		tech_log("Error message: " + e.msg() + "\n");
		exit(EXIT_FAILURE);
	} catch (std::exception& e) {
		log("UNEXPECTED " + FIG_ERROR + " compile the model/properties file.\n\n");
		tech_log(std::string("Error message: ") + e.what() + "\n");
		exit(EXIT_FAILURE);
	}

	// Estimate using requested configuration
	try {
		auto model = fig::ModelSuite::get_instance();
		model.set_timeout(simsTimeout);
		model.process_batch(engineName,
							impFunSpec,
							thrTechnique,
							estBounds,
							splittings);
		model.release_resources();
	} catch (fig::FigException& e) {
		log(FIG_ERROR + " perform estimations.\n\n");
		tech_log("Error message: " + e.msg() + "\n");
		exit(EXIT_FAILURE);
	} catch (std::exception& e) {
		log("UNEXPECTED " + FIG_ERROR + " perform estimations.\n\n");
		tech_log(std::string("Error message: ") + e.what() + "\n");
		exit(EXIT_FAILURE);
	}

	exit_point:
		return EXIT_SUCCESS;
}


//  Helper functions implementations  //////////////////////////////////////////

bool print_intro(const int& argc, const char** argv)
{
	auto main_log = fig::ModelSuite::main_log;
	auto tech_log = fig::ModelSuite::tech_log;
	using std::to_string;
	const std::time_t now = std::chrono::system_clock::to_time_t(
				std::chrono::system_clock::now());

	// First check if this is a version query and we should omit the greeting
	if (argc == 2 && (trim(argv[1]) == "-v" || trim(argv[1]) == "--version"))
		return true;

	// Print the big fat greeting the user deserves
	main_log("\n");
	main_log(" ~~~~~~~~~ \n");
	main_log("  · FIG ·  \n");
	main_log(" ~~~~~~~~~ \n");
	main_log("		   \n");
	main_log(" This is the Finite Improbability Generator.\n");
	main_log(" Version: " + std::string(fig_VERSION_STR) + "\n");
	main_log(" Build:   ");
	if (is_substring_ci(fig_CURRENT_BUILD, "release"))
		main_log("Release ");
	else
		main_log("Debug ");
#ifndef PCG_RNG
	main_log("(Mersenne-Twister RNG)\n");
#else
	main_log("(PCG family RNG)\n");
#endif
	main_log(" Authors: Budde, Carlos E.  <cbudde@famaf.unc.edu.ar>\n");
	main_log("          Monti, Raúl E.    <raulmonti88@gmail.com>\n");
	main_log("          Rodriguez, Leo M. <leonardomatiasrodriguez@gmail.com>\n");
	main_log("\n");

	// Print additional technical info if this is more than a query
	if (argc > 1 && trim(argv[1]) != "-h" && trim(argv[1]) != "--help") {
		tech_log(std::string("\nFIG tool invoked on ") + std::ctime(&now));
		tech_log("Build: " fig_CURRENT_BUILD "\n");
		tech_log("64-bit RNG: ");
#ifndef PCG_RNG
		tech_log("STL's Mersenne-Twister ");
#else
		tech_log("Builtin PCG ");
#endif
#ifndef RANDOM_RNG_SEED
		tech_log("(seed: " + std::to_string(fig::Clock::rng_seed()) + ")\n\n");
#else
		tech_log("(seeded from system's random device)\n\n");
#endif
		tech_log("Invocation command:");
		for (int i = 0 ; i < argc ; i++)
			tech_log(std::string(" ") + argv[i]);
		tech_log("\n\n");
	}

	return false;
}


bool file_exists(const std::string& filepath)
{
	struct stat buffer;
	return (stat(filepath.c_str(), &buffer) == 0);
}


void interact_with_JANI()
{
    auto log = fig::ModelSuite::main_log;
	fig::JaniTranslator translator;

	if (fig::JaniTranny::FROM_JANI == janiSpec.translateDirection) {
		log("Translating from JANI Specification format to IOSA model syntax\n");
		if (!file_exists(janiSpec.modelFileJANI)) {
			log(" *** Error: JANI-spec model file \""
				+ janiSpec.modelFileJANI +"\" not found! ***\n");
			throw_FigException("file with JANI model not found");
		} else if (janiSpec.translateOnly &&
		           !janiSpec.modelFileIOSA.empty() &&
		           file_exists(janiSpec.modelFileIOSA) &&
		           !forceOperation) {
			log(" *** Error: file \"" + janiSpec.modelFileIOSA +
			    "\" exists, can't write JANI translation there. ***\n");
			throw_FigException("can't overwrite file for IOSA translation "
			                   "(call with --force)");
		}
		translator.JANI_2_IOSA(janiSpec.modelFileJANI,
							   janiSpec.modelFileIOSA,
							   !janiSpec.translateOnly);

	} else if (fig::JaniTranny::TO_JANI == janiSpec.translateDirection) {
		log("Translating from IOSA model syntax to JANI Specification format\n");
		if (!file_exists(janiSpec.modelFileIOSA)) {
			log(" *** Error: IOSA model file \""
				+ janiSpec.modelFileIOSA +"\" not found! ***\n");
			throw_FigException("file with IOSA model not found");
		} else if (!janiSpec.propsFileIOSA.empty() &&
				!file_exists(janiSpec.propsFileIOSA)) {
			log(" *** Error: properties file \""
				+ janiSpec.propsFileIOSA + "\" not found! ***\n");
			throw_FigException("file with properties not found");
		} else if (!janiSpec.modelFileJANI.empty() &&
		           file_exists(janiSpec.modelFileJANI) &&
		           !forceOperation) {
			log(" *** Error: file \"" + janiSpec.modelFileJANI +
			    "\" exists, can't write IOSA translation there. ***\n");
			throw_FigException("can't overwrite file for JANI translation "
			                   "(call with --force)");
		}
		const bool checkIOSAcorrectness = !forceOperation;
		translator.IOSA_2_JANI(janiSpec.modelFileIOSA,
							   janiSpec.propsFileIOSA,
							   janiSpec.modelFileJANI,
							   checkIOSAcorrectness);

	} else {
		log("Ill-defined JANI-IOSA interaction -- Skipping translation\n");
	}
}


void compile_model(bool modelAlreadyBuilt)
{
	auto log = fig::ModelSuite::main_log;
	auto tech_log = fig::ModelSuite::tech_log;

	if (modelAlreadyBuilt) {
		// Parsing + model building already done during JANI interaction
		assert(ModelSuite::get_instance().sealed());
		tech_log("- Model successfully compiled during JANI translation\n");
		return;
	}

	// Check for required files
	log("Model file: " + modelFile + "\n");
	if (!file_exists(modelFile)) {
		log("[ERROR] File \"" + modelFile + "\" not found!\n");
		throw_FigException("file with model not found");
	}
	if (!propertiesFile.empty()) {
		log("Properties file: " + propertiesFile + "\n");
		if (!file_exists(propertiesFile)) {
			log("[ERROR] File \"" + propertiesFile + "\" not found!\n");
			throw_FigException("file with properties not found");
		}
	}

	// Build AST from files, viz. parse
	shared_ptr<ModelAST> modelAST(nullptr);
	modelAST = ModelAST::from_files(modelFile.c_str(), propertiesFile.c_str());
	if (nullptr == modelAST) {
		log("[ERROR] Failed to parse the model.\n");
		throw_FigException("failed parsing the model file");
	}

	// Debug print:
	// { ModelPrinter printer(std::cerr,true); modelAST->accept(printer); }

	// Check types
	ModelTC typechecker;
	modelAST->accept(typechecker);
	if (typechecker.has_errors()) {
		log(typechecker.get_messages());
		throw_FigException("type-check for the model failed");
	}
	tech_log("- Type-checking  succeeded\n");

    // Reduce expressions (errors when irreducible constants are found)
	ModelReductor reductor;
	modelAST->accept(reductor);
    if (reductor.has_errors()) {
        log(reductor.get_messages());
        throw_FigException("reduction of constant expressions failed");
    }
	tech_log("- Expr-reduction succeeded\n");

	// Check confluence if requested
    if (confluenceCheck) {
    	iosa::ConfluenceChecker confluence_verifier;
        modelAST->accept(confluence_verifier);
        if (confluence_verifier.has_errors()) {
            log(confluence_verifier.get_messages());
            tech_log("- Confluence-checking failed\n");
        } else {
            tech_log("- Confluence-checking succeeded\n");
        }
    }

	// Check IOSA correctness
	if (ModuleScope::modules_size_bounded_by(ModelVerifier::NTRANS_BOUND)) {
		ModelVerifier verifier;
		modelAST->accept(verifier);
		assert(!verifier.has_errors());
		if (verifier.has_warnings()) {
			log("\n[WARNING] IOSA-checking failed");
			tech_log(verifier.get_messages());
			if (!forceOperation) {
				log(" -- aborting\n");
				log("To force estimation disregarding ");
				log("IOSA errors call with \"--force\"");
				throw_FigException("iosa-check for the model failed");
			} else {
				log("\n");
			}
		}
		tech_log("- IOSA-checking  succeeded\n");
	} else {
		log("- IOSA-checking skipped: model is too big\n");
	}

	// Build model (i.e. populate ModelSuite)
	ModelBuilder builder;
	modelAST->accept(builder);
	if (builder.has_errors()) {
		log(builder.get_messages());
		throw_FigException("parser failed to build the model");
	}
	tech_log("- Model building succeeded\n");

	// Seal model
	auto& modelInstance = ModelSuite::get_instance();
	modelInstance.seal();
	if (!modelInstance.sealed()) {
		log("[ERROR] Failed to seal the model.\n");
		throw_FigException("parser failed sealing the model");
	}
	tech_log("- Model sealing  succeeded\n\n");

	log(std::string("Model") +
	    (propertiesFile.empty() ? (" file ") : (" and properties files "))
	    + "successfully compiled.\n\n");
}

