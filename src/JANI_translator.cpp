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
#include <map>
#include <memory>
#include <fstream>
#include <sstream>
// FIG
#include <string_utils.h>
#include <JANI_translator.h>
#include <ModelVerifier.h>
#include <ModelBuilder.h>
#include <ModelPrinter.h>
#include <ModelAST.h>
#include <ModelTC.h>
#include <ModuleScope.h>
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

/// Operators arity
enum OpArity
{
	null,      // constant function
	i_unary,   // infix unary,   e.g. !a
	s_unary,   // suffix unary,  e.g. floor(a)
	i_binary,  // infix binary,  e.g. a + b
	s_binary,  // suffix binary, e.g. log(a,b)
	ternary    // i-t-e operator
};


/// IOSA -> JANI type translator
const std::map< Type, std::string > JANI_type =
{
	{ Type::tbool,   "bool"  },
	{ Type::tint,    "int"   },
	{ Type::tfloat,  "real"  },
	{ Type::tclock,  "clock" }
};


/// JANI operators arity
const std::map< std::string, OpArity > JANI_operator_arity =
{
	{ "⇒",  OpArity::i_binary },
	{ "∧",  OpArity::i_binary },
	{ "∨",  OpArity::i_binary },
	{ "¬",  OpArity::i_unary  },
	{ "=",  OpArity::i_binary },
	{ "≠",  OpArity::i_binary },
	{ "<",  OpArity::i_binary },
	{ ">",  OpArity::i_binary },
	{ "≤",  OpArity::i_binary },
	{ "≥",  OpArity::i_binary },
	{ "+",  OpArity::i_binary },
	{ "-",  OpArity::i_binary },
	{ "*",  OpArity::i_binary },
	{ "/",  OpArity::i_binary },
	{ "%",  OpArity::i_binary }
};


/// IOSA -> JANI operator translator
const std::map< ExpOp, std::string > JANI_operator_string =
{
	{ ExpOp::implies, "⇒" },
	{ ExpOp::andd,    "∧" },
	{ ExpOp::orr,     "∨" },
	{ ExpOp::nott,    "¬" },
	{ ExpOp::eq,      "=" },
	{ ExpOp::neq,     "≠" },
	{ ExpOp::lt,      "<" },
	{ ExpOp::gt,      ">" },
	{ ExpOp::le,      "≤" },
	{ ExpOp::ge,      "≥" },
	{ ExpOp::plus,    "+" },
	{ ExpOp::minus,   "-" },
	{ ExpOp::times,   "*" },
	{ ExpOp::div,     "/" },
	{ ExpOp::mod,     "%" }
};


/// IOSA -> JANI clock distribution translator
const std::map< DistType, std::string > JANI_distribution_string =
{
	{ DistType::uniform,     "Uniform"    },
	{ DistType::exponential, "Exponential"},
	{ DistType::normal,      "Normal"     },
	{ DistType::lognormal,   "LogNormal"  },
	{ DistType::weibull,     "Weibull"    },
	{ DistType::rayleigh,    "Rayleigh"   },
	{ DistType::gamma,       "Gamma"      },
	{ DistType::erlang,      "Erlang"     }
};


/// JANI -> IOSA type translator
const std::map< std::string, Type > IOSA_type =
{
	{"bool",   Type::tbool  },
	{"int",    Type::tint   },
	{"real",   Type::tfloat },
	{"clock",  Type::tclock }
};


/// JANI -> IOSA operator translator
const std::map< std::string, ExpOp > IOSA_operator =
{
	{ "⇒", ExpOp::implies },
	{ "∧", ExpOp::andd    },
	{ "∨", ExpOp::orr     },
	{ "¬", ExpOp::nott    },
	{ "=", ExpOp::eq      },
	{ "≠", ExpOp::neq     },
	{ "<", ExpOp::lt      },
	{ ">", ExpOp::gt      },
	{ "≤", ExpOp::le      },
	{ "≥", ExpOp::ge      },
	{ "+", ExpOp::plus    },
	{ "-", ExpOp::minus   },
	{ "*", ExpOp::times   },
	{ "/", ExpOp::div     },
	{ "%", ExpOp::mod     }
};


/// JANI -> IOSA operator (string) translator
const std::map< std::string, std::string > IOSA_operator_string =
{
	{ "⇒", "=>" },
	{ "∧",  "&" },
	{ "∨",  "|" },
	{ "¬",  "!" },
	{ "=",  "=" },
	{ "≠", "!=" },
	{ "<",  "<" },
	{ ">",  ">" },
	{ "≤", "<=" },
	{ "≥", ">=" },
	{ "+",  "+" },
	{ "-",  "-" },
	{ "*",  "*" },
	{ "/",  "/" },
	{ "%",  "%" }
};


/// JANI -> IOSA clock distribution translator
const std::map< std::string, std::string > IOSA_distribution_string =
{
	{ "Uniform",     "uniform"    },
	{ "Exponential", "exponential"},
	{ "Normal",      "normal"     },
	{ "LogNormal",   "logNormal"  },
	{ "Weibull",     "weibull"    },
	{ "Rayleigh",    "rayleigh"   },
	{ "Gamma",       "gamma"      },
	{ "Erlang",      "erlang"     }
};


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


