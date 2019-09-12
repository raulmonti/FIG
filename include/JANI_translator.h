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

class Label;



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
	virtual ~JaniTranslator();

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

	/// Label used to synchronise modules when moving from their
	/// initial-location to their "real location"
	/// @note For compatibility with the Modest Toolset
	static constexpr char INIT_CLOCKS[] = "init_clocks";

	/// If a label in a module synchronises es with no other module,
	/// still create a sync-array for it where the module talks to itself
	/// @note For compatibility with the Modest Toolset
	static constexpr bool ALLOW_SELF_SYNCING = true;

	/// An empty Json::Value of "object" type
	static const Json::Value EMPTY_JSON_OBJ;

	/// An empty Json::Value of "array" type
	static const Json::Value EMPTY_JSON_ARR;

	/// ModelAST of the last parsed model
	shared_ptr< Model > IOSAroot_;

	/// JsonCPP of the last parsed model, in JANI Specifiaction format
	shared_ptr< Json::Value > JANIroot_;

	/// Current JSON field to fill in with info from just-parsed IOSA model
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

	/// Labels from all modules
	std::set< std::string > modelLabels_;

	/// Variables that appear in the properties
	/// @note For compatibility with the Modest Toolset,
	///       these will be stored as *** global variables ***
	/// @note The resulting JANI file is no longer an IOSA !!!
	std::set< Json::Value > variablesInProperties_;

	/// Invariant needed by STA to make time progress
	/// @note Updated by build_JANI_guard()
	/// @note Reset by visit(shared_ptr<ModuleAST>)
	/// @note Used for IOSA -> STA translation
	std::map< std::string, Json::Value > timeProgressInvariant_;

	/// Real variables defined in a JANI Specification file,
	/// used to sample the distributions that IOSA maps to clocks
	/// @note Used for STA -> IOSA translation
	/// @see clk2rv_
	std::map< std::string, std::shared_ptr<ClockReset> > rv2dist_;

	/// Maping of clock variable names to their real variable counterparts,
	/// used in JANI Specification files to model time progress through
	/// location invariants.
	/// @note Used for STA -> IOSA translation
	std::map< std::string, std::string > clk2rv_;

	/// Renaming from automata and label to fresh sync label
	/// @note Used for JANI -> IOSA translation
	/// @note Populated when interpreting flat labels as I/O for synchronization
	/// @see sync_label()
	/// @see test_and_build_IOSA_synchronization()
	std::map< std::pair< std::string, std::string >,
			  Label > syncLabel_;

	/// Rates of the edges whose action labels where classified as inputs;
	/// for CTMC parallel synchronization these should reduce to 1 or 0.
	/// @note Used for CTMC -> IOSA translation
	/// @note Populated by build_exponential_clock()
	std::vector< std::tuple< std::string, std::string, std::shared_ptr<Exp> > >
			inputRatesCTMC_;

private:  // Class utils: general

	/// Generate a fresh label name for use in synchronization
	/// @param hint Suggested name for the label in Json string format, if any
	/// @return Content of 'hint' if not null nor empty,
	///         fresh label name otherwise
	/// @post Not empty string returned
	static std::string fresh_label(const Json::Value& hint);

	/// Get the name of the real variable corresponding to this clock name
	/// @return Name of the real variable linked to the clock;
	///         if none found and '!force' then empty string;
	///         if none found and 'force' then build one from clockName.
	/// @see REAL_VAR_FROM_CLOCK_PREFIX
	/// @see clk2rv_
	std::string rv_from(const std::string& clockName, bool force = true);

	/// Get the sync label assigned to this module-label pair,
	/// decided after parsing the JANI synchronization vectors
	/// @param module Module name
	/// @param label  Label name in the module
	/// @return Synchronization label chosen for this module and label
	///         (TAU if none assigned)
	/// @note Used for JANI -> IOSA translation
	/// @see test_and_build_IOSA_synchronization()
	const Label& sync_label(const std::string& module, const std::string& label);

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

	/// Process the time-progress conditions extracted during guards parsing,
	/// and build a "condensed" time-progress invariant that groups
	/// preconditions by clocks
	Json::Value build_JANI_time_progress();

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
	/// from the info gathered into modulesLabels_
	void build_JANI_synchronization(Json::Value& JANIobj);

	/// Append to JANIarr the "JANI synchronization vector" corresponding to
	/// this output label, using the info gathered into modulesLabels_
	void build_JANI_sync_vector(const std::string& oLabel,
								Json::Value& JANIarr);

