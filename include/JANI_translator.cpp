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
// External code
#include <json.h>


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

std::fstream
JaniTranslator::IOSA_2_JANI(const std::ifstream& iosaModelFile,
							bool validityCheck,
							const std::string& janiFilename)
{

}


std::fstream
JaniTranslator::IOSA_2_JANI(const std::ifstream& iosaModelFile,
							const std::ifstream& iosaPropsFile,
							bool validityCheck,
							const std::string& janiFilename)
{

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
		janiModelFile.close();
	} else {
		std::fstream janiModelFile(IOSA_2_JANI(iosaFile,
											   validityCheck,
											   janiFilename));
		if (!janiModelFile.good())
			throw_FigException("failed translating IOSA file \""
							   + iosaModelFile + "\"");
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
	janiModelFile >> root;  // read Json file into JsonCPP variable
	if (!root.isObject() ||
		!root.isMember("jani-version") ||
		!root.isMember("type") ||
		!root.isMember("automata"))
		throw_FigException("invalid JANI format");

	const std::string modelType = root.get("type", Json::nullValue).asString();
	if ( ! (modelType == "ctmc" || modelType == "sta") )
		throw_FigException("can't convert to IOSA from JANI model of type \""
						   + modelType + "\"");

	/// @todo TODO continue parsing JANI-Json and translate to IOSA
	///            (modularize in anonymous namespace)



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