/// Build a IOSA model from given AST (viz. populate the ModelSuite)
/// performing all necessary checks in the process.
/// @return Whether the IOSA model could be successfully built
bool
build_IOSA_model_from_AST(Model& modelAST, bool verifyIOSA = true)
{
	// Check types
	ModelTC typechecker;
	modelAST.accept(typechecker);
	if (typechecker.has_errors()) {
		fig::figTechLog << "[ERROR] Type-checking failed" << std::endl;
		fig::figTechLog << typechecker.get_messages() << std::endl;
		return false;
	}

	// Check IOSA compliance if requested
	if (verifyIOSA &&
			ModuleScope::modules_size_bounded_by(ModelVerifier::NTRANS_BOUND)) {
		ModelVerifier verifier;
		modelAST.accept(verifier);
		if (verifier.has_errors() || verifier.has_warnings()) {
			fig::figTechLog << "[ERROR] IOSA-checking failed" << std::endl;
			fig::figTechLog << verifier.get_messages() << std::endl;
			return false;
		}
	}

	// Build model (i.e. populate ModelSuite)
	ModelBuilder builder;
	modelAST.accept(builder);
	if (builder.has_errors()) {
		fig::figTechLog << "[ERROR] Model building failed" << std::endl;
		fig::figTechLog << builder.get_messages() << std::endl;
		return false;
	}

	// Success iff the ModelSuite is sealed
	return ModelSuite::get_instance().sealed();
}

} // namespace   // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

constexpr char JaniTranslator::REAL_VAR_FROM_CLOCK_PREFIX[];
const Json::Value JaniTranslator::EMPTY_JSON_OBJ = Json::Value(Json::objectValue);
const Json::Value JaniTranslator::EMPTY_JSON_ARR = Json::Value(Json::arrayValue);


JaniTranslator::JaniTranslator() :
	JANIroot_(make_shared<Json::Value>(EMPTY_JSON_OBJ)),
	JANIfield_(make_shared<Json::Value>(EMPTY_JSON_OBJ)),
	currentModule_(),
	currentScope_(nullptr),
	timeProgressInvariant_(make_shared<Json::Value>(Json::nullValue))
{ /* Not much to do around here */ }


JaniTranslator::~JaniTranslator()
{
	JANIroot_.reset();
	JANIfield_.reset();
	currentScope_.reset();
	timeProgressInvariant_.reset();
	modulesLabels_.clear();
	modelLabels_.clear();;
}


std::string
JaniTranslator::rv_from(const std::string& clockName)
{
	return REAL_VAR_FROM_CLOCK_PREFIX + clockName;
}


int
JaniTranslator::get_int_or_error(shared_ptr<Exp> exp,
								 const std::string& msg)
{
	int res = 0;
	ExpEvaluator ev (currentScope_);
	exp->accept(ev);
	if (ev.has_type_int())
		res = ev.get_int();
	else
		put_error(msg);
	return (res);
}


bool
JaniTranslator::get_bool_or_error(shared_ptr<Exp> exp,
								  const std::string& msg)
{
	bool res = false;
	ExpEvaluator ev (currentScope_);
	exp->accept(ev);
	if (ev.has_type_bool())
		res = ev.get_bool();
	else
		put_error(msg);
	return res;
}


float
JaniTranslator::get_float_or_error(shared_ptr<Exp> exp,
								   const std::string& msg)
{
	float res = 0.0;
	ExpEvaluator ev (currentScope_);
	exp->accept(ev);
	if (ev.has_type_float())
		res = ev.get_float();
	else
		put_error(msg);
	return res;
}



////////////////////////////////////////////////////////////////////////////////
//                   ///////////////////////////////////////////////////////////
//   IOSA --> JANI   ///////////////////////////////////////////////////////////


std::string
JaniTranslator::IOSA_2_JANI(const std::string& iosaModelFile,
							const std::string& iosaPropsFile,
							const std::string& janiFilename,
							bool validityCheck)
{
	// Parse IOSA model file
	parse_IOSA_model(iosaModelFile, iosaPropsFile, validityCheck);
	if (IOSAroot_ == nullptr)
		throw_FigException("invalid IOSA model given for translation to JANI");

	// Translate IOSA to JANI
	(*JANIroot_) = EMPTY_JSON_OBJ;
	add_jani_header(*JANIroot_, iosaModelFile);
	IOSAroot_->accept(*this);
	if (!jani_is_valid_iosa(*JANIroot_, false)) {
#ifndef NDEBUG
		figMainLog << "[ERROR] Invalid JANI model file created !!!\n";
		figTechLog << "[ERROR] Invalid JANI model file created !!!\n";
#else
		throw_FigException("failed translating IOSA files to the "
						   "JANI Specifiaction format");
#endif
	}

	// Dump JANI-spec translation to file and exit
	auto janiFname = compose_jani_fname(iosaModelFile, janiFilename);
	std::ofstream janiFile(janiFname);
	janiFile << (*JANIroot_);
	assert(janiFile.good());
	janiFile.close();
	figTechLog << "Translated JANI file: " << janiFname << std::endl;

	return janiFname;
}


