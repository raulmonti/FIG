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
#include <libgen.h>		// dirname()
#include <unistd.h>		// getcwd()
#include <sys/stat.h>	// stat()
// C++
#include <cstring>		// strndup()
#include <cassert>
// TESTS
#include <tests_definitions.h>


namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

//	/// Absolute path of current Process Working Directory
//	/// https://stackoverflow.com/a/2203177
//	string getPWD()
//	{
//		char tmp[128ul];
//		return (NULL != getcwd(tmp, 128ul) ? string(tmp) : string(""));
//	}

/// Absolute path to the dir containing this source file
/// @note Must be defined in an anonymous namespace in the corresp. file
string getThisDir()
{
	static const size_t MAXLEN(128ul);
	char tmp[MAXLEN];
	string fullpath(strndup(__FILE__, MAXLEN));
	assert(fullpath.length() > 3ul);
	return dirname(const_cast<char*>(fullpath.data()));
}

/// Check file accessibility
/// https://stackhttps://stackoverflow.com/a/12774387overflow.com/a/12774387
bool file_exists(const std::string& filepath)
{
	struct stat buffer;
	return (stat(filepath.c_str(), &buffer) == 0);
}

} // namespace   // // // // // // // // // // // // // // // // // // // // //



namespace tests  // // // // // // // // // // // // // // // // // // // // //
{

//const string MODELS_DIR(getCWD() + "../models/");
const string MODELS_DIR(getThisDir() + "/models/");


/// See declaration in tests_definitions.h
bool compile_model(const string& modelFilePath)
{
	REQUIRE(file_exists(modelFilePath));

	// Parse model file
	auto modelAST(ModelAST::from_files(modelFilePath.c_str(),""));
	REQUIRE(nullptr != modelAST);

	// Check types
	ModelTC typeChecker;
	modelAST->accept(typeChecker);
	REQUIRE_FALSE(typeChecker.has_errors());

	// Reduce expressions
	ModelReductor reductor;
	modelAST->accept(reductor);
	REQUIRE_FALSE(reductor.has_errors());

	// Check confluence
	iosa::ConfluenceChecker confluence_verifier;
	modelAST->accept(confluence_verifier);
	CHECK_FALSE(confluence_verifier.has_errors());

	// Check IOSA correctness (for small enough modules only)
	if (ModuleScope::modules_size_bounded_by(ModelVerifier::NTRANS_BOUND)) {
		ModelVerifier verifier;
		modelAST->accept(verifier);
		REQUIRE_FALSE(verifier.has_errors());
	}

	// Build model, i.e. populate ModelSuite
	ModelBuilder builder;
	modelAST->accept(builder);
	REQUIRE_FALSE(builder.has_errors());

	return true;
}


/// See declaration in tests_definitions.h
bool seal_model()
{
	static auto& model(fig::ModelSuite::get_instance());
	model.seal();
	REQUIRE(model.sealed());
	return true;
}


} // namespace tests   // // // // // // // // // // // // // // // // // // //
