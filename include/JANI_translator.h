//==============================================================================
//
//  JANI_translator.h
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

#ifndef JANI_TRANSLATOR_H
#define JANI_TRANSLATOR_H

// C++
#include <set>
#include <map>
#include <vector>
#include <memory>
#include <string>
// External code
#include <json-forwards.h>
// FIG
#include <ModelAST.h>
#include <core_typedefs.h>


class ModuleScope;


namespace fig
{

/// Transform Stochastic Automata model specification files between
/// <a href="http://jani-spec.org/">JANI format</a> and IOSA syntax.<br>
/// From JANI's point of view the files correspond to
/// <a href="http://goo.gl/vros8C">Stochatic Timed Automata</a> (STA),
/// of which only a deterministic subset (viz. without non-determinism)
/// is valid for translation.<br>
/// From FIG's point of view the files correspond to
/// <a href="http://dsg.famaf.unc.edu.ar/node/643">Input/Output Stochastic
/// Automata</a> (IOSA).
class JaniTranslator : public Visitor
{
public:  // Ctor/Dtor

	/// Empty ctor
	JaniTranslator();

	/// Default dtor
	~JaniTranslator();

public:  // Translation facilities

	/**
	 * Translate IOSA model file to <a href="http://jani-spec.org/">
	 * JANI specification format</a>. If specified, include also all properties
	 * defined in the properties file.
	 *
	 * @param iosaModelFile Path to (or name of) file with the IOSA model
	 * @param iosaPropsFile Path to (or name of) file with properties to check
	 * @param janiFilename  Desired name of the translated JANI file to create
	 * @param validityCheck Whether to validate the IOSA syntax of the model file
	 *
	 * @return Name of file with the model translated to an STA written in
	 *         the JANI specification format (see notes)
	 *
	 * @throw FigException if we couldn't generate a valid JANI translation
	 *
	 * @note IOSA model files can also have properties defined in them,
	 *       inside a "properties...endproperties" section.
	 * @note If 'janiFilename' was passed then the JANI file generated will
	 *       have that name. Otherwise a name related to 'iosaModelFile' is
	 *       automatically generated.
	 */
	std::string
	IOSA_2_JANI(const std::string& iosaModelFile,
				const std::string& iosaPropsFile = "",
				const std::string& janiFilename = "",
				bool validityCheck = true);

	/**
	 * Translate model file specified in <a href="http://jani-spec.org/">
	 * JANI format</a> to IOSA syntax. Properties, if present, will be included
	 * inside a "properties...endproperties" section in the IOSA file.
	 *
	 * @param janiModelFile Path to (or name of) file with STA model
	 *                      written in valid JANI-spec format
	 * @param iosaFilename  Desired name of the translated IOSA file to create
	 * @param skipFileDump  Don't write result to file; just keep the model
	 *                      compiled in memory (in the ModelSuite singleton)
	 *
	 * @return Name of file with the model translated to IOSA syntax (see notes)
	 *
	 * @throw FigException if we couldn't generate a valid IOSA translation,
	 *                     e.g. when the JANI model wasn't a deterministic STA
	 *
	 * @note If 'skiṕFileDump' then an empty string is returned.
	 *       Otherwise if 'iosaFilename' was specified the IOSA file will
	 *       have that name. Otherwise a name related to 'janiModelFile' is
	 *       automatically generated.
	 */
	std::string
	JANI_2_IOSA(const std::string& janiModelFile,
				const std::string& iosaFilename = "",
				bool skipFileDump = false);

private:  // Class attributes

	/// Prefix used to generate a real variable from a clock
	/// @note Used for IOSA -> STA translation
	static constexpr char REAL_VAR_FROM_CLOCK_PREFIX[] = "x_";

	/// An empty Json::Value of "object" type
	static const Json::Value EMPTY_JSON_OBJ;

	/// An empty Json::Value of "array" type
	static const Json::Value EMPTY_JSON_ARR;

	/// ModelAST of the last parsed model
	shared_ptr< Model > IOSAroot_;

	/// JsonCPP of the last parsed model, in JANI Specifiaction format
	shared_ptr< Json::Value > JANIroot_;

	/// Current JSON field to fill in with info from last parsed IOSA model
	shared_ptr< Json::Value > JANIfield_;

	/// Name of the module currently translated (visited)
	/// @note Used for IOSA -> STA translation
	std::string currentModule_;

	/// Scope of the module currently translated (visited)
	/// @note Used for IOSA -> STA translation
	shared_ptr<ModuleScope> currentScope_;

	/// Name of the clock currently translated (visited)
	/// @note Used for JANI -> IOSA translation
	std::string currentClock_;