void
JaniTranslator::parse_IOSA_model(const string& iosaModelFile,
								 const string& iosaPropsFile,
								 bool validityCheck)
{
	IOSAroot_.reset();
	const string iosaFileNames = iosaPropsFile.empty()
			? ("file \"" + iosaModelFile + "\"")
			: ("files \"" + iosaModelFile + " and \"" + iosaPropsFile + "\"");

	// Parse IOSA files
	shared_ptr<ModelAST> modelAST = ModelAST::from_files(iosaModelFile.c_str(),
														 iosaPropsFile.c_str());
	if (nullptr != modelAST) {
		// Populate ModelSuite
		bool success = build_IOSA_model_from_AST(dynamic_cast<Model&>(*modelAST),
												 validityCheck);
		if (success)
			IOSAroot_ = std::dynamic_pointer_cast<Model>(modelAST);
		else
			figTechLog << "[ERROR] Failed building IOSA " << iosaFileNames << std::endl;
	} else {
		figTechLog << "[ERROR] Failed parsing IOSA " << iosaFileNames << std::endl;
	}
}


void
JaniTranslator::build_JANI_constant(shared_ptr<InitializedDecl> node,
									Json::Value& JANIobj)
{
	assert(node->is_constant());
	assert(JANIobj.isObject());
	switch(node->get_type())
	{
	case Type::tbool:
		JANIobj["type"] = "bool";
		JANIobj["value"] = get_bool_or_error(node->get_init(),
								   "failed to reduce boolean value of "
								   "constant \"" + node->get_id() + "\"\n");
		break;

	case Type::tint:
		JANIobj["type"] = "int";
		JANIobj["value"] = get_int_or_error(node->get_init(),
								   "failed to reduce integer value of "
								   "constant \"" + node->get_id() + "\"\n");
		break;

	case Type::tfloat:
		JANIobj["type"] = "real";
		JANIobj["value"] = get_float_or_error(node->get_init(),
								   "failed to reduce floating point value of "
								   "constant \"" + node->get_id() + "\"\n");
		break;

	default:
		throw_FigException("invalid initialized declaration type: " +
						   std::to_string(static_cast<int>(node->get_type())));
		break;
	}
}


void
JaniTranslator::build_JANI_guard(shared_ptr<TransitionAST> trans,
								 Json::Value& JANIobj)
{
	if (JANIobj.isNull())
		JANIobj = EMPTY_JSON_OBJ;
	assert(JANIobj.isObject());
	(*JANIfield_) = EMPTY_JSON_OBJ;
	trans->get_precondition()->accept(*this);
	if (trans->has_triggering_clock()) {
		const auto clockName = trans->to_output()
							   ->get_triggering_clock()->get_identifier();
		// Add STA guard for the clock: "clk >= real_var"
		auto guard(EMPTY_JSON_OBJ);
		guard["left"]  = *JANIfield_;
		guard["op"]    = JANI_operator_string.at(ExpOp::andd).c_str();
		build_JANI_clock_comp(clockName, ExpOp::ge, guard["right"]);
		JANIfield_->swap(guard);  // now 'guard' has the "clean precondition"
		// Add STA condition for time progress invariant: "guard implies clk <= real_var"
		auto tmp(EMPTY_JSON_OBJ);
		tmp["left"]  = *timeProgressInvariant_;
		tmp["op"]    = JANI_operator_string.at(ExpOp::andd).c_str();
		tmp["right"] = EMPTY_JSON_OBJ;
		tmp["right"]["left"]  = guard;
		tmp["right"]["op"]    = JANI_operator_string.at(ExpOp::implies).c_str();
		build_JANI_clock_comp(clockName, ExpOp::le, tmp["right"]["right"]);
		if (timeProgressInvariant_->isNull())
			timeProgressInvariant_->swap(tmp["right"]);
		else
			timeProgressInvariant_->swap(tmp);
	}
	JANIobj["guard"] = EMPTY_JSON_OBJ;
	JANIobj["guard"]["exp"] = *JANIfield_;
}


void
JaniTranslator::build_JANI_clock_comp(const std::string& clockName,
									  ExpOp op,
									  Json::Value& JANIobj)
{
	if (JANIobj.isNull())
		JANIobj = EMPTY_JSON_OBJ;
	assert(JANIobj.isObject());
	if (op != ExpOp::ge && op != ExpOp::le)
		throw_FigException("invalid comparison operator for clock guard");
	JANIobj.clear();
	JANIobj["left"]  = clockName.c_str();
	JANIobj["op"]    = JANI_operator_string.at(op).c_str();
	JANIobj["right"] = rv_from(clockName).c_str();
}


