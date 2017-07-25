//==============================================================================
//
//  tests_common.h
//	
//	Copyleft 2017-
//	Authors:
//  * Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
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
#include <sys/stat.h>
// TESTS
#include <tests_definitions.h>


namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

// Check file accessibility
// https://stackhttps://stackoverflow.com/a/12774387overflow.com/a/12774387
bool file_exists(const std::string& filepath)
{
	struct stat buffer;
	return (stat(filepath.c_str(), &buffer) == 0);
}

} // namespace   // // // // // // // // // // // // // // // // // // // // //



namespace tests  // // // // // // // // // // // // // // // // // // // // //
{

/// Default IOSA model compilation
/// @return Whether the model file could be successfully compiled
/// @note Must be called from within a TEST_CASE
bool compile_model(const string& modelFilePath)
{
//	auto log = fig::ModelSuite::main_log;
//	auto tech_log = fig::ModelSuite::tech_log;
//
//	if (modelAlreadyBuilt) {
//		// Parsing + model building already done during JANI interaction
//		assert(ModelSuite::get_instance().sealed());
//		tech_log("- Model successfully compiled during JANI translation\n");
//		return;
//	}
//
//	// Check for required files
//	log("Model file: " + modelFile + "\n");
//	if (!file_exists(modelFile)) {
//		log("[ERROR] File \"" + modelFile + "\" not found!\n");
//		throw_FigException("file with model not found");
//	}
//	if (!propertiesFile.empty()) {
//		log("Properties file: " + propertiesFile + "\n");
//		if (!file_exists(propertiesFile)) {
//			log("[ERROR] File \"" + propertiesFile + "\" not found!\n");
//			throw_FigException("file with properties not found");
//		}
//	}


	REQUIRE( file_exists(modelFilePath) );


	/// @todo TODO: go on from here


//	// Parse model
//	shared_ptr<ModelAST> modelAST(nullptr);
//	modelAST = ModelAST::from_files(modelFile.c_str(), propertiesFile.c_str());
//	if (nullptr == modelAST) {
//		log("[ERROR] Failed to parse the model.\n");
//		throw_FigException("failed parsing the model file");
//	}
//	tech_log("- Parsing        succeeded\n");
//
//	// Debug print:
//	// { ModelPrinter printer(std::cerr,true); modelAST->accept(printer); }
//
//	// Check types
//	ModelTC typechecker;
//	modelAST->accept(typechecker);
//	if (typechecker.has_errors()) {
//		log(typechecker.get_messages());
//		throw_FigException("type-check for the model failed");
//	}
//	tech_log("- Type-checking  succeeded\n");
//
//    // Reduce expressions (errors when irreducible constants are found)
//	ModelReductor reductor;
//	modelAST->accept(reductor);
//    if (reductor.has_errors()) {
//        log(reductor.get_messages());
//        throw_FigException("reduction of constant expressions failed");
//    }
//	tech_log("- Expr-reduction succeeded\n");
//
//	// Check confluence if requested
//    if (confluenceCheck) {
//    	iosa::ConfluenceChecker confluence_verifier;
//        modelAST->accept(confluence_verifier);
//        if (confluence_verifier.has_errors()) {
//            log(confluence_verifier.get_messages());
//            tech_log("- Confluence-checking failed\n");
//        } else {
//            tech_log("- Confluence-checking succeeded\n");
//        }
//    }
//
//	// Check IOSA correctness
//	if (ModuleScope::modules_size_bounded_by(ModelVerifier::NTRANS_BOUND)) {
//		ModelVerifier verifier;
//        modelAST->accept(verifier);
//        if (verifier.has_errors()) {
//            log("\n[WARNING] IOSA-checking failed\n");
//			tech_log(verifier.get_messages());
//			if (!forceOperation) {
//				log(" -- aborting\n");
//				log("To force estimation disregarding ");
//				log("IOSA errors call with \"--force\"");
//				throw_FigException("iosa-check for the model failed");
//			} else {
//				log("\n");
//			}
//        } else if (verifier.has_warnings()) {
//            tech_log(verifier.get_messages());
//        }
//		tech_log("- IOSA-checking  succeeded\n");
//	} else {
//		log("- IOSA-checking skipped: model is too big\n");
//	}
//
//
//	// Build model (i.e. populate ModelSuite)
//	ModelBuilder builder;
//	modelAST->accept(builder);
//	if (builder.has_errors()) {
//		log(builder.get_messages());
//		throw_FigException("parser failed to build the model");
//	}
//	tech_log("- Model building succeeded\n");
//
//	// Seal model
//	auto& modelInstance = ModelSuite::get_instance();
//	modelInstance.seal();
//	if (!modelInstance.sealed()) {
//		log("[ERROR] Failed to seal the model.\n");
//		throw_FigException("parser failed sealing the model");
//	}
//	tech_log("- Model sealing  succeeded\n\n");
//
//	log(std::string("Model") +
//	    (propertiesFile.empty() ? (" file ") : (" and properties files "))
//	    + "successfully compiled.\n\n");

	return true;
}


} // namespace tests   // // // // // // // // // // // // // // // // // // //
