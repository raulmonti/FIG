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
 * @brief Parse the formula as a list of DNF clauses
 * @param DNFformula AST of a logical formula assumed to be in DNF
 * @return Vector of DNF clauses: each is a conjunction of literals
 *         (i.e. boolean values or comparisons of arithmetic values)
 *
 * @todo FIXME As soon as Raúl implements it, use the "outer-parenthesis-deleter"
 *             function on every AST before calling "get_all_ast_ff()" on them
 */
DNF extract_clauses(AST& DNFformula)
{
	DNF clauses;
	for (AST* clause: DNFformula.get_all_ast_ff(parser::_EQUALITY))
		clauses.emplace_back(clause->get_first(parser::_EXPRESSION)
								   ->get_all_ast_ff(parser::_EQUALITY));
	return clauses;
}


/// @todo TODO write docstring
vector< Clause >
project(const DNF& clauses, const vector< std::string >& varnames)
{
	vector< Clause > dnfClauses;
	const std::string AMP(" & ");
	auto include =
		[&] (const std::string& literal) -> bool
		{ return std::find(begin(varnames), end(varnames), literal) != end(varnames); };
	for (const auto& clause: clauses) {
		std::string clauseSTR;
		for (const AST* literal: clause) {
			const std::string literalSTR(literal->toString());
			if (include(literalSTR))
				clauseSTR += literalSTR + AMP;
		}
		if (!clauseSTR.empty()) {
			clauseSTR.resize(clauseSTR.length() - AMP.length());
			dnfClauses.emplace_back(clauseSTR, varnames);
		}
	}
	return dnfClauses;
}

} // namespace



namespace parser
{

DNFclauses::DNFclauses() :
	propIdx_(0)
{ /* Not much to do around here */ }


DNFclauses::DNFclauses(const fig::Property& property) :
	propIdx_(property.index())
{
	populate(property);
	assert(rares_.size() > 0ul);
}


DNFclauses::~DNFclauses()
{
	for (auto& vec: rares_)
		vec.clear();
	for (auto& vec: others_)
		vec.clear();
	DNF().swap(rares_);
	DNF().swap(others_);
}


void
DNFclauses::populate(const fig::Property& property)
{
	if (property.index() == static_cast<int>(propIdx_) && !rares_.empty())
		return;  // nothing to do

	assert(GLOBAL_PROP_AST);
	std::vector<AST*> ASTprops(GLOBAL_PROP_AST->get_all_ast(parser::_PROPERTY));
	assert(property.index() < static_cast<int>(ASTprops.size()));
	AST& ASTprop(*ASTprops[property.index()]);

	switch (property.type) {

	case fig::PropertyType::TRANSIENT: {
		AST& ASTtransient(*ASTprop.get_first(parser::_PPROP));
		std::vector< AST* > transientFormulae(ASTtransient.get_list(parser::_EXPRESSION));
		assert(transientFormulae.size() == 2ul);
		rares_ = extract_clauses(*transientFormulae[1]);
		others_ = extract_clauses(*transientFormulae[0]);
		} break;

	case fig::PropertyType::RATE: {
		AST& ASTrate(*ASTprop.get_first(parser::_SPROP));
		std::vector< AST* > rateFormulae(ASTrate.get_list(parser::_EXPRESSION));
		assert(rateFormulae.size() == 1ul);
		rares_ = extract_clauses(*rateFormulae[0]);
		others_.clear();
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

	/// @todo TODO erase debug print
	std::cerr << "Rares projection: ";
	for (const Clause& clause: rares)
		std::cerr << clause.expression() << " | ";
	std::cerr << "\b\b  \n";
	///////////////////////////////

	auto others = ::project(others_, VARNAMES);
	for (Clause& clause: others)
		clause.pin_up_vars(localState);

	/// @todo TODO erase debug print
	std::cerr << "Others projection: ";
	for (const Clause& clause: others)
		std::cerr << clause.expression() << " | ";
	std::cerr << "\b\b  \n";
	///////////////////////////////

	return std::make_pair(rares, others);
}

} // namespace parser