void
JaniTranslator::build_JANI_destinations(shared_ptr<TransitionAST> trans,
										Json::Value& JANIobj)
{
	if (JANIobj.isNull())
		JANIobj = EMPTY_JSON_OBJ;
	assert(JANIobj.isObject());
	JANIobj["destinations"] = EMPTY_JSON_ARR;
	JANIobj["destinations"].append(EMPTY_JSON_OBJ);
	JANIobj["destinations"][0]["location"] = "location";
	JANIobj["destinations"][0]["assignments"] = EMPTY_JSON_ARR;
	auto& ass = JANIobj["destinations"][0]["assignments"];
	// Variables assignments
	for (auto ass_ptr: trans->get_assignments()) {
		ass_ptr->accept(*this);
		ass.append(*JANIfield_);
	}
	// Clocks resets
	for (auto reset_ptr: trans->get_clock_resets()) {
		auto tmp(EMPTY_JSON_OBJ);
		const auto clockName = reset_ptr->get_effect_location()->get_identifier();
		// Clock value becomes zero...
		tmp["ref"] = clockName.c_str();
		tmp["value"] = 0;
		ass.append(tmp);
		// ...and its real variable is sampled from the distribution
		tmp.clear();
		tmp["ref"] = rv_from(clockName).c_str();
		build_JANI_distribution(reset_ptr->get_dist(), tmp["value"]);
		assert(tmp["value"].isMember("distribution"));
		assert(tmp["value"].isMember("args"));
		ass.append(tmp);
	}
}


void
JaniTranslator::build_JANI_distribution(shared_ptr<Dist> clockDist,
										Json::Value& JANIobj)
{
	if (JANIobj.isNull())
		JANIobj = EMPTY_JSON_OBJ;
	assert(JANIobj.isObject());
	JANIobj["distribution"] = JANI_distribution_string.at(clockDist->get_type()).c_str();
	JANIobj["args"] = EMPTY_JSON_ARR;
	if (clockDist->has_single_parameter()) {
		auto spDist = clockDist->to_single_parameter();
		assert(nullptr != spDist);
		*JANIfield_ = EMPTY_JSON_OBJ;
		spDist->get_parameter()->accept(*this);
		JANIobj["args"].append(*JANIfield_);
	} else {
		auto mpDist = clockDist->to_multiple_parameter();
		assert(nullptr != mpDist);
		*JANIfield_ = EMPTY_JSON_OBJ;
		mpDist->get_first_parameter()->accept(*this);
		JANIobj["args"].append(*JANIfield_);
		*JANIfield_ = EMPTY_JSON_OBJ;
		mpDist->get_second_parameter()->accept(*this);
		JANIobj["args"].append(*JANIfield_);
	}
}


void
JaniTranslator::build_JANI_synchronization(Json::Value& JANIobj)
{
	if (JANIobj.isNull())
		JANIobj = EMPTY_JSON_OBJ;
	assert(JANIobj.isObject());

	JANIobj["elements"] = EMPTY_JSON_ARR;
	JANIobj["syncs"] = EMPTY_JSON_ARR;
	for (const auto& pair: modulesLabels_) {
		auto tmp = EMPTY_JSON_OBJ;

		// Synchronizing elements
		tmp["automaton"] = pair.first.c_str();
		tmp["input-enable"] = EMPTY_JSON_ARR;
		for (const std::string& inputLabel: pair.second.first)
			tmp["input-enable"].append(inputLabel.c_str());
		if (tmp["input-enable"].empty())
			tmp.removeMember("input-enable");
		JANIobj["elements"].append(tmp);

		// Synchronization vectors
		tmp.clear();
		for (const std::string& outputLabel: pair.second.second)
			build_JANI_sync_vector(outputLabel, tmp["synchronize"]);
		if (tmp["synchronize"].size() > 0ul)
			JANIobj["syncs"].append(tmp);  // no nested sync => no need for "result"
	}
	if (JANIobj["syncs"].empty())
		JANIobj.removeMember("syncs");
}


void
JaniTranslator::build_JANI_sync_vector(const std::string& oLabel,
									   Json::Value& JANIarr)
{
	bool noSync(true);
	if (JANIarr.isNull())
		JANIarr = EMPTY_JSON_ARR;
	assert(JANIarr.isArray());
	if (oLabel.length() <= 0ul)  // tau => no sync => empty vector
		return;
	for (const auto& pair: modulesLabels_) {
		if (pair.second.first.find(oLabel) != end(pair.second.first)) {
			JANIarr.append(oLabel.c_str());   noSync = false;
		} else if (pair.second.second.find(oLabel) != end(pair.second.second)) {
			JANIarr.append(oLabel.c_str());
		} else {
			JANIarr.append(Json::nullValue);
		}
	}
	if (noSync)
		JANIarr = EMPTY_JSON_ARR;
}


