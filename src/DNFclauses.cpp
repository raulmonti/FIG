//==============================================================================
//
//  DNFclauses.cpp
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


// C++
#include <iterator>
#include <algorithm>  // std::find()
// FIG
#include <DNFclauses.h>
#include <PropertyTransient.h>
#include <PropertyRate.h>
#include <Parser.h>

// ADL
using std::begin;
using std::end;


namespace
{

using DNF    = parser::DNFclauses::DNF;
using Clause = parser::DNFclauses::Clause;

/**
 * Parse the formula as a list of DNF clauses
 * @param DNFformula AST of a logical formula assumed to be in DNF
 * @return Vector of ASTs each of which represents a conjunction of literals,
 *         i.e. of boolean variables/values or comparisons of arithmetic variables/values
 * @note User owns memory of the returned vector
 */
DNF extract_clauses(const AST& DNFformula)
{
	using parser::Parser;

	DNF clauses;
	AST DNFformula_N = Parser::normalize_ast(&DNFformula);  // remove any outer '()'

	for (AST* clause: DNFformula_N.get_all_ast_ff(parser::_EQUALITY)) {

		AST clause_N = Parser::normalize_ast(clause);
		AST *ASTclause(clause_N.get_first(parser::_EXPRESSION));

		if (nullptr == ASTclause) {
			// May be a single boolean or arithmetic comparison
			ASTclause = clause->get_first(parser::_EQUALITY);
			if (nullptr == ASTclause)
				throw_FigException("couldn't parse the property \"" +
								   DNFformula.toString()+"\"; is it in DNF?");
			AST* ASTclause_N = new AST(Parser::normalize_ast(ASTclause));
			clauses.emplace_back(vector<AST*>({ASTclause_N}));

		} else {
			// Assume it's a conjunction of literals
			vector<AST*> literals(ASTclause->get_all_ast_ff(parser::_EQUALITY)),
						 literals_N(literals.size());
			for (size_t i = 0ul ; i < literals.size() ; i++)
				 literals_N[i] = new AST(Parser::normalize_ast(literals[i]));
			if (literals_N.size() > 0ul)
				clauses.emplace_back(literals_N);
		}
	}

	return clauses;
}


/**
 * Project 'clauses' over the set of variables names passed,
 * so that all literals ocurring in the result have at least one of them
 * @param clauses  DNF clauses to restrict
 * @param varnames Variable names defining the projection
 * @return Clauses projected over the set of variables names passed
 */
vector< Clause >
project(const DNF& clauses, const vector< std::string >& varnames)
{
	vector< Clause > dnfClauses;
	const std::string AMP(" & ");
	auto include =
		[&] (const std::string& literal) -> bool
		{
			for (const auto& var: varnames)
				if (literal.find(var) != std::string::npos)
					return true;
			return false;
		};
	for (const auto& clause: clauses) {
		std::string clauseSTR;
		for (const AST* literal: clause) {
			const std::string literalSTR(literal->toString());
			if (include(literalSTR) && clauseSTR.find(literalSTR) == std::string::npos)
				clauseSTR += literalSTR + AMP;
		}
		if (!clauseSTR.empty() &&
			end(dnfClauses) != std::find_if(
				begin(dnfClauses), end(dnfClauses),
				[&clauseSTR](const Clause& c){return c.expression()!=clauseSTR;})) {
			/// @todo TODO avoid adding repeated clauses
			clauseSTR.resize(clauseSTR.length() - AMP.length());
			dnfClauses.emplace_back(clauseSTR, varnames);
		}
	}
	return dnfClauses;
}

} // namespace



namespace parser
{

DNFclauses::DNFclauses(const fig::Property& property) :
	propIdx_(property.index())
{
	populate(property);
	assert(rares_.size() > 0ul);
}


void
DNFclauses::populate(const fig::Property& property)
{
	if (property.index() == static_cast<int>(propIdx_) && !rares_.empty())
		return;   // nothing to do
	else
		clear();  // make room for the young

	assert(GLOBAL_PROP_AST);
	std::vector<AST*> ASTprops(GLOBAL_PROP_AST->get_all_ast(parser::_PROPERTY));
	assert(property.index() < static_cast<int>(ASTprops.size()));
	AST& ASTprop(*ASTprops[property.index()]);

	switch (property.type) {

	case fig::PropertyType::TRANSIENT: {
		AST& ASTtransient(*ASTprop.get_first(parser::_PPROP));
		std::vector< AST* > transientFormulae(ASTtransient.get_list(parser::_EXPRESSION));
		assert(transientFormulae.size() == 2ul);
		rares_ = extract_clauses(const_cast<const AST&>(*transientFormulae[1]));
		others_ = extract_clauses(const_cast<const AST&>(*transientFormulae[0]));
		} break;

	case fig::PropertyType::RATE: {
		AST& ASTrate(*ASTprop.get_first(parser::_SPROP));
		std::vector< AST* > rateFormulae(ASTrate.get_list(parser::_EXPRESSION));
		assert(rateFormulae.size() == 1ul);
		rares_ = extract_clauses(const_cast<const AST&>(*rateFormulae[0]));
		} break;

	case fig::PropertyType::THROUGHPUT:
	case fig::PropertyType::RATIO:
	case fig::PropertyType::BOUNDED_REACHABILITY:
		throw_FigException("property type isn't supported yet");
		break;

	default:
		throw_FigException("invalid property type");
		break;
	}
}


std::pair < vector< Clause >, vector< Clause > >
DNFclauses::project(const State& localState) const
{
	vector< std::string > VARNAMES(localState.varnames());

	auto rares = ::project(rares_, VARNAMES);
	for (Clause& clause: rares)
		clause.pin_up_vars(localState);

	auto others = ::project(others_, VARNAMES);
	for (Clause& clause: others)
		clause.pin_up_vars(localState);

	return std::make_pair(rares, others);
}


void
DNFclauses::clear()
{
	for (auto& vec: rares_)
		for (AST* ast: vec)
			delete ast;
	for (auto& vec: others_)
		for (AST* ast: vec)
			delete ast;
	DNF().swap(rares_);
	DNF().swap(others_);
}

} // namespace parser