	/// Input/Output label sets of a module
	typedef std::pair< std::set< std::string >,   // inputs
					   std::set< std::string > >  // outputs/tau
		LabelSets;

	/// Labels of each module split in I/O
	std::map< std::string, LabelSets > modulesLabels_;

	/// All model labels grouped together without discrimination
	std::set< std::string > modelLabels_;

	/// Invariant needed by STA to make time progress
	/// @note Updated by build_JANI_guard()
	/// @note Reset by visit(shared_ptr<ModuleAST>)
	/// @note Used for IOSA -> STA translation
	shared_ptr< Json::Value> timeProgressInvariant_;

	/// Real variables defined in a JANI Specification file
	/// which should later be mapped one-to-one to clock variables
	/// @note Used for STA -> IOSA translation
	/// @see clock2real_
	std::set< std::string > realVars_;

	/// Maping of clock variable names to their real variable counterparts,
	/// used in JANI Specification files to model time progress through
	/// location invariants.
	/// @note Used for STA -> IOSA translation
	std::map< std::string, Reference<std::string> > clock2real_;

	/// Renaming from automata and label to fresh sync label name
	/// @note Used for JANI -> IOSA translation
	/// @note Populated when interpreting flat labels as I/O for synchronization
	/// @see sync_label()
	/// @see test_and_build_IOSA_synchronization()
	std::map< std::pair< std::string, std::string >,
			  std::string > syncLabel_;

private:  // Class utils: general

	/// Generate a fresh label name for use in synchronization
	/// @param hint Suggested name for the label in Json string format, if any
	/// @return Content of 'hint' if given, fresh label name otherwise
	static std::string fresh_label(const Json::Value& hint = 0);

	/// Get the name of the real variable corresponding to this clock name
	/// @return Name of the real variable linked to the clock;
	///         if none found and '!force' then empty string;
	///         if none found and 'force' then build one from clockName.
	/// @see REAL_VAR_FROM_CLOCK_PREFIX
	/// @see clock2real_
	std::string rv_from(const std::string& clockName, bool force = true);

	/// Get the name of the sync label assigned to this module-label pair,
	/// decided after parsing the JANI synchronization vectors
	/// @param module Module name
	/// @param label  Label name in the module
	/// @return Label chosen for synchronization for this module and label,
	///         or empty string if none assigned
	/// @note Used for JANI -> IOSA translation
	/// @see test_and_build_IOSA_synchronization()
	std::string sync_label(const std::string& module, const std::string& label);

	/// Try to evaluate an expression to an integral value;
	/// put an error in our ErrorMessage if unsuccessfull
	int get_int_or_error(shared_ptr<Exp> exp, const std::string& msg);

	/// Try to evaluate an expression to a boolean value;
	/// put an error in our ErrorMessage if unsuccessfull
	bool get_bool_or_error(shared_ptr<Exp> exp, const std::string& msg);

	/// Try to evaluate an expression to a floating point value;
	/// put an error in our ErrorMessage if unsuccessfull
	float get_float_or_error(shared_ptr<Exp> exp, const std::string& msg);

private:  // Class utils: IOSA -> JANI

	/// Parse the given files and populate IOSAroot_
	/// @warning Any previous content in IOSAroot_ will be lost
	void parse_IOSA_model(const string& iosaModelFile,
						  const string& iosaPropsFile = "",
						  bool validityCheck = true);

	/// Interpret 'decl', which should be a boolean/integer/floating point
	/// constant, and add the corresponding "JANI constant fields" in JANIobj
	void build_JANI_constant(shared_ptr<InitializedDecl> decl,
							 Json::Value& JANIobj);

	/// Add to JANIobj the "JANI guard fields" translated from
	/// the corresponding data inside the IOSA transition 'trans'
	/// @note If the transition has an output and hence a triggering clock,
	///       "&& (clock >= real_var)" is added to the guard condition
	/// @note If the transition has an output and hence a triggering clock,
	///       "&& (guard implies clock <= real_var)" is added to
	///       timeProgressInvariant_, where 'guard' is this precondition
	/// @warning JANIfield_ is used and invalidated
	void build_JANI_guard(shared_ptr<TransitionAST> trans,
						  Json::Value& JANIobj);

	/// Build the comparison "(clock op real_var)" and paste it into JANIobj,
	/// where 'op' must be either "greater or equal" or "less or equal".
	/// This is needed for IOSA -> STA translation.
	/// @warning Any previous value of JANIobj is lost
	void build_JANI_clock_comp(const std::string& clockName,
							   ExpOp op,
							   Json::Value& JANIobj);

	/// Add to JANIobj the "JANI destination fields" translated from
	/// the corresponding data inside the IOSA transition 'trans'
	/// @warning JANIfield_ is used and invalidated
	void build_JANI_destinations(shared_ptr<TransitionAST> trans,
								 Json::Value& JANIobj);