void
JaniTranslator::visit(shared_ptr<Model> node)
{
	// Format fields to fill in
	if (JANIroot_->isObject() && JANIroot_->isMember("automata"))
		JANIroot_ = make_shared<Json::Value>(Json::objectValue);
	currentScope_.reset();
	modelLabels_.clear();
	modulesLabels_.clear();

	// Parse global constants
	JANIfield_ = make_shared<Json::Value>(Json::arrayValue);
	for (auto decl_ptr: node->get_globals()) {
		assert(decl_ptr->is_constant());  // only global *constants* for now
		decl_ptr->accept(*this);
	}
	(*JANIroot_)["constants"] = *JANIfield_;

	// Parse all modules
	JANIfield_ = make_shared<Json::Value>(Json::arrayValue);
	for (auto module_ptr: node->get_modules())
		module_ptr->accept(*this);
	(*JANIroot_)["automata"] = *JANIfield_;

	// Get the labels from all modules
	(*JANIroot_)["actions"] = Json::Value(Json::arrayValue);
	for (const string& label: modelLabels_) {
		Json::Value action(Json::objectValue);
		action["name"] = label.c_str();
		(*JANIroot_)["actions"].append(action);
	}

	// Compose the automata with synchronization vectors
	build_JANI_synchronization((*JANIroot_)["system"]);
	assert((*JANIroot_)["system"].isObject());
	assert((*JANIroot_)["system"].isMember("elements"));
	assert((*JANIroot_)["system"]["elements"].isArray());


	/// @todo TODO translate properties, if present

}



void
JaniTranslator::visit(shared_ptr<RangedDecl> node)
{
	auto JANIobj = EMPTY_JSON_OBJ, type = EMPTY_JSON_OBJ;

    assert( ! (has_errors() || has_warnings()) );
    assert(!node->is_constant());

    type["kind"] = "bounded";
    type["base"] = "int";
    type["lower-bound"] = get_int_or_error(node->get_lower_bound(),
                                  "failed to reduce lower bound of "
                                  "variable \"" + node->get_id() + "\"\n");
    type["upper-bound"] = get_int_or_error(node->get_upper_bound(),
                                  "failed to reduce upper bound of "
                                  "variable \"" + node->get_id() + "\"\n");
    JANIobj["type"] = type;
	JANIobj["name"] = node->get_id();
	JANIobj["initial-value"] = get_int_or_error(node->get_init(),
                                  "failed to reduce initial value of "
                                  "variable \"" + node->get_id() + "\"\n");

    if (has_errors() || has_warnings())
		throw_FigException("error translating declaration: " + get_messages());

    // Store translated data in corresponding field
	if (JANIfield_->isArray())
		JANIfield_->append(JANIobj);
    else
		(*JANIfield_) = JANIobj;
}


void
JaniTranslator::visit(shared_ptr<InitializedDecl> node)
{
	auto JANIobj(EMPTY_JSON_OBJ);
	assert( ! (has_errors() || has_warnings()) );

    JANIobj["name"] = node->get_id();
    if (!node->is_constant() && node->get_type() == Type::tbool) {
		// Variable of type bool (not "ranged")
        JANIobj["type"] = "bool";
        JANIobj["initial-value"] = get_bool_or_error(node->get_init(),
                                      "failed to reduce initial value of \""
                                      + node->get_id() + "\"\n");
    } else {
		// Constant
		build_JANI_constant(node, JANIobj);
    }

	if (has_errors() || has_warnings())
		throw_FigException("error translating declaration: " + get_messages());

	// Store translated data in corresponding field
	if (JANIfield_->isArray())
		JANIfield_->append(JANIobj);
	else
		(*JANIfield_) = JANIobj;
}


void
JaniTranslator::visit(shared_ptr<ClockDecl> node)
{
	auto JANIobj(EMPTY_JSON_OBJ);
	JANIobj["name"] = node->get_id().c_str();
    JANIobj["type"] = "clock";
	JANIobj["initial-value"] = 0;
	if (JANIfield_->isArray())
		JANIfield_->append(JANIobj);
    else
		(*JANIfield_) = JANIobj;
	// Create also a real variable used by STA to make time progress as in SA
	JANIobj.clear();
	JANIobj["name"] = rv_from(node->get_id()).c_str();
	JANIobj["type"] = "real";
	JANIobj["initial-value"] = 0;
	if (JANIfield_->isArray())
		JANIfield_->append(JANIobj);
	else
		throw_FigException("can't append second variable; current technique "
						   "for SA -> STA translation needs an array here");
}


void
JaniTranslator::visit(shared_ptr<ArrayDecl>)
{
	throw_FigException("arrays not yet supported");
}


void
JaniTranslator::visit(shared_ptr<BConst> node)
{
	if (JANIfield_->isArray())
		JANIfield_->append(node->get_value() ? "true" : "false");
	else
		(*JANIfield_) = node->get_value() ? "true" : "false";
}


void
JaniTranslator::visit(shared_ptr<IConst> node)
{
	if (JANIfield_->isArray())
		JANIfield_->append(node->get_value());
	else
		(*JANIfield_) = node->get_value();
}


void
JaniTranslator::visit(shared_ptr<FConst> node)
{
	if (JANIfield_->isArray())
		JANIfield_->append(node->get_value());
	else
		(*JANIfield_) = node->get_value();
}


void
JaniTranslator::visit(shared_ptr<LocExp> node)
{
	/// @todo FIXME: how do we check whether this is an array? Or can't it be?
	if (JANIfield_->isArray())
		JANIfield_->append(node->get_exp_location()->get_identifier().c_str());
	else
		(*JANIfield_) = node->get_exp_location()->get_identifier().c_str();
}


