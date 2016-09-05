//==============================================================================
//
//  JANI_translator.cpp
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
#include <ctime>
#include <cassert>
#include <cstring>
// C++
#include <memory>
#include <fstream>
// FIG
#include <string_utils.h>
#include <JANI_translator.h>
#include <ModelBuilder.h>
#include <ModelAST.h>
#include <ModelTC.h>
#include <FigException.h>
#include <FigLog.h>
// External code
#include <json.h>

using std::shared_ptr;


namespace   // // // // // // // // // // // // // // // // // // // // // // //
{

/// Build parser AST model of IOSA model file (and properties)
shared_ptr<ModelAST>
parse_IOSA_model(const string& iosaModelFile, const string& iosaPropsFile)
{
	const string iosaFileNames = iosaPropsFile.empty()
			? ("file \"" + iosaModelFile + "\"")
			: ("files \"" + iosaModelFile + " and \"" + iosaPropsFile + "\"");
	// Parse IOSA files
	shared_ptr<ModelAST> model = ModelAST::from_files(iosaModelFile.c_str(),
													  iosaPropsFile.c_str());
	if (nullptr == model) {
		fig::figTechLog  << "ERROR: failed parsing IOSA "
						 << iosaFileNames << std::endl;
	} else {
		ModelTC typechecker;
		model->accept(typechecker);
		if (typechecker.has_errors()) {
			fig::figTechLog << "ERROR: type-check failed for IOSA "
							<< iosaFileNames << std::endl;
			fig::figTechLog << typechecker.get_errors() << std::endl;
			model = nullptr;
		} else {
			ModelBuilder builder;
			model->accept(builder);
			if (builder.has_errors()) {
				fig::figTechLog << "ERROR: model building failed for IOSA "
								<< iosaFileNames << std::endl;
				fig::figTechLog << builder.get_errors() << std::endl;
				model = nullptr;
			}
		}
	}
	return model;
}


/// Compose filename for JANI file translated from IOSA model
/// @param iosaFname Name of the model file in IOSA syntax
/// @param janiFname Desired name for translated JANI file (optional)
/// @return If 'janiFname' is empty then change the extension of 'iosaFname'
///         from '.sa/.iosa' to '.jani' ; else 'janiFname' is returned.
string
compose_jani_fname(const string& iosaFname, const string& janiFname = "")
{
	if (!janiFname.empty())
		return janiFname;
	else if (filename_has_extension(iosaFname, ".iosa") ||
			 filename_has_extension(iosaFname, ".sa"))
		return change_filename_extension(iosaFname, ".jani");
	else
		return iosaFname + ".jani";
}


/// Compose filename for IOSA model translated from JANI Specifiaction format
/// @param janiFname Name of the model file in JANI Specification format
/// @param iosaFname Desired name for translated IOSA model (optional)
/// @return If 'iosaFname' is empty then change the extension of 'janiFname'
///         from '.jani' to '.sa' ; else 'iosaFname' is returned.
string
compose_iosa_fname(const string& janiFname, const string& iosaFname = "")
{
	if (!iosaFname.empty())
		return iosaFname;
	else if (filename_has_extension(janiFname, ".jani"))
		return change_filename_extension(janiFname, ".sa");
	else
		return janiFname + ".sa";
}


/// Fast superficial check for IOSA compatibility of JANI model
/// @param JANIjson JSON root object of a JANI model file
/// @param fatal    Whether to throw exception if JANI is not compatible
/// @return Whether the JANI model seems valid and convertible to IOSA
bool jani_is_valid_iosa(const Json::Value& JANIjson, bool fatal = true)
{
	if (!JANIjson.isObject() ||
		!JANIjson.isMember("jani-version") ||
		!JANIjson.isMember("type") ||
		!JANIjson.isMember("automata")) {
		if (fatal)
			throw_FigException("invalid JANI format");
		else
			return false;
	}
	const std::string modelType = JANIjson.get("type", Json::nullValue).asString();
	if ( ! (modelType == "ctmc" || modelType == "sta") ) {
		if (fatal)
			throw_FigException("can't convert to IOSA from JANI model "
							   "of type \"" + modelType + "\"");
		else
			return false;
	}
	return true;
}


/// Tell wether a property named "propertyName" is present
/// in the JSON array "propertiesArray" of JANI properties
bool present(const std::string& propertyName,
			 const Json::Value& propertiesArray)
{
	assert(propertiesArray.type() == Json::arrayValue);
	for (const auto& p: propertiesArray)
		if (p["name"] == propertyName)  // properties *must* be named in JANI
			return true;
	return false;
}


/// Add default JANI info (as from IOSA translated model file)
void
jani_header(Json::Value& root, const string& iosaModelFile)
{
	if (root.isNull())
		root = Json::Value(Json::objectValue);
	else if (!root.isObject())
		throw_FigException("invalid JsonCPP value root");
	root["jani-version"] = 1;
	root["name"] = iosaModelFile.c_str();
	root["type"] = "sta";
	root["metadata"] = Json::Value(Json::objectValue);
	auto now = std::time(nullptr);
	root["metadata"]["version"] = std::ctime(&now);
	root["metadata"]["author"]  = "FIG translator";
	root["metadata"]["description"] = "JANI file generated from IOSA model";
}

} // namespace   // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{


