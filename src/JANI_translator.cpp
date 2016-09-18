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
#include <ModelVerifier.h>
#include <ModelAST.h>
#include <ModelTC.h>
#include <ExpEvaluator.h>
#include <FigException.h>
#include <FigLog.h>
// External code
#include <json.h>

using std::shared_ptr;
using std::make_shared;
using std::string;
using std::vector;


namespace   // // // // // // // // // // // // // // // // // // // // // // //
{

/// Build parser AST model of IOSA model file (and properties)
shared_ptr<Model>
parse_IOSA_model(const string& iosaModelFile,
				 const string& iosaPropsFile,
				 bool validityCheck = false)
{
	ModelTC typechecker;
	ModelVerifier verifier;
	ModelBuilder builder;
	const string iosaFileNames = iosaPropsFile.empty()
			? ("file \"" + iosaModelFile + "\"")
			: ("files \"" + iosaModelFile + " and \"" + iosaPropsFile + "\"");

	// Parse IOSA files
	shared_ptr<ModelAST> modelAST = ModelAST::from_files(iosaModelFile.c_str(),
														 iosaPropsFile.c_str());
	if (nullptr == modelAST) {
		fig::figTechLog  << "ERROR: failed parsing IOSA "
						 << iosaFileNames << std::endl;
		goto exit_point;
	}

	// Check types
	modelAST->accept(typechecker);
	if (typechecker.has_errors()) {
		fig::figTechLog << "ERROR: type-checking failed for IOSA "
						<< iosaFileNames << std::endl;
		fig::figTechLog << typechecker.get_messages() << std::endl;
		modelAST = nullptr;
		goto exit_point;
	}

	// Check IOSA compliance if requested
	if (validityCheck &&
			ModuleScope::modules_size_bounded_by(ModelVerifier::NTRANS_BOUND)) {
		modelAST->accept(verifier);
		if (verifier.has_errors() || verifier.has_warnings()) {
			fig::figTechLog << "ERROR: IOSA-checking failed for "
							<< iosaFileNames << std::endl;
			fig::figTechLog << verifier.get_messages() << std::endl;
			modelAST = nullptr;
			goto exit_point;
		}
	}

	// Build "parser" model
	modelAST->accept(builder);
	if (builder.has_errors()) {
		fig::figTechLog << "ERROR: model building failed for IOSA "
						<< iosaFileNames << std::endl;
		fig::figTechLog << builder.get_messages() << std::endl;
		modelAST = nullptr;
	}

	exit_point:
		return std::dynamic_pointer_cast<Model>(modelAST);
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
	if (!JANIjson.isObject()               ||
		!JANIjson.isMember("jani-version") ||
		!JANIjson.isMember("name")         ||
		!JANIjson.isMember("type")         ||
		!JANIjson.isMember("system")       ||
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


// /// Tell wether a property named "propertyName" is present
// /// in the JSON array "propertiesArray" of JANI properties
// bool present(const std::string& propertyName,
// 			 const Json::Value& propertiesArray)
// {
// 	assert(propertiesArray.type() == Json::arrayValue);
// 	for (const auto& p: propertiesArray)
// 		if (p["name"] == propertyName)  // properties *must* be named in JANI
// 			return true;
// 	return false;
// }


/// Add default JANI header to JsonCPP object,
/// as from IOSA translated model file.
void
add_jani_header(Json::Value& root, const string& iosaModelFile)
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
	if (root.isMember("features")) {
		bool hasDerivedOps(false);
		for (const auto& f: root["features"])
			if (f.isString() && f.asString() == "derived-operators") {
				hasDerivedOps = true;
				break;
			}
		if (!hasDerivedOps)
			root["features"].append("derived-operators");
	} else {
		root["features"] = Json::Value(Json::arrayValue);
		root["features"].append("derived-operators");
	}
}

} // namespace   // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

int JaniTranslator::get_int_or_error(shared_ptr<Exp> exp,
                                     const std::string& msg) {
    int res = 0;
    ExpEvaluator ev;
    exp->accept(ev);
    if (ev.has_type_int()) {
        res = ev.get_int();
    } else {
        put_error(msg);
    }
    return (res);
}

bool JaniTranslator::get_bool_or_error(shared_ptr<Exp> exp,
                                       const std::string& msg) {
    bool res = false;
    ExpEvaluator ev;
    exp->accept(ev);
    if (ev.has_type_bool()) {
        res = ev.get_bool();
    } else {
        put_error(msg);
    }
    return (res);
}

float JaniTranslator::get_float_or_error(shared_ptr<Exp> exp,
                                         const std::string& msg) {
    float res = 0.0;
    ExpEvaluator ev;
    exp->accept(ev);
    if (ev.has_type_float()) {
        res = ev.get_float();
    } else {
        put_error(msg);
    }
    return (res);
}

//Json::Value JaniTranslator::JANIroot = Json::Value(Json::nullValue);
//vector< string > JANITracfield;


JaniTranslator::JaniTranslator() :
	JANIroot(make_shared<Json::Value>(Json::Value(Json::nullValue))),
	JANIfield(make_shared<Json::Value>(Json::Value(Json::nullValue))),
	builder()
{ /* Not much to do around here */ }


JaniTranslator::~JaniTranslator()
{
	JANIroot->clear();
	JANIroot.reset();
	JANIfield->clear();
	JANIfield.reset();
}


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
	auto model = parse_IOSA_model(iosaModelFile, iosaPropsFile, validityCheck);
	if (nullptr == model)
		throw_FigException("failed parsing IOSA files for JANI translation");

