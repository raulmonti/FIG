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
#include <libgen.h>  // basename(), dirname()
// C++
#include <map>
#include <memory>
#include <fstream>
#include <sstream>
#include <algorithm>
// FIG
#include <string_utils.h>
#include <JANI_translator.h>
#include <ModelVerifier.h>
#include <ModelReductor.h>
#include <ModelBuilder.h>
#include <ModelPrinter.h>
#include <ModelAST.h>
#include <ModelTC.h>
#include <ModuleScope.h>
#include <ExpEvaluator.h>
#include <FigException.h>
#include <FigLog.h>
#include <Label.h>
// External code
#include <json.h>

using std::shared_ptr;
using std::make_shared;
using std::string;
using std::vector;
// ADL
using std::begin;
using std::end;


namespace   // // // // // // // // // // // // // // // // // // // // // // //
{

/// IOSA -> JANI type translator
const std::map< Type, std::string > JANI_type =
{
	{ Type::tbool,   "bool"  },
	{ Type::tint,    "int"   },
	{ Type::tfloat,  "real"  },
	{ Type::tclock,  "clock" }
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
	{ ExpOp::mod,     "%" },
	{ ExpOp::floor,   "floor" },
	{ ExpOp::ceil,    "ceil"  },
	{ ExpOp::abs,     "abs"   },
	{ ExpOp::sgn,     "sgn"   },
	{ ExpOp::min,     "min"   },
	{ ExpOp::max,     "max"   },
	{ ExpOp::pow,     "pow"   },
	{ ExpOp::log,     "log"   }
};


/// IOSA -> JANI clock distribution translator
const std::map< DistType, std::string > JANI_distribution =
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


/// IOSA -> JANI clock distribution name translator
const std::map< std::string, std::string > JANI_distribution_string =
{
	{ "uniform",     "Uniform"    },
	{ "exponential", "Exponential"},
	{ "normal",      "Normal"     },
	{ "logNormal",   "LogNormal"  },
	{ "weibull",     "Weibull"    },
	{ "rayleigh",    "Rayleigh"   },
	{ "gamma",       "Gamma"      },
	{ "erlang",      "Erlang"     }
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
	{ "%", ExpOp::mod     },
	{ "floor", ExpOp::floor  },
	{ "ceil",  ExpOp::ceil   },
	{ "abs",   ExpOp::abs    },
	{ "sgn",   ExpOp::sgn    },
	{ "min",   ExpOp::min    },
	{ "max",   ExpOp::max    },
	{ "pow",   ExpOp::pow    },
	{ "log",   ExpOp::log    },
	{ "trc",   ExpOp::floor  },
	{ "der",   ExpOp::invalid}
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
	{ "%",  "%" },
	{ "floor", "floor" },
	{ "ceil",  "ceil"  },
	{ "abs",   "abs"   },
	{ "sgn",   "sgn"   },
	{ "min",   "min"   },
	{ "max",   "max"   },
	{ "pow",   "pow"   },
	{ "log",   "log"   },
	{ "trc",   "floor" },
	{ "der",   ""      }
};


/// JANI -> IOSA clock distribution translator
const std::map< std::string, DistType > IOSA_distribution =
{
	{ "Uniform",     DistType::uniform    },
	{ "Exponential", DistType::exponential},
	{ "Normal",      DistType::normal     },
	{ "LogNormal",   DistType::lognormal  },
	{ "Weibull",     DistType::weibull    },
	{ "Rayleigh",    DistType::rayleigh   },
	{ "Gamma",       DistType::gamma      },
	{ "Erlang",      DistType::erlang     }
};


/// JANI -> IOSA clock distribution name translator
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
	auto filepath(strndup(iosaModelFile.c_str(), 1ul<<7ul));
	root["jani-version"] = 1;
	root["name"] = basename(filepath);
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
	free(filepath);
}


/// Return all sub-expressions of 'expr' where an identifyer from 'ids' occurs
/// as operand of some binary operators from 'bops' (modulo unary operators).
/// Examples:
/// @code
/// relevant_bin_subexpr({a,b}, {&&,||}, "!(a && b)")      == {"a && b"};
/// relevant_bin_subexpr({a,b}, {||,<=}, "!(a && b)")      == {};
/// relevant_bin_subexpr({a},   {&&},    "c || (!a && b)") == {"!a && b"};
/// relevant_bin_subexpr({a,b}, {+,*,>}, "(3*a+2*b) > c")  == {"3*a", "2*b"};
/// @endcode
/// @param ids  Relevant identifiers
/// @param bops Relevant binary operators
/// @param expr Expression to parse
/// @warning Nested sub-expressions are likely to be returned,
///          e.g. ({a,b}, {+,*}, "a+2*b") would yield {"a+2*b","2*b"}
shared_vector< BinOpExp >
relevant_bin_subexpr(const std::vector<std::string>& ids,
					 const std::vector<ExpOp>& bops,
					 shared_ptr<Exp> expr)
{
	shared_vector<BinOpExp> subexprs;
	bool selfRelevant(false), selfIncluded(false);
	// Peel unary operators wrapping up expression and return the juicy pulp
	static auto peel_off = [](shared_ptr<Exp> expr) {
		while (expr->is_unary_operator())
			expr = std::static_pointer_cast<UnOpExp>(expr)->get_argument();
		return expr;
	};
	expr = peel_off(expr);
	if (!expr->is_binary_operator())
		return subexprs;
	auto bexpr = std::static_pointer_cast<BinOpExp>(expr);
	selfRelevant = std::find(begin(bops), end(bops), bexpr->get_operator())
					 != end(bops);
	// Review left
	auto left = peel_off(bexpr->get_first_argument());
	if (left->is_binary_operator()) {
		auto lsubexprs = relevant_bin_subexpr(ids, bops, left);
		subexprs.insert(end(subexprs), begin(lsubexprs), end(lsubexprs));
	} else if (!left->is_constant() && selfRelevant) {
		auto loc = std::static_pointer_cast<LocExp>(left);
		assert(nullptr != loc);  // must be a location we're talking about
		auto id = loc->get_exp_location()->get_identifier();
		if (std::find(begin(ids), end(ids), id) != end(ids)) {
			subexprs.push_back(bexpr);
			selfIncluded = true;
		}
	}
	// Review right
	auto right = peel_off(bexpr->get_second_argument());
	if (right->is_binary_operator()) {
		auto rsubexprs = relevant_bin_subexpr(ids, bops, right);
		subexprs.insert(end(subexprs), begin(rsubexprs), end(rsubexprs));
	} else if (!right->is_constant() && selfRelevant && !selfIncluded) {
		auto loc = std::static_pointer_cast<LocExp>(right);
		assert(nullptr != loc);  // must be a location we're talking about
		auto id = loc->get_exp_location()->get_identifier();
		if (std::find(begin(ids), end(ids), id) != end(ids))
			subexprs.push_back(bexpr);
	}
	return subexprs;
}


/// Traverse expression and return the ID of the first Location matching
/// any of the IDs in the vector of Declarations passed.
/// @param expr Expression to traverse
/// @param IDs  Declarations with the IDs to look for
/// @return First Location identifier in 'exp' matching any ID from IDs,
///         or empty string if none found
std::string
first_ID_in_expr(std::shared_ptr<Exp> expr, const shared_vector<Decl>& IDs)
{
	if (nullptr == expr) {
		return "";
	} else if (expr->is_constant()) {
		return "";
	} else if (expr->is_unary_operator()) {
		return first_ID_in_expr(std::static_pointer_cast<UnOpExp>
								(expr)->get_argument(), IDs);
	} else if (expr->is_binary_operator()) {
		auto bexpr = std::static_pointer_cast<BinOpExp>(expr);
		auto firstID = first_ID_in_expr(bexpr->get_first_argument(), IDs);
		if (firstID.empty())
			firstID = first_ID_in_expr(bexpr->get_second_argument(), IDs);
		return firstID;
	} else {
		auto loc = std::dynamic_pointer_cast<LocExp>(expr);
		assert(nullptr != loc);
		auto id = loc->get_exp_location()->get_identifier();
		auto first = std::find_if(begin(IDs), end(IDs),
								  [&id](const std::shared_ptr<Decl>& d)
								  { return d->get_id() == id; });
		if (end(IDs) != first)
			return id;
		else
			return "";
	}
}