void
JaniTranslator::visit(shared_ptr<UnOpExp> node)
{
	auto JANIobj(EMPTY_JSON_OBJ);
	const auto tmp = *JANIfield_;
	// Translate operator
	JANIobj["op"] = JANI_operator_string.at(node->get_operator()).c_str();
	// Translate single operand
	(*JANIfield_) = EMPTY_JSON_OBJ;
	node->get_argument()->accept(*this);
	JANIobj["exp"] = *JANIfield_;  // NOTE: won't work for JANI's "derivative"
	// Store translated data in corresponding field
	(*JANIfield_) = tmp;
	if (JANIfield_->isArray())
		JANIfield_->append(JANIobj);
	else
		(*JANIfield_) = JANIobj;
}


void
JaniTranslator::visit(shared_ptr<BinOpExp> node)
{
	auto JANIobj(EMPTY_JSON_OBJ);
	const auto tmp = *JANIfield_;
	// Translate operator
	JANIobj["op"] = JANI_operator_string.at(node->get_operator()).c_str();
	// Translate operands
	(*JANIfield_) = EMPTY_JSON_OBJ;
	node->get_first_argument()->accept(*this);
	JANIobj["left"] = *JANIfield_;
	(*JANIfield_) = EMPTY_JSON_OBJ;
	node->get_second_argument()->accept(*this);
	JANIobj["right"] = *JANIfield_;
	// Store translated data in corresponding field
	*JANIfield_ = tmp;
	if (JANIfield_->isArray())
		JANIfield_->append(JANIobj);
	else
		(*JANIfield_) = JANIobj;
}


void
JaniTranslator::visit(shared_ptr<ModuleAST> node)
{
	auto JANIobj(EMPTY_JSON_OBJ);
	const auto tmp = *JANIfield_;

	// Reference this module as "current"
	currentModule_ = node->get_name();
	currentScope_  = ModuleScope::scopes.at(currentModule_);
	*timeProgressInvariant_ = Json::Value(Json::nullValue);
	modulesLabels_[currentModule_] = std::make_pair(std::set<string>(),
													std::set<string>());
	// Easy-to-guess module fields
	JANIobj["name"] = node->get_name();
	JANIobj["initial-locations"] = EMPTY_JSON_ARR;
	JANIobj["initial-locations"].append("location");

	// Module variables + Real variables for clocks (STA)
	(*JANIfield_) = EMPTY_JSON_ARR;
	for (auto decl_ptr: node->get_local_decls()) {
		assert(!decl_ptr->is_constant());  // only local *variables* for now
		decl_ptr->accept(*this);
	}
	JANIobj["variables"] = *JANIfield_;

	// Module transitions
	(*JANIfield_) = EMPTY_JSON_ARR;
	for (auto tr_ptr: node->get_transitions())
		tr_ptr->accept(*this);  // timeProgressInvariant_ is updated
	JANIobj["edges"] = *JANIfield_;

	// STA time-progress invariant
	JANIobj["locations"] = EMPTY_JSON_ARR;
	JANIobj["locations"].append(EMPTY_JSON_OBJ);
	JANIobj["locations"][0]["name"] = "location";
	JANIobj["locations"][0]["time-progress"] = EMPTY_JSON_OBJ;
	JANIobj["locations"][0]["time-progress"]["exp"] = *timeProgressInvariant_;

	// Store translated data in corresponding field
	*JANIfield_ = tmp;
	if (JANIfield_->isArray())
		JANIfield_->append(JANIobj);
	else
		(*JANIfield_) = JANIobj;
}


void
JaniTranslator::visit(shared_ptr<TransitionAST> node)
{
	auto JANIobj(EMPTY_JSON_OBJ);
	const auto tmp = *JANIfield_;

	JANIobj["location"] = "location";

	// Label
	const auto label = node->get_label();
	JANIobj["action"] = label.c_str();
	modelLabels_.emplace(label);
	switch (node->get_label_type())
	{
	case LabelType::in:
		modulesLabels_[currentModule_].first.emplace(label);
		break;
	case LabelType::out:
	case LabelType::tau:
		modulesLabels_[currentModule_].second.emplace(label);
		break;
	case LabelType::in_committed:
	case LabelType::out_committed:
		throw_FigException("committed acctions not yet supported in JANI");
		break;
	default:
		throw_FigException("invalid label type: " + std::to_string(
							   static_cast<int>(node->get_label_type())));
		break;
	}

	// Precondition
	build_JANI_guard(node, JANIobj);  // also updates timeProgressInvariant_
	assert(JANIobj.isMember("guard"));
	assert(JANIobj["guard"].isObject());
	assert(JANIobj["guard"].isMember("exp"));

	// Postcondition
	build_JANI_destinations(node, JANIobj);
	assert(JANIobj.isMember("destinations"));
	assert(JANIobj["destinations"].isArray());
	assert(JANIobj["destinations"][0].isMember("assignments"));

	// Store translated data in corresponding field
	*JANIfield_ = tmp;
	if (JANIfield_->isArray())
		JANIfield_->append(JANIobj);
	else
		(*JANIfield_) = JANIobj;
}