// std::fstream
// JaniTranslator::IOSA_2_JANI(std::ifstream& iosaModelFile,
// 							std::ifstream& iosaPropsFile,
// 							bool validityCheck,
// 							const std::string& janiFilename)
// {
//
// 	assert(iosaModelFile.good());
// 	assert(iosaPropsFile.good());
// 	assert(!janiFilename.empty());
//
// 	// Translate IOSA model
// 	auto janiModelFile = IOSA_2_JANI(iosaModelFile, validityCheck, janiFilename);
// 	assert(janiModelFile.good());
//
// 	// Parse IOSA properties and translate to JSON
// 	shared_ptr<ModelAST> model = ModelAST::from_file(iosaModelFile);
//
// 	/// @todo TODO IOSA parsing using Leo's parser
// 	///            Save properties parsed in a Json::Value object
//
//
// 	// Add only new properties to JANI model
// 	Json::Value root;
// 	janiModelFile.seekg(0);
// 	janiModelFile >> root;
// 	assert(root.isObject());
// 	Json::Value modelProperties(Json::arrayValue);
// 	if (root.isMember("properties"))
// 		modelProperties = root["properties"];
// 	for (const auto& p: ??Json::Value object from above!!) {
// 		const std::string& pName(p["name"].asString());
// 		if (!present(pName, modelProperties))
// 			modelProperties.append(p);
// 		else
// 			figTechLog << "WARNING: skipping property \"" << pName
// 					   << "\" from properties file translation, since an "
// 					   << "homonimous property was found in the model file.\n";
// 	}
// 	root["properties"] = modelProperties;
//
// 	// Dump to JANI file and return
// 	// NOTE: overwriting shouldn't be a problem (biggest file seen was < 6MB)
// 	janiModelFile.seekg(0);
// 	janiModelFile << root;
// 	janiModelFile << std::endl;
// 	assert(janiModelFile.good());
// 	janiModelFile.close();
//
// 	return janiModelFile;
// }


std::string
JaniTranslator::IOSA_2_JANI(const std::string& iosaModelFile,
							const std::string& iosaPropsFile,
							const std::string& janiFilename,
							bool validityCheck)
{
	auto model = parse_IOSA_model(iosaModelFile, iosaPropsFile);
	if (nullptr == model)
		throw_FigException("failed parsing IOSA files for JANI translation");
	if (validityCheck) {
		/// @todo TODO check for IOSA compliance!!!
	}

	/// @todo TODO translate IOSA parsed AST to JANI-Json



	Json::Value root;
	jani_header(root, iosaModelFile);
	/// @todo TODO write translated model in JsonCPP object


	// Dump translation in file and exit (FIXME!)
	auto janiFname = compose_jani_fname(iosaModelFile, janiFilename);
	std::ofstream janiFile(janiFname);
	janiFile << root;
	assert(janiFile.good());
	janiFile.close();

	return janiFname;
}


std::string
JaniTranslator::JANI_2_IOSA(const std::string& janiModelFile,
							const std::string& iosaFilename)
{
	std::ifstream janiFile(janiModelFile);
	if (!janiFile.good())
		throw_FigException("failed opening JANI file \"" + janiModelFile + "\"");

	Json::Value root;
	janiFile >> root;          // parse Json file
	jani_is_valid_iosa(root);  // IOSA compatibility fast-check


	/// @todo TODO continue parsing JANI-Json and translate to IOSA
	///            (check IOSA_2_JANI for tips on JsonCPP usage)


	// example:
//	const auto& actions = root.get("actions", Json::nullValue);
//	assert(actions.isArray() || actions.isNull());
//	if (!actions.isNull()) {
//		std::cout << "Model actions:";
//		for (const auto& a: actions) {
//			assert(a.isObject());
//			assert(a.isMember("name"));
//			std::cout << " " << a.get("name",Json::nullValue).asString();
//		}
//		std::cout << std::endl;
//	}
	// // // // // // // // // // // // // // // // // // // // //

	auto iosaFname = compose_iosa_fname(janiModelFile, iosaFilename);
	std::ofstream iosaFile(iosaFname);

	/// @todo TODO write translated model in 'iosaFile'

	return iosaFname;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