private:  // Visitor overrides for parsing

	/// Populate JANIroot with all data we can extract from given Model
	/// @warning If there's some previously parsed model information
	///	         then JANIroot will be cleared from all data
	void visit(shared_ptr<Model> node) override;

	/// Append/assign to JANIfield the JANI translation of this IOSA module
	void visit(shared_ptr<ModuleAST> node) override;

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

	/// Append/assign to JANIfield the reduction of this IOSA integral constant.
	void visit(shared_ptr<IConst> node) override;

	/// Append/assign to JANIfield the reduction of this IOSA floating point
	/// constant (i.e. JANI's "real")
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

	/// Append/assign to JANIfield the JANI translation of this IOSA transition
	/// @note Specialization'd be pointless, right?
	void visit(shared_ptr<TransitionAST> node) override;

	/// Append/assign to JANIfield the JANI translation of this IOSA
	/// postcondition variable assignment
	void visit(shared_ptr<Assignment> node) override;

	/// Append/assign to JANIfield the JANI translation of this
	/// transient property expression
	void visit(shared_ptr<TransientProp> node) override;

	/// Append/assign to JANIfield the JANI translation of this
	/// rate property expression
	void visit(shared_ptr<RateProp> node) override;

private:  // Class utils: JANI -> IOSA

	/// Parse the given file and populate JANIroot_
	/// @warning Any previous content in JANIroot_ will be lost
	void parse_JANI_model(const string& janiModelFile);

	/// Translate the current JANI specification in JANIroot_, if possible
	/// @return Whether a valid IOSA model could be built
	/// @throw FigException if JANI specification is badly formated
	bool build_IOSA_from_JANI();

	/// Get IOSA translation of this JANI expression
	/// @param JANIexpr Json object with the Expression to translate
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

	/// Build a RangedDecl from given data
	/// @param varName Name of the variable from the JANI spec
	/// @param varType JANI BoundedType specification
	/// @param varInit Variable initialization or nullObject if absent
	/// @return IOSA RangedDecl or nullptr if translation failed
	/// @throw FigException if JANI data is badly formated
	shared_ptr<Decl> build_IOSA_ranged_variable(const std::string& varName,
												const Json::Value& varType,
												const Json::Value& varInit);

	/// Build an InitializedDecl for a boolean variable from given data
	/// @param varName Name of the variable from the JANI spec
	/// @param varInit Variable initialization or nullObject if absent
	/// @return IOSA InitializedDecl or nullptr if translation failed
	/// @throw FigException if JANI data is badly formated
	shared_ptr<Decl> build_IOSA_boolean_variable(const std::string& varName,
												 const Json::Value& varInit);

	/// Build a ClockReset for a clock from given data
	/// @param clockName Name of the clock to reset
	/// @param JANIdsamp Json object with the DistributionSampling to translate
	/// @return IOSA ClockReset or nullptr if translation failed
	/// @throw FigException if JANI DistributionSampling is badly formated
	shared_ptr<ClockReset> build_IOSA_clock_reset(const std::string& clockName,
												  const Json::Value& JANIdsamp);

	/// In the CTMC -> IOSA translation there's one clock per non-input edge;
	/// this routine checks the edge for "CTMC consistency" and builds the clock
	/// @param JANIedge    Json object with JANI edge to translate (CTMC-like)
	/// @param moduleName  Name of the module to which this edge belongs
	/// @return For output and tau edges, clock with exponential distribution.
	///         For input edges, nullptr.
	/// @note The edge is classified as 'input', 'output' or 'tau'
	///       according to the value of sync_label(moduleName,JANIedge["action"])
	/// @throw FigException if JANI edge is badly formated (e.g. not CTMC-like)
	/// @throw FigException if something goes terribly wrong
	shared_ptr<ClockReset> build_exponential_clock(const Json::Value& JANIedge,
												   const std::string& moduleName);

	/// In the STA -> IOSA translation there should be a bijection between
	/// clocks and real variables; this routine interprets the time-progress
	/// invariant of the locations and generates such map
	/// @param moduleLocations Json array with all locations from an automaton
	/// @return Whether the required one-to-one map could be generated
	/// @note Needs clk2rv_ and rv2dist_ with their keys already generated,
	///       viz. the clocks and real vars names should already be known
	/// @note Fills in the value data of clk2rv_
	bool map_clocks_to_rvs(const Json::Value& moduleLocations);

	/// In the STA -> IOSA translation there should be a bijection between
	/// real variables and continuous probabilistic distributions; this
	/// routine tries to map a real var to one such distribution
	/// @param rvName    Real variable to map
	/// @param JANIdsamp Json object with the DistributionSampling to translate
	/// @return Whether the required one-to-one map could be generated,
	///         e.g. false if the real var was already mapped to another
	///         (different) distribution.
	/// @throw FigException if JANI DistributionSampling is badly formated
	/// @note map_clocks_to_rvs() must've been called beforehand
	/// @note Needs rv2dist_ with its keys already generated,
	///       viz. the real vars names should already be known
	/// @note Fills in the value data of rv2dist_
	bool map_rv_to_dist(const std::string& rvName, const Json::Value& JANIdsamp);

	/// Verify synchronization is compatible with IOSA broadacst;
	/// if so then interpret and build IOSA I/O synchronization
	/// @param JANIcomposition JANI specification  'system'  field
	/// @param JANIautomata    JANI specifiaction 'automata' field
	/// @return Whether the JANI synchronization specified is IOSA-compatible
	/// @note Populates syncLabel_
	bool test_and_build_IOSA_synchronization(const Json::Value& JANIcomposition,
											 const Json::Value& JANIautomata);

	/// Interpret this JANI automaton as a CTMC and translate it
	/// to a IOSA module if possible
	/// @param JANIautomaton JANI specification of the CTMC module to translate
	/// @return IOSA module AST or nullptr if translation failed
	/// @throw FigException if JANI file is badly formated
	shared_ptr<ModuleAST> build_IOSA_module_from_CTMC(const Json::Value& JANIautomaton);

	/// Interpret this JANI automaton as a STA and translate it
	/// to a IOSA module if possible
	/// @param JANIautomaton JANI specification of the STA module to translate
	/// @return IOSA module AST or nullptr if translation failed
	/// @throw FigException if JANI file is badly formated
	shared_ptr<ModuleAST> build_IOSA_module_from_STA(const Json::Value& JANIautomaton);

	/// Get IOSA transition translated from this JANI edge,
	/// interpreting the JANI automaton as a CTMC
	/// @param JANIedge    Json object with JANI edge to translate (CTMC-like)
	/// @param moduleName  Name of the automaton to which this edge belongs
	/// @param moduleDecls All variables declarations of the automaton
	/// @param edgeClock   Clock assigned to this edge or nullptr if none
	/// @param allClocks   All the module clocks, to reset in every postcondition
	/// @return IOSA transition or nullptr if translation failed
	/// @throw FigException if JANI edge specifiaction is badly formated
	/// @note A nullptr 'edgeClock' classifies the edge as "input"
	/// @note CTMCs should have only boolean and integral variable declarations
	shared_ptr<TransitionAST> build_IOSA_transition_from_CTMC(
			const Json::Value& JANIedge,
			const std::string& moduleName,
			const shared_vector<Decl>& moduleDecls,
			std::shared_ptr<Location> edgeClock,
			const shared_vector<ClockReset>& allClocks);

	/// Get IOSA transition translated from this JANI edge,
	/// interpreting the JANI automaton as an STA
	/// @param JANIedge    Json object with JANI edge to translate (STA-like)
	/// @param moduleName  Name of the automaton to which this edge belongs
	/// @param moduleDecls All variable declarations of the automaton (clocks included)
	/// @return IOSA transition or nullptr if translation failed
	/// @throw FigException if JANI edge specifiaction is badly formated
	shared_ptr<TransitionAST> build_IOSA_transition_from_STA(
			const Json::Value& JANIedge,
			const std::string& moduleName,
			const shared_vector<Decl>& moduleDecls);

	/// Get IOSA precondition translated from this JANI guard,
	/// interpreting the JANI automaton as an STA
	/// @param JANIguard Json object with the Expression of an edge's guard
	/// @param clocks    Declarations of all clocks in the current automaton
	/// @return pair.first: IOSA precondition or nullptr if translation failed
	///         pair.second: Clock location if the guard belongs to an output
	///                      edge, nullptr otherwise
	/// @throw FigException if JANI guard specifiaction is badly formated
	/// @note The precondition returned as the first component of the pair
	///       is stripped from the STA expression with the clock comparison
	std::pair< std::shared_ptr<Exp>,
			   std::shared_ptr<Location> >
	build_IOSA_precondition_from_STA(const Json::Value& JANIguard,
									 const shared_vector<Decl>& clocks);

	/// Get IOSA postcondition from given JANI edge destinations vector
	/// @param JANIdest     Json array with JANI edge-detinations to translate
	/// @param moduleVars   All variables of the current automaton (no clocks)
	/// @param moduleClocks All clock variables of the current automaton
	/// @return IOSA transition postcondition or empty vector if translation
	///         failed (or if JANIdest is has no assignments)
	/// @throw FigException if JANI edge-destinations specifiaction is badly formated
	shared_vector<Effect> build_IOSA_postcondition(
			const Json::Value& JANIdest,
			const shared_vector<Decl>& moduleVars,
			const shared_vector<Decl>& moduleClocks = shared_vector<Decl>());

	/// Get IOSA property from given JANI property specifiaction
	/// @param JANIprop Json object with the Property to translate
	/// @return IOSA property or nullptr if translation failed
	/// @throw FigException if JANI Property specifiaction is badly formated
	std::shared_ptr<Prop> build_IOSA_property(const Json::Value& JANIprop);

	/// Get IOSA transient property from given JANI property expression
	/// coming from the 'exp' field of a Pmin/Pmax property
	/// @param JANIpexp Json object with the PropertyExpression to translate
	/// @return IOSA transient property or nullptr if translation failed
	/// @throw FigException if JANI PropertyExpression specifiaction is badly formated
	/// @note Pmin/Pmax can only be translated to a IOSA transient property
	///       if they contain a single 'U' (until) sub-property
	shared_ptr<TransientProp> build_IOSA_transient_property(const Json::Value& JANIpexp);
};

} // namespace fig

#endif // JANI_TRANSLATOR_H