void
JaniTranslator::visit(shared_ptr<Assignment> node)
{
	auto JANIobj(EMPTY_JSON_OBJ);
	JANIobj["ref"] = node->get_effect_location()->get_identifier().c_str();
	node->get_rhs()->accept(*this);
	JANIobj["value"] = *JANIfield_;
	JANIfield_->swap(JANIobj);
}



////////////////////////////////////////////////////////////////////////////////
//                   ///////////////////////////////////////////////////////////
//   JANI --> IOSA   ///////////////////////////////////////////////////////////


std::string
JaniTranslator::JANI_2_IOSA(const std::string& janiModelFile,
							const std::string& iosaFilename,
							bool skipFileDump)
{
	// Parse JANI model file
	parse_JANI_model(janiModelFile);
	assert(JANIroot_ != nullptr);
	if ( ! (JANIroot_->isObject() && JANIroot_->isMember("jani-version")) )
		throw_FigException("invalid JANI file given for translation to IOSA");

	// Translate JANI to IOSA
	jani_is_valid_iosa(*JANIroot_);  // exits if invalid
	bool success = build_IOSA_from_JANI();
	if (!success) {
#ifndef NDEBUG
		figMainLog << "[ERROR] Invalid IOSA model file created !!!\n";
		figTechLog << "[ERROR] Invalid IOSA model file created !!!\n";
#else
		throw_FigException("failed translating JANI-spec file"
						   "to IOSA model syntax");
#endif
	}

	// Dump translated IOSA model to file and exit
	bool dumpToFile( ! (skipFileDump && success));
	std::string iosaFname;
	if (dumpToFile) {
		iosaFname = compose_iosa_fname(janiModelFile, iosaFilename);
		std::ofstream iosaFile(iosaFname);
		ModelPrinter printer(iosaFile);
		IOSAroot_->accept(printer);
		assert(iosaFile.good());
		if (iosaFile.is_open())
			iosaFile.close();
		figTechLog << "Translated IOSA file: " << iosaFname << std::endl;
	}

	return iosaFname;
}


void
JaniTranslator::parse_JANI_model(const std::string& janiModelFile)
{
	std::ifstream janiFile(janiModelFile);
	if (!janiFile.good())
		throw_FigException("failed opening JANI file \"" + janiModelFile + "\"");
	(*JANIroot_) = EMPTY_JSON_OBJ;
	janiFile >> (*JANIroot_);
}


bool
JaniTranslator::build_IOSA_from_JANI()
{
	assert(JANIroot_ != nullptr);
	assert(JANIroot_->isObject());

	const Json::Value& janiFile = *JANIroot_;
	IOSAroot_.reset(new Model);
	Model& iosa = *IOSAroot_;

	// Translate the global declarations
	if (janiFile.isMember("variables") && !janiFile["variables"].empty()) {
		figTechLog << "[ERROR] FIG doesn't handle global variables yet\n";
		return false;
	} else if (janiFile.isMember("constants") && !janiFile["constants"].empty()) {
		if (!janiFile["constants"].isArray())
			throw_FigException("JANI \"constants\" field must be an array");
		for (const Json::Value& c: janiFile["constants"]) {
			auto const_ptr = build_IOSA_constant(c);
			if (nullptr == const_ptr)
				return false;
			iosa.add_decl(const_ptr);
		}
	}


	/// @todo TODO erase debug print
	ModelPrinter printer;
	iosa.accept(printer);


	throw_FigException("prematurely aborted; not ready yet!");


	return build_IOSA_model_from_AST(iosa);
}


shared_ptr<Exp>
JaniTranslator::build_IOSA_expression(const Json::Value& JANIexpr)
{
	shared_ptr<Exp> expr(nullptr);
	// Boolean?
	if (JANIexpr.isBool()) {
		expr = make_shared<BConst>(JANIexpr.asBool());
		goto exit_point;
	}
	// Integer?
	if (JANIexpr.isIntegral()) {
		expr = make_shared<IConst>(JANIexpr.asInt());
		goto exit_point;
	}
	// Floating point?
	if (JANIexpr.isDouble()) {
		expr = make_shared<FConst>(JANIexpr.asFloat());
		goto exit_point;
	}
	// Variable?
	if (JANIexpr.isString()) {
		expr = make_shared<LocExp>(JANIexpr.asString());
		goto exit_point;
	}
	// Operator!
	if ( ! (JANIexpr.isObject() && JANIexpr.isMember("op")))
		throw_FigException("this doesn't look like an operator declaration");
	else {
		auto opStr = JANIexpr["op"].asString();
		auto op = IOSA_operator.at(opStr);
		switch (op)
		{
		case ExpOp::nott:
		{
			auto operand  = build_IOSA_expression(JANIexpr["exp"]);
			if (nullptr == operand)
				figTechLog << "[ERROR] Failed translating operand of '"
						   << opStr << "'\n";
			else
				expr = make_shared<UnOpExp>(op, operand);
		}
			break;
		case ExpOp::implies:
		case ExpOp::andd:
		case ExpOp::orr:
		case ExpOp::eq:
		case ExpOp::neq:
		case ExpOp::lt:
		case ExpOp::gt:
		case ExpOp::le:
		case ExpOp::ge:
		case ExpOp::plus:
		case ExpOp::minus:
		case ExpOp::times:
		case ExpOp::div:
		case ExpOp::mod:
		{
			auto left  = build_IOSA_expression(JANIexpr["left"]);
			auto right = build_IOSA_expression(JANIexpr["right"]);
			if (nullptr == left || nullptr == right)
				figTechLog << "[ERROR] Failed translating operands of '"
						   << opStr << "'\n";
			else
				expr = make_shared<BinOpExp>(op, left, right);
		}
			break;
		default:
			throw_FigException("invalid expression operator: " + opStr);
			break;
		}
	}
	exit_point:
		return expr;
}