/// Build a IOSA model from given AST (viz. populate the ModelSuite)
/// performing all necessary checks in the process.
/// @return Whether the IOSA model could be successfully built
bool
build_IOSA_model_from_AST(Model& modelAST,
						  bool reduceExpressions = false,
						  bool verifyIOSA = true)
{
	// Check types
	ModelTC typechecker;
	modelAST.accept(typechecker);
	if (typechecker.has_errors()) {
		fig::figTechLog << "[ERROR] Type-checking failed" << std::endl;
		fig::figTechLog << typechecker.get_messages() << std::endl;
		return false;
	}

	// Reduce expressions if requested
	if (reduceExpressions) {
		ModelReductor reductor;
		modelAST.accept(reductor);
		if (reductor.has_errors()) {
			fig::figTechLog << "[ERROR] Expressions reduction failed" << std::endl;
			fig::figTechLog << reductor.get_messages() << std::endl;
			return false;
		}
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

	// Success iff the ModelSuite can be sealed
	ModelSuite::get_instance().seal();
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
JaniTranslator::fresh_label(const Json::Value& hint)
{
	static size_t fresh(0ul);
	if (!hint.isNull() && hint.isString() && !hint.asString().empty())
		return hint.asString();
	else
		return "aa" + std::to_string(fresh++);
}


std::string
JaniTranslator::rv_from(const std::string& clockName, bool force)
{
	auto realVarNamePtr = clk2rv_.find(clockName);
	if (end(clk2rv_) == realVarNamePtr && !force)
		return "";
	else if (end(clk2rv_) == realVarNamePtr && force)
		return REAL_VAR_FROM_CLOCK_PREFIX + clockName;
	else
		return realVarNamePtr->second;
}


const Label&
JaniTranslator::sync_label(const std::string& module, const std::string& label)
{
	static const Label TAU(Label::make_tau());
	auto syncLabelPtr = syncLabel_.find(std::make_pair(module,label));
	if (end(syncLabel_) == syncLabelPtr)
		return TAU;
	else
		return syncLabelPtr->second;
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
												 false,  // let constants IDs alone
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
	JANIobj["distribution"] = JANI_distribution.at(clockDist->get_type()).c_str();
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

		// Synchronizing elements: one input-enable per module input
		tmp["automaton"] = pair.first.c_str();
		tmp["input-enable"] = EMPTY_JSON_ARR;
		for (const std::string& inputLabel: pair.second.first)
			tmp["input-enable"].append(inputLabel.c_str());
		if (tmp["input-enable"].empty())
			tmp.removeMember("input-enable");
		JANIobj["elements"].append(tmp);

		// Synchronization vectors: one per module output
		for (const std::string& outputLabel: pair.second.second) {
			tmp.clear();
			build_JANI_sync_vector(outputLabel, tmp["synchronise"]);
			if (!tmp["synchronise"].empty()) {
				tmp["result"] = outputLabel.c_str();  // we could omit this
				JANIobj["syncs"].append(tmp);
			}
		}
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
		JANIarr.clear();
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

	// Parse all properties
	if (node->has_props()) {
		JANIfield_ = make_shared<Json::Value>(Json::arrayValue);
		for (auto prop_ptr: node->get_props())
			prop_ptr->accept(*this);
		(*JANIroot_)["properties"] = *JANIfield_;
	}
}



void
JaniTranslator::visit(shared_ptr<RangedDecl> node)
{
	auto JANIobj = EMPTY_JSON_OBJ, type = EMPTY_JSON_OBJ;
	const auto tmp = *JANIfield_;

    assert( ! (has_errors() || has_warnings()) );
    assert(!node->is_constant());

	// Type
    type["kind"] = "bounded";
    type["base"] = "int";
	(*JANIfield_) = EMPTY_JSON_OBJ;
	node->get_lower_bound()->accept(*this);
	type["lower-bound"] = *JANIfield_;
	(*JANIfield_) = EMPTY_JSON_OBJ;
	node->get_upper_bound()->accept(*this);
	type["upper-bound"] = *JANIfield_;

	// Declaration
	(*JANIfield_) = EMPTY_JSON_OBJ;
	node->get_init()->accept(*this);
	JANIobj["initial-value"] = *JANIfield_;
	JANIobj["name"] = node->get_id();
	JANIobj["type"] = type;

    if (has_errors() || has_warnings())
		throw_FigException("error translating declaration: " + get_messages());

    // Store translated data in corresponding field
	*JANIfield_ = tmp;
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
		JANIfield_->append(node->get_value());
	else
		(*JANIfield_) = node->get_value();
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
	if (!label.empty()) {
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


void
JaniTranslator::visit(shared_ptr<TransientProp> node)
{
	static size_t fresh(0ul);
	auto JANIobj(EMPTY_JSON_OBJ), transient(EMPTY_JSON_OBJ);
	const auto tmp = *JANIfield_;
	// Mandatory fields (name and filter)
	JANIobj["name"] = "Transient_" + std::to_string(fresh++);
	JANIobj["expression"] = EMPTY_JSON_OBJ;
	JANIobj["expression"]["op"] = "filter";
	JANIobj["expression"]["fun"] = "max";
	JANIobj["expression"]["states"] = EMPTY_JSON_OBJ;
	JANIobj["expression"]["states"]["op"] = "initial";
	// Until probability
	transient["op"] = "Pmax";
	transient["exp"] = EMPTY_JSON_OBJ;
	transient["exp"]["op"] = "U";
	(*JANIfield_) = EMPTY_JSON_OBJ;
	node->get_left()->accept(*this);
	transient["exp"]["left"] = *JANIfield_;
	(*JANIfield_) = EMPTY_JSON_OBJ;
	node->get_right()->accept(*this);
	transient["exp"]["right"] = *JANIfield_;
	JANIobj["expression"]["values"] = transient;
	// Store translated data in corresponding field
	*JANIfield_ = tmp;
	if (JANIfield_->isArray())
		JANIfield_->append(JANIobj);
	else
		(*JANIfield_) = JANIobj;
}


void
JaniTranslator::visit(shared_ptr<RateProp> node)
{
	static size_t fresh(0ul);
	auto JANIobj(EMPTY_JSON_OBJ), rate(EMPTY_JSON_OBJ);
	const auto tmp = *JANIfield_;
	// Mandatory fields (name and filter)
	JANIobj["name"] = "Rate_" + std::to_string(fresh++);
	JANIobj["expression"] = EMPTY_JSON_OBJ;
	JANIobj["expression"]["op"] = "filter";
	JANIobj["expression"]["fun"] = "max";
	JANIobj["expression"]["states"] = EMPTY_JSON_OBJ;
	JANIobj["expression"]["states"]["op"] = "initial";
	// Long run probability
	rate["op"] = "Smax";
	(*JANIfield_) = EMPTY_JSON_OBJ;
	node->get_expression()->accept(*this);
	rate["exp"] = *JANIfield_;
	JANIobj["expression"]["values"] = rate;
	// Store translated data in corresponding field
	*JANIfield_ = tmp;
	if (JANIfield_->isArray())
		JANIfield_->append(JANIobj);
	else
		(*JANIfield_) = JANIobj;
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
	const bool translated = build_IOSA_from_JANI();
	if (!translated) {
#ifndef NDEBUG
		figMainLog << "[ERROR] Invalid IOSA model file created !!!\n";
		figTechLog << "[ERROR] Invalid IOSA model file created !!!\n";
#else
		throw_FigException("failed translating JANI-spec file"
						   "to IOSA model syntax");
#endif
	}

	// Dump translated IOSA model to file and exit
	bool dumpToFile( ! (skipFileDump && translated));
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
	// Fast consistency check and initializations
	assert(nullptr != JANIroot_);
	assert(JANIroot_->isObject());
	auto janiModel = *JANIroot_;
	jani_is_valid_iosa(janiModel);  // exits if invalid
	IOSAroot_ = make_shared<Model>();
	Model& iosaModel = *IOSAroot_;

	// Translate global declarations
	if (janiModel.isMember("variables") && !janiModel["variables"].empty()) {
		figTechLog << "[ERROR] FIG doesn't handle global variables yet\n";
		return false;
	} else if (janiModel.isMember("constants") && !janiModel["constants"].empty()) {
		if (!janiModel["constants"].isArray())
			throw_FigException("JANI \"constants\" field must be an array");
		for (const auto& c: janiModel["constants"]) {
			auto const_ptr = build_IOSA_constant(c);
			if (nullptr == const_ptr)
				return false;
			iosaModel.add_decl(const_ptr);
		}
	}

	// Interpret and build IOSA I/O synchronization if possible
	bool compatible = test_and_build_IOSA_synchronization(janiModel["system"],
														  janiModel["automata"]);
	if (!compatible) {
		figTechLog << "[ERROR] JANI model doesn't seem to be compatible "
				   << "with the IOSA formalism. Have a good day sir.\n";
		return false;
	}

	// Translate each module
	shared_ptr<ModuleAST> (JaniTranslator::*build_IOSA_module)(const Json::Value&);
	if ("ctmc" == janiModel["type"].asString())
		build_IOSA_module = &JaniTranslator::build_IOSA_module_from_CTMC;
	else if ("sta" == janiModel["type"].asString())
		build_IOSA_module = &JaniTranslator::build_IOSA_module_from_STA;
	else
		return false;
	for (const auto& a: janiModel["automata"]) {
		auto module_ptr = (this->*build_IOSA_module)(a);
		if (nullptr == module_ptr) {
			figTechLog << "[ERROR] JANI automaton \"" << a["name"].asString()
					   << "\" couldn't be translated to a IOSA module.\n";
			return false;
		}
		iosaModel.add_module(module_ptr);
	}

	// Translate the properties we can
	// Avoid exact duplicates (not so unlikely from non-deterministic models)
	std::vector<std::string> translatedProps;
	for (const auto& p: janiModel["properties"]) {
		static std::stringstream prop_str;
		static ModelPrinter printer(prop_str);
		auto prop_ptr = build_IOSA_property(p);
		if (nullptr == prop_ptr) {
			figTechLog << "[WARNING] JANI property \"" << p["name"].asString()
					   << "\" doesn't seem to have a IOSA equivalent; skipping it.\n";
			continue;
		}
		prop_ptr->accept(printer);
		if (std::find(begin(translatedProps), end(translatedProps), prop_str.str())
				!= end(translatedProps)) {
			figTechLog << "[WARNING] already translated a JANI property equivalent "
					   << "to \"" << p["name"].asString() << "\"; skipping it.\n";
			figTechLog << "[NOTE] IOSA is a (weakly) deterministic language. "
					   << "Therefore non-det constructs like e.g. 'Pmin' and "
					   << "'Pmax' are equivalent when translated to IOSA.\n";
		} else {
			iosaModel.add_prop(prop_ptr);
			translatedProps.push_back(prop_str.str());
			std::stringstream().swap(prop_str);
		}
	}
	if (0 >= iosaModel.num_props()) {
		figTechLog << "[ERROR] Resulting IOSA model has no properties, aborting.\n";
		return false;
	}

	return ::build_IOSA_model_from_AST(iosaModel);
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
	// Distribution sampling?
	if (JANIexpr.isObject() && JANIexpr.isMember("distribution")) {
		figTechLog << "[ERROR] Clock samplings need build_IOSA_clock_reset()\n";
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
		case ExpOp::floor:
		case ExpOp::ceil:
		case ExpOp::abs:
		case ExpOp::sgn:
		{
			auto operand  = build_IOSA_expression(JANIexpr["exp"]);
			if (nullptr == operand)
				figTechLog << "[ERROR] Failed translating operand of JANI "
						   << "operator '" << opStr << "'\n";
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
		case ExpOp::min:
		case ExpOp::max:
		case ExpOp::pow:
		case ExpOp::log:
		{
			auto left  = build_IOSA_expression(JANIexpr["left"]);
			auto right = build_IOSA_expression(JANIexpr["right"]);
			if (nullptr == left || nullptr == right)
				figTechLog << "[ERROR] Failed translating operands of JANI "
						   << "operator '" << opStr << "'\n";
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
	Type iosaType(Type::tunknown);
	try {
		iosaType = IOSA_type.at(JANIconst["type"].asString());
		if (iosaType != Type::tbool &&
			iosaType != Type::tint  &&
			iosaType != Type::tfloat) {
			figTechLog << "[ERROR] Invalid constant type (in declaration of \""
					   << name << "\")" << std::endl;
			return nullptr;
		}
	} catch (std::out_of_range&) {
		throw_FigException("unrecognized JANI type (\"" +
						   JANIconst["type"].asString() + "\") in "
						   "declaration of \"" + name + "\"");
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
	return std::make_shared<InitializedDecl>(iosaType, name, iosaExp, true);
}


shared_ptr<Decl>
JaniTranslator::build_IOSA_variable(const Json::Value& JANIvar)
{
	assert(JANIvar.isObject());
	assert(JANIvar.isMember("name"));
	assert(JANIvar.isMember("type"));
	// Get identifyer
	auto name = JANIvar["name"].asString();
	if (name.empty()) {
		figTechLog << "[ERROR] Empty name for variable???" << std::endl;
		return nullptr;
	}
	if (JANIvar.isMember("transient")) {
		figTechLog << "[ERROR] FIG doesn't support transient variables yet "
				   << "(in declaration of \"" << name << "\")\n";
		return nullptr;
	}
	// Build according to type
	auto janiType(JANIvar["type"]);
	auto janiInit(JANIvar.get("initial-value", Json::nullValue));
	if ( ! (janiType.isString() || janiType.isObject()) )
		 throw_FigException("unrecognized JANI type (in declaration of "
							"variable \"" + name + "\")");
	if (janiType.isObject())
		return build_IOSA_ranged_variable(name, janiType, janiInit);
	else if (janiType.asString() == "bool")
		return build_IOSA_boolean_variable(name, janiInit);
	else if (janiType.asString() == "clock")
		return make_shared<ClockDecl>(name);
	else if (janiType.asString() == "real")
		return make_shared<InitializedDecl>(Type::tfloat, name, nullptr);
	else
		figTechLog << "[ERROR] FIG doesn't support variables of type \""
				   << janiType.asString() << "\" (in declaration of \""
				   << name << "\")\n";
	return nullptr;
}


shared_ptr<Decl>
JaniTranslator::build_IOSA_ranged_variable(const std::string& varName,
										   const Json::Value& varType,
										   const Json::Value& varInit)
{
	assert(varType.isObject());
	// Verify data consistency
	if ( ! (varType.isMember("kind") && varType.isMember("base")) )
		throw_FigException("unrecognized JANI type (in declaration of \""
						   + varName + "\")");
	else if (varType["kind"].asString() != "bounded")
		throw_FigException("unrecognized JANI type \"" + varType["kind"].asString()
						  + "\" (in declaration of \"" + varName + "\")");
	else if (varType["base"].asString() != "int") {
		figTechLog << "[ERROR] FIG doesn't support variables of type \""
				   << varType["base"].asString()
				   << "\" yet (in declaration of \"" << varName << "\")\n";
		return nullptr;
	} else if ( ! (varType.isMember("lower-bound") &&
				   varType.isMember("upper-bound")) ) {
		figTechLog << "[ERROR] FIG doesn't support bounded variables "
				   << "with undefined upper/lower bounds "
				   << "(in declaration of \"" << varName << "\")\n";
		return nullptr;
	}
	// Build bounds
	auto lower(build_IOSA_expression(varType["lower-bound"]));
	auto upper(build_IOSA_expression(varType["upper-bound"]));
	// Build ranged variable
	if (varInit.isNull())
		return make_shared<RangedDecl>(varName, lower, upper);
	else
		return make_shared<RangedDecl>(varName, lower, upper,
									   build_IOSA_expression(varInit));
}


shared_ptr<Decl>
JaniTranslator::build_IOSA_boolean_variable(const std::string& varName,
											const Json::Value& varInit)
{
	if (varInit.isNull())
		return make_shared<InitializedDecl>(Type::tbool, varName,
											make_shared<BConst>());
	else
		return make_shared<InitializedDecl>(Type::tbool, varName,
											build_IOSA_expression(varInit));
}


shared_ptr<ClockReset>
JaniTranslator::build_IOSA_clock_reset(const std::string& clockName,
									   const Json::Value& JANIdsamp)
{
	assert(JANIdsamp.isObject());
	if ( ! (JANIdsamp.isMember("distribution")   &&
			JANIdsamp.isMember("args")           &&
			JANIdsamp["distribution"].isString() &&
			JANIdsamp["args"].isArray()) )
		throw_FigException("assignment to real var mapped to clock \"" + clockName
						   + "\" doesn't look like a distribution sampling");
	// Check distribution for compatibility
	const auto JANIdist = JANIdsamp["distribution"].asString();
	const auto distType = IOSA_distribution.find(JANIdist);
	if (distType == end(IOSA_distribution)) {
		figTechLog << "[ERROR] FIG doesn't handle the \"" << JANIdist
				   << "\" distribution yet.\n";
		return nullptr;
	}
	// Retrieve the distribution parameters
	shared_vector<Exp> params;
	params.reserve(JANIdsamp["args"].size());
	for (const auto& arg: JANIdsamp["args"]) {
		auto param = build_IOSA_expression(arg);
		if (nullptr == param) {
			figTechLog << "[ERROR] Failed to translate an argument from the \""
					   << JANIdist << "\" distribution.\n";
			return nullptr;
		}
		params.push_back(param);
	}
	// Build distribution
	shared_ptr<Dist> IOSAdist(nullptr);
	switch (distType->second)
	{
	case DistType::exponential:
	case DistType::rayleigh:
		if (params.empty()) {
			figTechLog << "[ERROR] Missing parameter for \"" << JANIdist
					   << "\" distribution (one required).\n";
			return nullptr;
		}
		IOSAdist = make_shared<SingleParameterDist>(distType->second, params[0]);
		break;
	case DistType::uniform:
	case DistType::normal:
	case DistType::lognormal:
	case DistType::weibull:
	case DistType::erlang:
	case DistType::gamma:
		if (2ul > params.size()) {
			figTechLog << "[ERROR] Missing parameters for \"" << JANIdist
					   << "\" distribution (two required).\n";
			return nullptr;
		}
		IOSAdist = make_shared<MultipleParameterDist>(distType->second,
													  params[0], params[1]);
		break;
	default:
		figTechLog << "[ERROR] Unhandled distribution type: "
				   << static_cast<int>(distType->second) << std::endl;
		break;
	}
	return make_shared<ClockReset>(make_shared<Location>(clockName), IOSAdist);
}



shared_ptr<ClockReset>
JaniTranslator::build_exponential_clock(const Json::Value& JANIedge,
										const std::string& moduleName)
{
	static size_t clkCounter(0ul);
	std::string edgeLabel;
	enum { input, output, tau } edgeType(tau);
	assert(JANIedge.isObject());
	assert( ! (has_errors() || has_warnings()) );
	// Retrieve rate parameter
	if (!JANIedge.isMember("rate"))
		throw_FigException("this edge doesn't have a rate expression!");
	auto rate = build_IOSA_expression(JANIedge["rate"]["exp"]);
	if (nullptr == rate) {
		figTechLog << "[ERROR] FIG failed to translate the rate expression "
				   << "of an edge in automaton \"" << moduleName << "\"\n";
		throw_FigException("internal error, see tech log");
	}
	// Classify edge as input, output, or tau
	if (JANIedge.isMember("action")) {
		edgeLabel = JANIedge["action"].asString();
		auto syncLabel = sync_label(moduleName, edgeLabel);
		if (syncLabel.is_input())
			edgeType = input;
		else if (syncLabel.is_output())
			edgeType = output;
		else if (!edgeLabel.empty())
			throw_FigException("label \"" + edgeLabel +"\" in automaton \"" +
							   moduleName + "\" wasn't classified as I/O");
	}
	// Create edge's clock
	switch (edgeType)
	{
	case input:
		inputRatesCTMC_.push_back(std::make_tuple(moduleName, edgeLabel, rate));
		return nullptr;
	case output:
	case tau:
	{
		auto clockDist = std::make_shared<SingleParameterDist>(DistType::exponential, rate);
		auto clockLoc = std::make_shared<Location>("clk" + std::to_string(clkCounter++));
		return make_shared<ClockReset>(clockLoc, clockDist);
	}
	default:
		figTechLog << "[ERROR] Unhandled edge type.\n";
		return nullptr;
	}
}


bool
JaniTranslator::map_clocks_to_rvs(const Json::Value& moduleLocations)
{
	assert(moduleLocations.isArray());

	// Binary operators allowed to "combine" clocks with real vars
	static const std::vector<ExpOp> ops = {
		ExpOp::lt, ExpOp::le, ExpOp::gt, ExpOp::ge };
	auto op_str = Operator::operator_string;

	// Gather the names of the clocks and real vars we must pair up
	std::vector<std::string> cids, rvids, allids;
	cids.reserve(clk2rv_.size());
	rvids.reserve(rv2dist_.size());
	allids.reserve(clk2rv_.size() + rv2dist_.size());
	for (const auto& p: clk2rv_) {
		cids.push_back(p.first);
		allids.push_back(p.first);
	}
	for (const auto& p: rv2dist_) {
		rvids.push_back(p.first);
		allids.push_back(p.first);
	}

	// Review all time-progress invariants, where the clocks and the real vars
	// should appear compared to each other (these comparisons create the map)
	for (const auto& loc: moduleLocations) {
		assert(loc.isObject());
		if (!loc.isMember("time-progress"))
			continue;
		assert(loc["time-progress"].isMember("exp"));
		const std::string errMsg("[ERROR] Invalid use of clock/real variable in "
			"time-progress invariant of location \"" + loc["name"].asString() + "\"");
		auto locExpr = build_IOSA_expression(loc["time-progress"]["exp"]);
		for (auto comparison: relevant_bin_subexpr(allids, ops, locExpr)) {
			// Check this is a valid clock/real_var comparison
			auto left = std::dynamic_pointer_cast<LocExp>(comparison->get_first_argument());
			auto right = std::dynamic_pointer_cast<LocExp>(comparison->get_second_argument());
			if (nullptr == left || nullptr == right) {
				figTechLog << errMsg << " (in comparison with operator '"
						   << op_str(comparison->get_operator()) << "')\n";
				return false;
			}
			// Build the map if possible
			std::string clk, rv;
			clk = left->get_exp_location()->get_identifier();
			rv = right->get_exp_location()->get_identifier();
			if (std::find(begin(cids), end(cids), clk) == end(cids))
				rv.swap(clk);  // woops
			if (std::find(begin(rvids), end(rvids), rv) == end(rvids) ||
				std::find(begin(cids), end(cids), clk) == end(cids)) {
				figTechLog << errMsg << " (variables \"" << clk << "\" and \""
						   << rv << "\" not found)\n";
				return false;
			} else if (!clk2rv_[clk].empty() && rv != clk2rv_[clk]) {
				figTechLog << errMsg << " (clock \"" << clk << "\" is used "
						   << "with two real variables: \"" << rv << "\" "
						   << "and \"" << clk2rv_[clk] << "\")\n";
				return false;
			} else {
				clk2rv_[clk] = rv;
			}
		}
	}

	// Check we built a bijection
	for (const auto& p: clk2rv_) {
		auto clk = p.first;
		auto rv = p.second;
		if (rv.empty()) {
			figTechLog << "[ERROR] Clock \"" << clk << "\" has no real "
					   << "variable (and hence no distribution) associated.\n";
			return false;
		} else if (1ul != std::count(begin(rvids), end(rvids), rv)) {
			figTechLog << "[ERROR] Clock \"" << clk << "\" was mapped before "
					   << "to a real variable other than \"" << rv << "\"\n";
			return false;
		}
		rvids.erase(std::find(begin(rvids), end(rvids), rv));
	}

	return true;
}


bool
JaniTranslator::map_rv_to_dist(const std::string& rvName,
							   const Json::Value& JANIdsamp)
{
	// Find matching clock
	auto pair = std::find_if(begin(clk2rv_), end(clk2rv_),
							 [&rvName](const std::pair<string,string>& p)
							 { return p.second == rvName; } );
	assert(pair != end(clk2rv_));
	// Translate distribution sampling
	auto reset = build_IOSA_clock_reset(pair->first, JANIdsamp);
	if (nullptr == reset) {
		figTechLog << "[ERROR] Failed translating distribution sampling "
				   << "for real variable \"" << rvName << "\"\n";
		return false;
	}
	// Map; if there was any previous sampling then check compatibility
	if (nullptr == rv2dist_[rvName]) {
		rv2dist_[rvName] = reset;
	} else {
		const string errMsg("[ERROR] IOSA requires a single distribution per clock");
		auto oldDist = rv2dist_[rvName]->get_dist(),
			 newDist = reset->get_dist();
		if (oldDist->get_type() != newDist->get_type()) {
			figTechLog << errMsg << " (real var \"" << rvName << "\" is "
					   << "sampled from \"" << JANI_distribution.at(oldDist->get_type())
					   << "\" and also \"" << JANI_distribution.at(newDist->get_type())
					   << "\")\n";
			return false;
		} else if (oldDist->num_parameters() != newDist->num_parameters()) {
			figTechLog << errMsg << " (real var \"" << rvName << "\" "
					   << "is sampled from several distributions with "
					   << "different number of arguments)\n";
			return false;
		// FIXME: following checks may generate unnecessary trouble
		} else if (oldDist->has_single_parameter()) {
			assert(newDist->has_single_parameter());
			auto oldParamExpr = oldDist->to_single_parameter()->get_parameter();
			auto newParamExpr = newDist->to_single_parameter()->get_parameter();
			if (oldParamExpr->get_type() != newParamExpr->get_type()) {
				figTechLog << errMsg << " (real var \"" << rvName << "\" "
						   << "is sampled from several single-parameter "
						   << "distributions with different arguments)\n";
				return false;
			}
		} else {
			assert(oldDist->has_multiple_parameters());
			assert(newDist->has_multiple_parameters());
			auto oldParamExpr = oldDist->to_multiple_parameter()->get_first_parameter();
			auto newParamExpr = newDist->to_multiple_parameter()->get_first_parameter();
			if (oldParamExpr->get_type() != newParamExpr->get_type()) {
				figTechLog << errMsg << " (real var \"" << rvName << "\" "
						   << "is sampled from several multiple-parameter "
						   << "distributions with different first arguments)\n";
				return false;
			}
			oldParamExpr = oldDist->to_multiple_parameter()->get_second_parameter();
			newParamExpr = newDist->to_multiple_parameter()->get_second_parameter();
			if (oldParamExpr->get_type() != newParamExpr->get_type()) {
				figTechLog << errMsg << " (real var \"" << rvName << "\" "
						   << "is sampled from several multiple-parameter "
						   << "distributions with different second arguments)\n";
				return false;
			}
		}
	}
	return true;
}


bool
JaniTranslator::test_and_build_IOSA_synchronization(const Json::Value& JANIcomposition,
													const Json::Value& JANIautomata)
{
	assert(JANIcomposition.isObject());
	assert(JANIcomposition.isMember("elements"));
	assert(JANIcomposition["elements"].isArray());
	assert(JANIautomata.isArray());

	std::vector< std::string > automata;
	automata.reserve(JANIcomposition["elements"].size());
	modulesLabels_.clear();
	modelLabels_.clear();
	syncLabel_.clear();

	// Interpret I/O from input-enable declarations:
	// IOSA modules are input-enabled in all its input labels, so
	// !input-enable => output
	for (const auto& e: JANIcomposition["elements"]) {
		automata.emplace_back(e["automaton"].asString());  // automaton name
		if (!e.isMember("input-enable"))
			continue;
		auto& labels = modulesLabels_[automata.back()];
		for (const auto& act: e["input-enable"])
			labels.first.emplace(act.asString());  // input
			// NOTE: this action label *could* also be an output, but fuck that
	}
	for (const auto& a: JANIautomata) {
		assert(a.isMember("edges") && a["edges"].isArray());
		auto& labels = modulesLabels_[a["name"].asString()];
		for (const auto& e: a["edges"]) {
			if (!e.isMember("action"))
				continue;
			auto act = e["action"].asString();
			modelLabels_.emplace(act);
			if (labels.first.find(act) == end(labels.first))
				labels.second.emplace(act);  // output
		}
	}

	// Build modules synchronization as specified in the sync vectors;
	// abort if incompatible with IOSA broadcast synchronization
	auto allLabels = modulesLabels_;
	for (const auto& vec: JANIcomposition["syncs"]) {
		std::string vecOutput;  // the output of this sync vector
		auto freshLabel = fresh_label(vec.get("result",Json::nullValue));
		assert(vec.isObject() && vec.isMember("synchronise"));
		auto syncv = vec["synchronise"];  // one sync vector
		assert(syncv.isArray());
		for (unsigned i = 0u ; i < syncv.size() ; i++) {
			if (syncv[i].isNull())
				continue;
			// Mark action label as "used" in this module
			auto label = syncv[i].asString();
			auto& moduleInputs = allLabels[automata[i]].first;
			auto& moduleOutputs = allLabels[automata[i]].second;
			const bool isInput = 0ul < moduleInputs.erase(label);
			const bool isOutput = 0ul < moduleOutputs.erase(label);
			syncLabel_.emplace(std::make_pair(automata[i],label),
							   isInput ? Label::make_input(freshLabel)
									   : Label::make_output(freshLabel));
			// Check for compatibility with IOSA broadcast synchronization
			if (isOutput && vecOutput.empty()) {
				assert(!label.empty());
				vecOutput = label;
			} else if (isOutput) {
				figTechLog << "[ERROR] Synchronization pattern incompatible "
						   << "with IOSA broadcast: there can be only one "
						   << "output per sync vector (\"" << vecOutput
						   << "\" and \"" << label << "\" appear together)\n";
				figTechLog << "[NOTE] I/O is intepreted from the composition "
						   << "specification: all \"input-enable\" actions in "
						   << "an automaton will be inputs, the rest outputs.\n";
				return false;
			} else if ( ! (isInput || isOutput) ) {
				figTechLog << "[ERROR] Synchronization pattern incompatible "
						   << "with IOSA broadcast: each label from each auto"
						   << "maton can appear in a single sync vector (\""
						   << label << "\" appears in two)\n";
				return false;
			}
		}
	}

	// Labels not used in the synchronization vectors mustn't synchronize
	for (const auto& p: allLabels) {
		for (const auto& input: p.second.first)
			syncLabel_.emplace(std::make_pair(p.first,input), Label::make_tau());
		for (const auto& output: p.second.second)
			syncLabel_.emplace(std::make_pair(p.first,output), Label::make_tau());
	}

	return true;
}


shared_ptr<ModuleAST>
JaniTranslator::build_IOSA_module_from_CTMC(const Json::Value& JANIautomaton)
{
	assert(JANIautomaton.isMember("edges"));
	assert(JANIautomaton["edges"].isArray());

	const size_t NUM_EDGES(JANIautomaton["edges"].size());
	shared_ptr<ModuleAST> module(make_shared<ModuleAST>());
	module->set_name(JANIautomaton["name"].asString());

	// Local variables
	if (JANIautomaton.isMember("variables")) {
		if (!JANIautomaton["variables"].isArray())
			throw_FigException("JANI automaton's \"variables\" field must be an array");
		for (const auto& v: JANIautomaton["variables"]) {
			auto var = build_IOSA_variable(v);
			if (nullptr == var)
				return nullptr;
			else if (var->get_type() != Type::tbool &&
					 var->get_type() != Type::tint) {
				figTechLog << "[ERROR] Unsupported variable type from CTMC "
						   << "automaton (\"" << var->get_id() << "\" in \""
						   << module->get_name() << "\")\n";
				return nullptr;
			}
			module->add_decl(var);
		}
		if (JANIautomaton.isMember("restrict-initial"))
			figTechLog << "[WARNING] Ignoring \"restrict-initial\" field\n";

		/// @todo TODO verify restrict-initial field for IOSA compatibility
		///            and translate to variables initialization
	}

	// Exponential clocks, one per output/tau edge
	shared_vector< ClockReset > allClocks, notNullClocks(NUM_EDGES);
	allClocks.reserve(NUM_EDGES);
	for (const auto& edge: JANIautomaton["edges"]) {
		auto edgeClock = build_exponential_clock(edge, module->get_name());
		allClocks.push_back(edgeClock);  // nullptr for input edges
	}
	std::copy_if(begin(allClocks), end(allClocks), begin(notNullClocks),
				 [](shared_ptr<ClockReset> ptr) { return nullptr != ptr; });
	notNullClocks.erase(std::remove(begin(notNullClocks), end(notNullClocks), nullptr),
						end(notNullClocks));

	// Transitions
	for (unsigned i = 0u ; i < NUM_EDGES ; i++) {
		const auto& edge = JANIautomaton["edges"][i];
		shared_ptr<Location> edgeClock =
				nullptr == allClocks[i] ? nullptr
										: allClocks[i]->get_effect_location();
		auto trans = build_IOSA_transition_from_CTMC(edge,
													 module->get_name(),
													 module->get_local_decls(),
													 edgeClock,
													 notNullClocks);
		if (nullptr == trans)
			return nullptr;
		module->add_transition(trans);
	}

	return module;
}


shared_ptr<TransitionAST>
JaniTranslator::build_IOSA_transition_from_CTMC(const Json::Value& JANIedge,
												const std::string& moduleName,
												const shared_vector<Decl>& moduleDecls,
												std::shared_ptr<Location> edgeClock,
												const shared_vector<ClockReset>& allClocks)
{
	assert(JANIedge.isObject());
	assert(JANIedge.isMember("rate"));
	assert(JANIedge.isMember("destinations"));
	// Build precondition
	shared_ptr<Exp> pre;
	if (JANIedge.isMember("guard"))
		pre = build_IOSA_expression(JANIedge["guard"]["exp"]);
	else
		pre = make_shared<BConst>(true);  // empty guard => "true"
	// Build postcondition
	auto pos = build_IOSA_postcondition(JANIedge["destinations"], moduleDecls);
	pos.insert(end(pos), begin(allClocks), end(allClocks));  // reset all clocks
	// Build transition
	if (nullptr == edgeClock) {
		// Input
		assert(JANIedge.isMember("action"));
		auto label = sync_label(moduleName, JANIedge["action"].asString());
		assert(label.is_input());
		return make_shared<InputTransition>(label.str, pre, pos);
	} else if (JANIedge.isMember("action")) {
		// Output or Tau
		auto label = sync_label(moduleName, JANIedge["action"].asString());
		if (label.is_output())
			return make_shared<OutputTransition>(label.str, pre, pos, edgeClock);
		else
			return make_shared<TauTransition>(pre, pos, edgeClock);
	} else {
		// Tau
		return make_shared<TauTransition>(pre, pos, edgeClock);
	}
}


std::shared_ptr<ModuleAST>
JaniTranslator::build_IOSA_module_from_STA(const Json::Value& JANIautomaton)
{
	assert(JANIautomaton.isMember("edges"));
	assert(JANIautomaton["edges"].isArray());

	shared_ptr<ModuleAST> module(make_shared<ModuleAST>());
	module->set_name(JANIautomaton["name"].asString());
	clk2rv_.clear();
	rv2dist_.clear();

	// Local variables
	if (JANIautomaton.isMember("variables")) {
		if (!JANIautomaton["variables"].isArray())
			throw_FigException("JANI automaton's \"variables\" field must be an array");
		for (const auto& v: JANIautomaton["variables"]) {
			auto var = build_IOSA_variable(v);
			if (nullptr == var)
				return nullptr;
			else if (var->get_type() == Type::tfloat)
				rv2dist_[var->get_id()] = nullptr;
			else {
				if (var->get_type() == Type::tclock)
					clk2rv_[var->get_id()] = "";
				module->add_decl(var);
			}
		}
		if (JANIautomaton.isMember("restrict-initial"))
			figTechLog << "[WARNING] Ignoring \"restrict-initial\" field\n";

		/// @todo TODO verify restrict-initial field for IOSA compatibility
		///            and translate to variables initialization
	}

	// Map clocks to real vars by intepreting the time-progress invariants
	if (!map_clocks_to_rvs(JANIautomaton["locations"])) {
		figTechLog << "[ERROR] Clocks manipulation in the STA automaton \""
				   << module->get_name() << "\" isn't IOSA-compatible.\n";
		return nullptr;
	}

	// Transitions
	for (const auto& edge: JANIautomaton["edges"]) {
		auto trans = build_IOSA_transition_from_STA(edge,
													module->get_name(),
													module->get_local_decls());
		if (nullptr == trans) {
			figTechLog << "[ERROR] Failed translating an edge in the STA auto"
					   << "maton \"" << module->get_name() << "\" to IOSA.\n";
			return nullptr;
		}
		module->add_transition(trans);
	}

	return module;
}

std::shared_ptr<TransitionAST>
JaniTranslator::build_IOSA_transition_from_STA(const Json::Value& JANIedge,
											   const std::string& moduleName,
											   const shared_vector<Decl>& moduleDecls)
{
	assert(JANIedge.isObject());
	assert(JANIedge.isMember("destinations"));
	// Distinguish clocks from the other vars
	shared_vector<Decl> moduleVars, moduleClocks;
	moduleVars.reserve(moduleDecls.size());
	moduleClocks.reserve(moduleDecls.size());
	for (auto decl: moduleDecls) {
		if (decl->get_type() == Type::tclock)
			moduleClocks.emplace_back(decl);
		else
			moduleVars.emplace_back(decl);
	}
	// Build precondition
	std::shared_ptr<Exp> pre;
	std::shared_ptr<Location> edgeClock;
	if (!JANIedge.isMember("guard"))
		pre = make_shared<BConst>(true);  // empty guard => "true"
	else
		std::tie(pre, edgeClock) =
				build_IOSA_precondition_from_STA(JANIedge["guard"]["exp"], moduleClocks);
	if (nullptr == pre) {
		figTechLog << "[ERROR] Failed translating the guard of an edge "
				   << "in the STA automaton \"" << moduleName << "\".\n";
		return nullptr;
	}
	// Build postcondition
	auto pos = build_IOSA_postcondition(JANIedge["destinations"],
										moduleVars, moduleClocks);
	// Build transition
	if (nullptr == edgeClock) {
		// Input
		assert(JANIedge.isMember("action"));
		auto label = sync_label(moduleName, JANIedge["action"].asString());
		assert(label.is_input());
		return make_shared<InputTransition>(label.str, pre, pos);
	} else if (JANIedge.isMember("action")) {
		// Output or Tau
		auto label = sync_label(moduleName, JANIedge["action"].asString());
		if (label.is_output())
			return make_shared<OutputTransition>(label.str, pre, pos, edgeClock);
		else
			return make_shared<TauTransition>(pre, pos, edgeClock);
	} else {
		// Tau
		return make_shared<TauTransition>(pre, pos, edgeClock);
	}
}


std::pair< std::shared_ptr<Exp>,
		   std::shared_ptr<Location> >
JaniTranslator::build_IOSA_precondition_from_STA(const Json::Value& JANIguard,
												 const shared_vector<Decl>& clocks)
{
	/// @note <b>On the validity of JANI guards</b><br>
	/// We're only accepting guards in the format "g [ && clk ⋈ rv ]",
	/// where "⋈" ∈ {<,>,≤,≥}, "clk" is a clock, "rv" is a real variable,
	/// and "g" is any expression using only integral and boolean variables,
	/// e.g. no clock or real variable must appear in "g". That's because
	/// IOSA transitions can wait on (at most) a single clock.<br>
	/// The "&& clk ⋈ rv" section is optional: a valid guard could also be
	/// composed only of the "g" section. Edges whose guard *has* the clock
	/// comparison are "output" or "tau"; edges *without it* are "input".

	static constexpr char errMsg[]("[ERROR] One or less clock comparisons are "
								   "allowed per edge guard, since a transition "
								   "in IOSA can wait on (at most) one clock");
	static const std::pair<shared_ptr<Exp>, shared_ptr<Location>> errRet;

	if (JANIguard.isObject() && JANIguard.isMember("op") &&
		JANIguard["op"].asString() == JANI_operator_string.at(ExpOp::andd)) {
		// May still find clocks, look for them in subexpressions
		std::shared_ptr<Exp> leftExp, rightExp;
		std::shared_ptr<Location> leftClk, rightClk;
		std::tie(leftExp, leftClk) = build_IOSA_precondition_from_STA(
										 JANIguard["left"], clocks);
		std::tie(rightExp, rightClk) = build_IOSA_precondition_from_STA(
										 JANIguard["right"], clocks);
		if (nullptr == leftExp && nullptr == rightExp) {
			return errRet;  // failed translating subexpression
		} else if (nullptr != leftClk && nullptr != rightClk) {
			// Two clocks found! Error
			figTechLog << errMsg << " (clocks \"" << leftClk->get_identifier()
					   << "\" and \"" << rightClk->get_identifier() << "\" "
					   << "were found in the same guard)\n";
			return errRet;
		}
		// Alles in Ordnung
		std::shared_ptr<Exp> exp;
		if (nullptr == leftExp)
			exp = rightExp;
		else if (nullptr == rightExp)
			exp = leftExp;
		else
			exp = std::make_shared<BinOpExp>(ExpOp::andd, leftExp, rightExp);
		if (nullptr != leftClk)
			return std::make_pair(exp, leftClk);
		else
			return std::make_pair(exp, rightClk);  // rightClk may be null

	} else if (JANIguard.isObject() && JANIguard.isMember("op") && (
		JANIguard["op"].asString() == JANI_operator_string.at(ExpOp::lt) ||
		JANIguard["op"].asString() == JANI_operator_string.at(ExpOp::gt) ||
		JANIguard["op"].asString() == JANI_operator_string.at(ExpOp::le) ||
		JANIguard["op"].asString() == JANI_operator_string.at(ExpOp::ge))) {
		// This may be a clock comparison
		const ExpOp op(IOSA_operator.at(JANIguard["op"].asString()));
		auto left = build_IOSA_expression(JANIguard["left"]);
		auto right = build_IOSA_expression(JANIguard["right"]);
		auto clockIDL = first_ID_in_expr(left, clocks);
		auto clockIDR = first_ID_in_expr(right, clocks);
		if (clockIDL.empty() && clockIDR.empty()) {
			// No clocks found => this ain't special
			auto exp = std::make_shared<BinOpExp>(op, left, right);
			return std::make_pair(exp, std::shared_ptr<Location>());
		} else if (!clockIDL.empty() && !clockIDR.empty()) {
			// Two clocks found => error
			figTechLog << errMsg << " (clocks \"" << clockIDL << "\" and \""
					   << clockIDR << "\" were found in the same guard)\n";
			return errRet;
		}
		auto clk = clockIDL+clockIDR;  // one isn't empty
		auto realVar = clk2rv_[clk];
		assert(!realVar.empty());
		shared_ptr<LocExp> loc;
		if (!clockIDL.empty())  // clock to the left => real var to the right
			loc = std::dynamic_pointer_cast<LocExp>(right);
		else                    // clock to the right => real var to the left
			loc = std::dynamic_pointer_cast<LocExp>(left);
		if (nullptr == loc) {
			figTechLog << errMsg << " (clocks can only be compared to a plain "
					   << "real variable, e.g. clock \"" << clk << "\")\n";
			return errRet;
		} else if (loc->get_exp_location()->get_identifier() != realVar) {
			figTechLog << errMsg << " (clock \"" << clk << "\" is compared to "
					   << "real var \"" << loc->get_exp_location()->get_identifier()
					   << "\", but was mapped to the real var \"" << realVar << "\")\n";
			return errRet;
		}
		// As result return "true", since this was the operand of an 'and'
		return std::make_pair(nullptr,//std::make_shared<BConst>(true),
							  std::make_shared<Location>(clk));
	} else {
		// No more clocks allowed from here downwards
		auto exp = build_IOSA_expression(JANIguard);
		auto residualClock = first_ID_in_expr(exp, clocks);
		if (!residualClock.empty()) {
			// Unreachable clocks found deep within the guard
			figTechLog << errMsg << " (clock \"" << residualClock << "\" "
					   << "found too deep in the guard expression -- it "
					   << "should be an operand of an uppermost 'and')\n";
			return errRet;
		}
		return std::make_pair(exp, std::shared_ptr<Location>());
	}
}


shared_vector<Effect>
JaniTranslator::build_IOSA_postcondition(const Json::Value& JANIdest,
										 const shared_vector<Decl>& moduleVars,
										 const shared_vector<Decl>& moduleClocks)
{
	assert(JANIdest.isArray());
	assert(!JANIdest.empty());
	assert(JANIdest[0].isObject());

	if (JANIdest.size() > 1ul) {
		if (!JANIdest[0].isMember("probability"))
			figTechLog << "[ERROR] IOSA is incompatible with internal non-determinism.\n";
		else
			figTechLog << "[ERROR] FIG doesn't handle probabilities in edges yet.\n";
		return shared_vector<Effect>();
	}
	auto is_valid_var = [&moduleVars] (const std::string& var) {
		return std::find_if(begin(moduleVars), end(moduleVars),
							[&var](const shared_ptr<Decl> d){return d->get_id()==var;})
				!= end(moduleVars);
	};
	auto is_valid_clk = [&moduleClocks] (const std::string& clk) {
		return std::find_if(begin(moduleClocks), end(moduleClocks),
							[&clk](const shared_ptr<Decl> d){return d->get_id()==clk;})
				!= end(moduleClocks);
	};

	// Build assignments and clock resets of this postcondition
	shared_vector<Effect> pos;
	pos.reserve(JANIdest[0]["assignments"].size());
	std::stack<std::string> postponedClockResets;
	for (const auto& ass: JANIdest[0]["assignments"]) {
		auto lvalue = ass["ref"].asString();
		if (is_valid_var(lvalue)) {
			// Variable assignment
			auto loc = make_shared<Location>(lvalue);
			auto rvalue = build_IOSA_expression(ass["value"]);
			pos.push_back(make_shared<Assignment>(loc, rvalue));
		} else if (is_valid_clk(lvalue)) {
			// Clock reset
			auto reset = rv2dist_.find(clk2rv_[lvalue]);
			assert(end(rv2dist_) != reset);
			if (nullptr == reset->second)
				postponedClockResets.emplace(lvalue);
			else
				pos.push_back(reset->second);
		} else if (rv2dist_.find(lvalue) != end(rv2dist_)) {
			// Distribution sampling of real var (map for later)
			if (!map_rv_to_dist(lvalue, ass["value"])) {
				figTechLog << "[ERROR] Problems with distribution sampling "
						   << "for real variable \"" << lvalue << "\"\n";
				return shared_vector<Effect>();
			}
		} else {
			throw_FigException("invalid assignment for lvalue \""+lvalue+"\"");
		}
	}

	// Review postponed clocks
	while (!postponedClockResets.empty()) {
		auto clk = postponedClockResets.top();
		auto reset = rv2dist_.find(clk2rv_[clk]);
		assert(end(rv2dist_) != reset);
		if (nullptr != reset->second)
			pos.push_back(reset->second);
		else {
			figTechLog << "[ERROR] Failed to sample clock \"" << clk << "\" "
					   << "(mapped to real var \"" << clk2rv_[clk] << "\")\n";
			return shared_vector<Effect>();
		}
		postponedClockResets.pop();
	}

	return pos;
}


std::shared_ptr<Prop>
JaniTranslator::build_IOSA_property(const Json::Value& JANIprop)
{
	// Check validity of JANI Property format
	if ( ! JANIprop.isObject() ||
		 ! (JANIprop.isMember("name") && JANIprop.isMember("expression")))
		throw_FigException("invalid property in JANI file");
	const Json::Value& JANIfilter = JANIprop["expression"];
	if ( ! JANIfilter.isObject() ||
		 ! (JANIfilter.isMember("op") && JANIfilter.isMember("fun") &&
			JANIfilter.isMember("values") && JANIfilter.isMember("states")) ||
		 ! (JANIfilter["op"].isString() && JANIfilter["op"].asString() == "filter"))
		throw_FigException("all JANI properties must be wrapped in a filter");
	else if ( ! (JANIfilter["fun"].isString()    &&
				 JANIfilter["values"].isObject() &&
				 JANIfilter["states"].isObject()) ||
			  ! (JANIfilter["values"].isMember("op") &&
				 JANIfilter["values"]["op"].isString()) ||
			  ! (JANIfilter["states"].isMember("op") &&
				 JANIfilter["states"]["op"].isString()))
		throw_FigException("invalid JANI filter");

	// Check IOSA compatibility
	if (JANIfilter["fun"].asString() != "min" &&
		JANIfilter["fun"].asString() != "max" &&
		JANIfilter["fun"].asString() != "avg") {
		figTechLog << "[ERROR] Filter function \"" << JANIfilter["fun"].asString()
				   << "\" not supported; IOSA properties are only compatible "
				   << "with the \"min\", \"max\" and \"avg\" JANI filters.\n";
		return nullptr;
	} else if (JANIfilter["states"]["op"].asString() != "initial") {
		figTechLog << "[ERROR] Filter states \"" << JANIfilter["states"].asString()
				   << "\" not supported; IOSA properties are only compatible "
				   << "with the \"initial\" JANI state predicate.\n\"";
		return nullptr;
	}

	// Return specialized property, or nullptr if unsupported
	try {
		const auto propExprOp(JANIfilter["values"]["op"].asString());
		if ("Pmin" == propExprOp || "Pmax" == propExprOp) {
			// Transient
			if (!JANIfilter["values"].isMember("exp"))
				throw_FigException("invalid JANI property \"" + propExprOp + "\"");
			return build_IOSA_transient_property(JANIfilter["values"]["exp"]);
		} else if ("Smin" == propExprOp || "Smax" == propExprOp) {
			// Rate
			if (!JANIfilter["values"].isMember("exp"))
				throw_FigException("invalid JANI property \"" + propExprOp + "\"");
			auto states = build_IOSA_expression(JANIfilter["values"]["exp"]);
			if (nullptr == states) {
				figTechLog << "[ERROR] Failed translating the content of "
						   << "PropertyExpression \"" << propExprOp << "\"\n.";
				return nullptr;
			}
			return make_shared<RateProp>(states);
		} else {
			// Incompatible
			figTechLog << "[ERROR] PropertyExpression \"" << propExprOp << "\" "
					   << "not supported; IOSA properties are only compatible "
					   << "with \"Pmin\", \"Pmax\", \"Smin\" and \"Smax\" JANI "
					   << "property expressions.\n";
			return nullptr;
		}
	} catch (std::out_of_range&) {
		// this may happen when translating the expression, if it turns out
		// to be another PropertyExpression rather than a plain Expression
		figTechLog << "[ERROR] IOSA has no support for nested properties "
				   << "(außer 'U' in 'Pmin'/'Pmax')\n";
		return nullptr;
	}
}


std::shared_ptr<TransientProp>
JaniTranslator::build_IOSA_transient_property(const Json::Value& JANIpexp)
{
	if ( ! JANIpexp.isObject() ||
		 ! (JANIpexp.isMember("op") && JANIpexp["op"].isString()))
		throw_FigException("invalid Pmin/Pmax PropertyExpression");
	else if (JANIpexp["op"] != "U") {
		figTechLog << "[ERROR] Unsupported PropertyExpression; IOSA properties "
				   << "are only compatible with Pmin/Pmax JANI properties when "
				   << "these have a nested 'U'.\n";
		return nullptr;
	}
	if ( ! (JANIpexp.isMember("left")  && JANIpexp["left"].isObject()) ||
		 ! (JANIpexp.isMember("right") && JANIpexp["right"].isObject()) )
		throw_FigException("invalid 'U' PropertyExpression");
	if (JANIpexp.isMember("step-bounds"))
		figTechLog << "[WARNING] Ignoring \"step-bounds\" in 'U' subproperty.\n";
	if (JANIpexp.isMember("time-bounds"))
		figTechLog << "[WARNING] Ignoring \"time-bounds\" in 'U' subproperty.\n";
	if (JANIpexp.isMember("reward-bounds"))
		figTechLog << "[WARNING] Ignoring \"reward-bounds\" in 'U' subproperty.\n";
	// out_of_range exceptions due to nested props should be caught above us:
	auto left = build_IOSA_expression(JANIpexp["left"]);
	auto right = build_IOSA_expression(JANIpexp["right"]);
	if (nullptr == left || nullptr == right) {
		figTechLog << "[ERROR] Failed translating the contents of 'U' inside a "
				   << "Pmin/Pmax PropertyExpression.\n";
		return nullptr;
	}
	return std::make_shared<TransientProp>(left, right);
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