	// Translate IOSA to JANI
	JANIroot->clear();
	add_jani_header(*JANIroot, iosaModelFile);
	model->accept(*this);

	/// @todo erase debug print
	std::cerr << *JANIroot << std::endl;
//	for (auto it = JANIroot->begin() ; it != JANIroot->end() ; it++)
//		std::cerr << "\tJSON data: \"" << *it << "\"\n";
//	for (const auto& field: JANIroot->getMemberNames())
//		std::cerr << "\tField: \"" << field << "\"\n";
	throw_FigException("prematurely aborted");

	if (!jani_is_valid_iosa(*JANIroot, false))
		throw_FigException("failed translating IOSA files to the "
						   "JANI Specifiaction format");

	// Dump JANI-spec translation in file and exit (FIXME!)
	auto janiFname = compose_jani_fname(iosaModelFile, janiFilename);
	std::ofstream janiFile(janiFname);
	janiFile << *JANIroot;
	assert(janiFile.good());
	janiFile.close();
	figTechLog << "Translated JANI file: " << janiFname << std::endl;

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


void
JaniTranslator::visit(shared_ptr<Model> node)
{
	// Format JANIroot
	if (JANIroot->isObject() && JANIroot->isMember("automata"))
		JANIroot = make_shared<Json::Value>(Json::objectValue);

	// Parse global constants
	JANIfield = make_shared<Json::Value>(Json::arrayValue);
	for (auto decl_ptr : node->get_globals())
		decl_ptr->accept(*this);
	(*JANIroot)["constants"] = *JANIfield;

	/// @todo TODO continue with translation from IOSA to JANI-Json
}

void JaniTranslator::reduce_init(Json::Value &JANIobj,
                                 shared_ptr<InitializedDecl> node) {
    switch(node->get_type())
    {
    case Type::tbool:
    {
        JANIobj["type"] = "bool";
        std::stringstream msg;
        msg << "failed to reduce boolean value of ";
        msg << "\"" << node->get_id() << "\"\n" ;
        auto init = node->get_init();
        JANIobj["value"] = get_bool_or_error(init, msg.str());
        break;
    }
    case Type::tint:
        JANIobj["type"] = "int";
        JANIobj["value"] = get_int_or_error(node->get_init(),
                                   "failed to reduce integer value of "
                                   "constant \"" + node->get_id() + "\"\n");
        break;
    default:
        /// @todo Type::tfloat unsupported ???
        throw_FigException("unknown constant/clock declaration type: "
                           + std::to_string(static_cast<int>(node->get_type())));
        break;
    }
}


void
JaniTranslator::visit(shared_ptr<RangedDecl> node) {
    Json::Value JANIobj(Json::objectValue);
    /// @todo: variable builder should not be used anymore.
    assert( ! (builder.has_errors() || builder.has_warnings()) );

    /// @todo: typechecking should ensure "ranged things cannot be constant"
    assert(!node->is_constant());

    JANIobj["name"] = node->get_id();
    Json::Value type(Json::objectValue);
    type["kind"] = "bounded";
    type["base"] = "int";
    type["lower-bound"] = get_int_or_error(node->get_lower_bound(),
                                  "failed to reduce lower bound of "
                                  "variable \"" + node->get_id() + "\"\n");
    type["upper-bound"] = get_int_or_error(node->get_upper_bound(),
                                  "failed to reduce upper bound of "
                                  "variable \"" + node->get_id() + "\"\n");
    JANIobj["type"] = type;
    JANIobj["initial-value"] = get_int_or_error(node->get_init(),
                                  "failed to reduce initial value of "
                                  "variable \"" + node->get_id() + "\"\n");

    if (builder.has_errors() || builder.has_warnings())
        throw_FigException("error translating declaration: " + builder.get_messages());

    // Store translated data in corresponding field
    if (JANIfield->isArray())
        JANIfield->append(JANIobj);
    else
        (*JANIfield) = JANIobj;
}


void
JaniTranslator::visit(shared_ptr<InitializedDecl> node)
{
    Json::Value JANIobj(Json::objectValue);
	assert( ! (builder.has_errors() || builder.has_warnings()) );

    JANIobj["name"] = node->get_id();
    if (!node->is_constant() && node->get_type() == Type::tbool) {
        JANIobj["type"] = "bool";
        JANIobj["initial-value"] = get_bool_or_error(node->get_init(),
                                      "failed to reduce initial value of "
                                      "variable \"" + node->get_id() + "\"\n");
    } else {
        reduce_init(JANIobj, node);
    }

	if (builder.has_errors() || builder.has_warnings())
		throw_FigException("error translating declaration: " + builder.get_messages());

	// Store translated data in corresponding field
	if (JANIfield->isArray())
		JANIfield->append(JANIobj);
	else
		(*JANIfield) = JANIobj;
}

void
JaniTranslator::visit(shared_ptr<ClockDecl> node) {
    Json::Value JANIobj(Json::objectValue);
    JANIobj["name"] = node->get_id();
    JANIobj["type"] = "clock";
    JANIobj["value"] = 0;
    // Store translated data in corresponding field
    if (JANIfield->isArray())
        JANIfield->append(JANIobj);
    else
        (*JANIfield) = JANIobj;
}

void
JaniTranslator::visit(shared_ptr<ArrayDecl> node) {
    (void) node;
    throw_FigException("Arrays not yet supported");
}

void
JaniTranslator::visit(shared_ptr<BConst> node)
{
	if (JANIfield->isArray())
        JANIfield->append(node->get_value() ? "true" : "false");
	else
        (*JANIfield) = node->get_value() ? "true" : "false";
}


void
JaniTranslator::visit(shared_ptr<IConst> node)
{
	if (JANIfield->isArray())
        JANIfield->append(node->get_value());
	else
        (*JANIfield) = node->get_value();
}


void
JaniTranslator::visit(shared_ptr<FConst> node)
{
	if (JANIfield->isArray())
        JANIfield->append(node->get_value());
	else
        (*JANIfield) = node->get_value();
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
