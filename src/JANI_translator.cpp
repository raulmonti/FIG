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
#include <cassert>
// FIG
#include <string_utils.h>
#include <JANI_translator.h>
#include <FigException.h>
#include <FigLog.h>
// External code
#include <json.h>


namespace   // // // // // // // // // // // // // // // // // // // // // // //
{

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

} // namespace   // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

std::fstream
JaniTranslator::IOSA_2_JANI(const std::ifstream& iosaModelFile,
							bool validityCheck,
							const std::string& janiFilename)
{
	/// @todo TODO: IOSA validity check when requested

}


std::fstream
JaniTranslator::IOSA_2_JANI(const std::ifstream& iosaModelFile,
							const std::ifstream& iosaPropsFile,
							bool validityCheck,
							const std::string& janiFilename)
{

	assert(iosaModelFile.good());
	assert(iosaPropsFile.good());
	assert(!janiFilename.empty());

	// Translate IOSA model
	auto janiModelFile = IOSA_2_JANI(iosaModelFile, validityCheck, janiFilename);
	assert(janiModelFile.good());

	// Parse IOSA properties and translate to JSON

	/// @todo TODO IOSA parsing using Leo's parser
	///            Save properties parsed in a Json::Value object


	// Add only new properties to JANI model
	Json::Value root;
	janiModelFile.seekg(0);
	janiModelFile >> root;
	assert(root.isObject());
	Json::Value modelProperties(Json::arrayValue);
	if (root.isMember("properties"))
		modelProperties = root["properties"];
	for (const auto& p: ??Json::Value object from above!!) {
		const std::string& pName(p["name"].asString());
		if (!present(pName, modelProperties))
			modelProperties.append(p);
		else
			figTechLog << "WARNING: skipping property \"" << pName
					   << "\" from properties file translation, since an "
					   << "homonimous property was found in the model file.\n";
	}
	root["properties"] = modelProperties;

	// Dump to JANI file and return
	// NOTE: overwriting shouldn't be a problem (biggest file seen was < 6MB)
	janiModelFile.seekg(0);
	janiModelFile << root;
	janiModelFile << std::endl;
	assert(janiModelFile.good());
	janiModelFile.close();

	return janiModelFile
}


std::string
JaniTranslator::IOSA_2_JANI(const std::string& iosaModelFile,
							const std::string& iosaPropsFile,
							bool validityCheck)
{
	std::string janiFilename = (filename_has_extension(iosaModelFile, ".iosa") ||
								filename_has_extension(iosaModelFile, ".sa"))
							   ? change_filename_extension(janiModelFile, ".jani")
							   : janiModelFile + ".jani";
	std::ifstream iosaFile(iosaModelFile);
	if (!iosaFile.good())
		throw_FigException("failed opening IOSA file \"" + iosaModelFile + "\"");
	// Don't reinvent the wheel
	if (iosaPropsFile.empty()) {
		std::ifstream propsFile(iosaPropsFile);
		if (!propsFile.good())
			throw_FigException("failed opening properties file \""
							   + iosaPropsFile + "\"");
		std::fstream janiModelFile(IOSA_2_JANI(iosaFile,
											   propsFile,
											   validityCheck,
											   janiFilename));
		if (!janiModelFile.good())
			throw_FigException("failed translating IOSA file \""
							   + iosaModelFile + "\" and properties file \""
							   + iosaPropsFile + "\"");
		else if (janiModelFile.is_open())
			janiModelFile.close();
	} else {
		std::fstream janiModelFile(IOSA_2_JANI(iosaFile,
											   validityCheck,
											   janiFilename));
		if (!janiModelFile.good())
			throw_FigException("failed translating IOSA file \""
							   + iosaModelFile + "\"");
		else if (janiModelFile.is_open())
			janiModelFile.close();
	}
	return janiFilename;
}


std::fstream
JaniTranslator::JANI_2_IOSA(const std::ifstream& janiModelFile,
							const std::string& iosaFilename)
{
	assert(janiModelFile.good());

	Json::Value root;
	janiModelFile >> root;     // read Json file into JsonCPP variable
	jani_is_valid_iosa(root);  // check IOSA compatibility

	/// @todo TODO continue parsing JANI-Json and translate to IOSA
	///            (check IOSA_2_JANI for tips on JsonCPP usage)


	// example:
	const auto& actions = root.get("actions", Json::nullValue);
	assert(actions.isArray() || actions.isNull());
	if (!actions.isNull()) {
		std::cout << "Model actions:";
		for (const auto& a: actions) {
			assert(a.isObject());
			assert(a.isMember("name"));
			std::cout << " " << a.get("name",Json::nullValue).asString();
		}
		std::cout << std::endl;
	}
	// // // // // // // // // // // // // // // // // // // // //

}


std::string
JaniTranslator::JANI_2_IOSA(const std::string& janiModelFile)
{
	std::string iosaFilename = filename_has_extension(janiModelFile, ".jani")
							   ? change_filename_extension(janiModelFile, ".sa")
							   : janiModelFile + ".sa";
	std::ifstream janiFile(janiModelFile);
	if (!janiFile.good())
		throw_FigException("failed opening JANI file \"" + janiModelFile + "\"");
	// Don't reinvent the wheel
	std::fstream iosaModelFile = JANI_2_IOSA(janiFile, iosaFilename);
	if (!iosaModelFile.good())
		throw_FigException("failed translating JANI file \"" + janiModelFile + "\"");
	iosaModelFile.close();
	return iosaFilename;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