	/// Add to JANIobj the "JANI STA distribution fields" translated from
	/// the IOSA distribution 'clockDist'
	/// @warning JANIfield_ is used and invalidated
	void build_JANI_distribution(shared_ptr<Dist> clockDist,
								 Json::Value& JANIobj);

	/// Add to JANIobj the "JANI automata composition fields" translated
	/// from the info gathered into modulesLabels_ from the current model
	void build_JANI_synchronization(Json::Value& JANIobj);

	/// Append to JANIarr the "JANI synchronization vector" corresponding to
	/// this output label, using the info gathered into modulesLabels_
	/// from the current model under translation
	void build_JANI_sync_vector(const std::string& oLabel,
								Json::Value& JANIarr);

private:  // Visitor overrides for parsing

	/// Populate JANIroot with all data we can extract from given Model
	/// @warning If there's some previously parsed model information
	///	         then JANIroot will be cleared from all data
	void visit(shared_ptr<Model> node) override;

	/// Append/assign to JANIfield the JANI translation of this IOSA constant
	/// (or boolean variable)
	void visit(shared_ptr<InitializedDecl> node) override;

	/// Append/assign to JANIfield the JANI translation of this IOSA variable
	void visit(shared_ptr<RangedDecl> node) override;

	/// Append/assign to JANIfield the JANI translation of this IOSA clock
	/// @note A real variable named "x_name" is also created, where "name" is
	///       the clock's id, since JANI actually supports STA rather than SA
	void visit(shared_ptr<ClockDecl> node) override;

	/// Append/assign to JANIfield the JANI translation of this IOSA array
	void visit(shared_ptr<ArrayDecl> node) override;

	/// Append/assign to JANIfield the reduction of this IOSA boolean constant.
	void visit(shared_ptr<BConst> node) override;

	/// Append/assign to JANIfield the reduction of this IOSA totegral constant.
	void visit(shared_ptr<IConst> node) override;

	/// Append/assign to JANIfield the reduction of this IOSA floattog potot constant
	/// (i.e. JANI's "real")
	void visit(shared_ptr<FConst> node) override;

	/// Append/assign to JANIfield the identifier of this IOSA location
	/// (aka variable: boolean, integral, clock)
	/// @note Arrays are not yet supported
	void visit(shared_ptr<LocExp> node)  override;

	/// Append/assign to JANIfield the JANI translation of this IOSA
	/// unary operator.
	void visit(shared_ptr<UnOpExp> node) override;

	/// Append/assign to JANIfield the JANI translation of this IOSA
	/// binary operator.
	void visit(shared_ptr<BinOpExp> node) override;

	/// Append/assign to JANIfield the JANI translation of this IOSA module
	void visit(shared_ptr<ModuleAST> node) override;

	/// Append/assign to JANIfield the JANI translation of this IOSA transition
	/// @note Specialization'd be pointless, right?
	void visit(shared_ptr<TransitionAST> node) override;

	/// Append/assign to JANIfield the JANI translation of this IOSA
	/// postcondition variable assignment
	void visit(shared_ptr<Assignment> node) override;

private:  // Class utils: JANI -> IOSA

	/// Parse the given file and populate JANIroot_
	/// @warning Any previous content in JANIroot_ will be lost
	void parse_JANI_model(const string& janiModelFile);

	/// Translate the current JANI specification in JANIroot_, if possible
	/// @return Whether a valid IOSA model could be built
	/// @throw FigException if JANI specification is badly formated
	bool build_IOSA_from_JANI();

//	/// Interpret the JANI model in JANIroot_ as a CTMC, translate it to IOSA
//	/// populating IOSAroot_, and build the model in ModelSuite.
//	/// @param janiModel JANI specification of the CTMC model to translate
//	/// @param iosaModel IOSA AST to fill in with translation
//	/// @return true
//	/// @throw FigException if JANI file is badly formated
//	bool build_IOSA_from_JANI_CTMC(const Json::Value& janiModel, Model& iosaModel);
//
//	/// Interpret the JANI model in JANIroot_ as a STA, (if possible) translate
//	/// it to IOSA populating IOSAroot_, and build the model in ModelSuite.
//	/// @param janiModel JANI specification of the STA model to translate
//	/// @param iosaModel IOSA AST to fill in with translation
//	/// @return Whether a valid IOSA model could be built
//	/// @throw FigException if JANI file is badly formated
//	bool build_IOSA_from_JANI_STA(const Json::Value& janiModel, Model& iosaModel);

