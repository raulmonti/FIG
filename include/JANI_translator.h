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
	 * Translate existing IOSA model file to <a href="http://jani-spec.org/">
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
	 * inside a "properties...endproperties" section of the IOSA file.
	 *
	 * @param janiModelFile Path to (or name of) file with STA model
	 *                      written in valid JANI-spec format
	 * @param iosaFilename  Desired name of the translated IOSA file to create
	 *
	 * @return Name of file with the model translated to IOSA syntax (see notes)
	 *
	 * @throw FigException if we couldn't generate a valid IOSA translation,
	 *                     e.g. when the JANI model wasn't a deterministic STA
	 *
	 * @note If 'iosaFilename' was passed then the IOSA file generated will
	 *       have that name. Otherwise a name related to 'janiModelFile' is
	 *       automatically generated.
	 */
	std::string
	JANI_2_IOSA(const std::string& janiModelFile,
				const std::string& iosaFilename = "");

private:  // Class attributes

	/// JsonCPP of the last parsed model, in JANI Specifiaction format
	shared_ptr< Json::Value > JANIroot_;

	/// Current JSON field to fill in with info from last parsed IOSA model
	shared_ptr< Json::Value > JANIfield_;

	/// Name of the module currently translated (visited)
	std::string currentModule_;

	/// Scope of the module currently translated (visited)
	shared_ptr<ModuleScope> currentScope_;

	/// Input/Output label sets of a module
	typedef std::pair< std::set< std::string >,   // inputs
					   std::set< std::string > >  // outputs/tau
		LabelSets;

	/// Labels of each module (for the currently parsed model)
	std::map< std::string, LabelSets > modulesLabels_;

	/// All model labels grouped together without discrimination
	std::set< std::string > modelLabels_;

	/// Invariant needed for IOSA -> STA translation to make time progress
	/// @note Updated by build_JANI_guard()
	/// @note Reset by visit(shared_ptr<ModuleAST>)
	shared_ptr< Json::Value> timeProgressInvariant_;

	/// Prefix used to generate a real variable from a clock
	/// @note Needed for IOSA -> STA translation
	static constexpr char REAL_VAR_FROM_CLOCK_PREFIX[] = "x_";

	/// An empty Json::Value of "object" type
	static const Json::Value EMPTY_JSON_OBJ;

	/// An empty Json::Value of "array" type
	static const Json::Value EMPTY_JSON_ARR;

private:  // Class utils

	/// Get the name of the real variable corresponding to this clock name
	/// @see REAL_VAR_FROM_CLOCK_PREFIX
	static std::string rv_from(const std::string& clockName);

	/// Try to evaluate an expression to an integral value;
	/// put an error in our ErrorMessage if unsuccessfull
	int get_int_or_error(shared_ptr<Exp> exp, const std::string& msg);

	/// Try to evaluate an expression to a boolean value;
	/// put an error in our ErrorMessage if unsuccessfull
	bool get_bool_or_error(shared_ptr<Exp> exp, const std::string& msg);

	/// Try to evaluate an expression to a floating point value;
	/// put an error in our ErrorMessage if unsuccessfull
	float get_float_or_error(shared_ptr<Exp> exp, const std::string& msg);

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
};

} // namespace fig

#endif // JANI_TRANSLATOR_H