shared_ptr<InitializedDecl>
JaniTranslator::build_IOSA_constant(const Json::Value& JANIconst)
{
	assert(JANIconst.isObject());
	assert(JANIconst.isMember("name"));
	assert(JANIconst.isMember("type"));
	// Get identifyer
	auto name = JANIconst["name"].asString();
	if (name.empty()) {
		figTechLog << "[ERROR] Empty name for constant???" << std::endl;
		return nullptr;
	}
	// Get type
	auto iosaType(IOSA_type.at(JANIconst["type"].asString()));
	if (iosaType != Type::tbool &&
		iosaType != Type::tint  &&
		iosaType != Type::tfloat) {
		figTechLog << "[ERROR] Invalid constant type (in declaration of \""
				   << name << "\")" << std::endl;
		return nullptr;
	}
	// Get initialization expression
	if (!JANIconst.isMember("value")) {
		figTechLog << "[ERROR] FIG doesn't handle model parameters yet "
				   << "(in declaration of \"" << name << "\")" << std::endl;
		return nullptr;
	}
	auto iosaExp = build_IOSA_expression(JANIconst["value"]);
	if (nullptr == iosaExp) {
		figTechLog << "[ERROR] Failed parsing constant value expression "
				   << "(in declaration of \"" << name << "\")" << std::endl;
		return nullptr;
	}
	// Build IOSA constant
	return std::make_shared<InitializedDecl>(iosaType, name, iosaExp);
}
















//std::string
//JaniTranslator::build_IOSA_expression(const Json::Value& JANIexpr)
//{
//	std::stringstream ss;
//	assert(JANIexpr.isObject());
//	assert(JANIexpr.isMember("op"));
//	const auto op = JANIexpr["op"].asString();
//	switch (JANI_operator_arity.at(op))
//	{
//	case OpArity::null:
//		ss << IOSA_operator_string.at(op);
//		break;
//	case OpArity::i_unary:
//		assert(JANIexpr.isMember("exp"));
//		ss << "(" << IOSA_operator_string.at(op);
//		ss << build_IOSA_expression(JANIexpr["exp"].asString()) << ")";
//		break;
//	case OpArity::s_unary:
//		assert(JANIexpr.isMember("exp"));  // NOTE: this fails for JANI's "der"
//		ss << IOSA_operator_string.at(op) << "(";
//		ss << build_IOSA_expression(JANIexpr["exp"].asString()) << ")";
//		break;
//	case OpArity::i_binary:
//		assert(JANIexpr.isMember("left"));
//		assert(JANIexpr.isMember("right"));
//		ss << "(" << build_IOSA_expression(JANIexpr["left"].asString());
//		ss << " " << IOSA_operator_string.at(op) << " ";
//		ss << build_IOSA_expression(JANIexpr["right"].asString()) << ")";
//		break;
//	case OpArity::s_binary:
//		assert(JANIexpr.isMember("left"));
//		assert(JANIexpr.isMember("right"));
//		ss << IOSA_operator_string.at(op) << "(";
//		ss << build_IOSA_expression(JANIexpr["left"].asString())  << ",";
//		ss << build_IOSA_expression(JANIexpr["right"].asString()) << ")";
//		break;
//	case OpArity::ternary:
//		throw_FigException("IOSA doesn't have the ternary operator yet");
//		break;
//	default:
//		throw_FigException("invalid operator arity");
//		break;
//	}
//	return ss.str();
//}
//
//
//std::string
//JaniTranslator::build_IOSA_constant(const Json::Value& JANIconst)
//{
//	std::stringstream ss;
//	assert(JANIconst.isObject());
//	assert(JANIconst.isMember("name"));
//	assert(JANIconst.isMember("type"));
//	if (!JANIconst.isMember("value"))
//		throw_FigException("FIG doesn't handle model parameters yet");
//	ss << "const ";
//	ss << IOSA_type.at(JANIconst["type"].asString()) << " ";
//	ss << JANIconst["name"].asString()               << " = ";
//	ss << build_IOSA_expression(JANIconst["value"])  << ";";
//	ss << std::endl;
//	return ss.str();
//}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