	/// Get IOSA translation of this JANI expression
	/// @param JANIconst Json object with the Expression to translate
	/// @return IOSA Exp or nullptr if translation failed
	/// @throw FigException if JANI Expression is badly formated
	shared_ptr<Exp> build_IOSA_expression(const Json::Value& JANIexpr);

	/// Get IOSA translation of this JANI constant
	/// @param JANIconst Json object with the ConstantDeclaration to translate
	/// @return IOSA constant declaration or nullptr if translation failed
	/// @throw FigException if JANI ConstantDeclaration is badly formated
	shared_ptr<InitializedDecl> build_IOSA_constant(const Json::Value& JANIconst);

	/// Get IOSA translation of this JANI variable
	/// @param JANIvar Json object with the VariableDeclaration to translate
	/// @return IOSA variable declaration or nullptr if translation failed
	/// @throw FigException if JANI VariableDeclaration is badly formated
	shared_ptr<Decl> build_IOSA_variable(const Json::Value& JANIvar);

	/// Build a RangedDecl from the given data
	/// @param varName Name of the variable from the JANI spec
	/// @param varType JANI BoundedType specification
	/// @param varInit Variable initialization or nullObject if absent
	/// @return IOSA RangedDecl or nullptr if translation failed
	/// @throw FigException if JANI data is badly formated
	shared_ptr<Decl> build_IOSA_ranged_variable(const std::string& varName,
												const Json::Value& varType,
												const Json::Value& varInit);

	/// Build an InitializedDecl for a boolean variable from the given data
	/// @param varName Name of the variable from the JANI spec
	/// @param varInit Variable initialization or nullObject if absent
	/// @return IOSA InitializedDecl or nullptr if translation failed
	/// @throw FigException if JANI data is badly formated
	shared_ptr<Decl> build_IOSA_boolean_variable(const std::string& varName,
												 const Json::Value& varInit);

	/// Verify synchronization is compatible with IOSA broadacst;
	/// if so then interpret and build IOSA I/O synchronization
	/// @param janiComposition JANI specification  'system'  field
	/// @param janiAutomata    JANI specifiaction 'automata' field
	/// @return Whether the JANI synchronization specified is IOSA-compatible
	/// @note Populates syncLabel_
	bool test_and_build_IOSA_synchronization(const Json::Value& janiComposition,
											 const Json::Value& janiAutomata);

//	/// Get IOSA module translated from this JANI automaton
//	/// @param JANIautomaton Json object with the Automaton to translate
//	/// @return IOSA module AST or nullptr if translation failed
//	/// @throw FigException if JANI Automaton is badly formated
//	shared_ptr<ModuleAST> build_IOSA_module(const Json::Value& JANIautomaton);

	/// Interpret this JANI automaton as a CTMC and translate it
	/// to a IOSA module if possible
	/// @param janiAutomaton JANI specification of the CTMC module to translate
	/// @return IOSA module AST or nullptr if translation failed
	/// @throw FigException if JANI file is badly formated
	shared_ptr<ModuleAST> build_IOSA_module_from_CTMC(const Json::Value& janiAutomaton);

	/// Interpret this JANI automaton as a STA and translate it
	/// to a IOSA module if possible
	/// @param janiAutomaton JANI specification of the STA module to translate
	/// @return IOSA module AST or nullptr if translation failed
	/// @throw FigException if JANI file is badly formated
	shared_ptr<ModuleAST> build_IOSA_module_from_STA(const Json::Value& janiAutomaton);

	/// Get IOSA transition translated from this JANI edge,
	/// interpreting the JANI automaton as a CTMC
	/// @param JANIedge     Json object with JANI edge to translate (CTMC-like)
	/// @param JANIlocation Json object with the current automaton locations
	/// @param IOSAvars     Variables of the current IOSA module
	/// @return IOSA transition or nullptr if translation failed
	/// @throw FigException if JANI edge specifiaction is badly formated
	shared_ptr<TransitionAST> build_IOSA_transition_from_CTMC(
			const Json::Value& JANIedge,
			const Json::Value& JANIlocations,
			const shared_vector<Decl>& IOSAvars);

	/// Get IOSA transition translated from this JANI edge,
	/// interpreting the JANI automaton as an STA
	/// @param JANIedge     Json object with JANI edge to translate (STA-like)
	/// @param JANIlocation Json object with the current automaton locations
	/// @param IOSAvars     Variables of the current IOSA module
	/// @return IOSA transition or nullptr if translation failed
	/// @throw FigException if JANI edge specifiaction is badly formated
	shared_ptr<TransitionAST> build_IOSA_transition_from_STA(
			const Json::Value& JANIedge,
			const Json::Value& JANIlocations,
			const shared_vector<Decl>& IOSAvars);
};

} // namespace fig

#endif // JANI_TRANSLATOR_H
