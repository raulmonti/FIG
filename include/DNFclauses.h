//==============================================================================
//
//  DNFclauses.h
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

#ifndef DNFCLAUSES_H
#define DNFCLAUSES_H

// C++
#include <vector>
#include <utility>  // std::pair<>
// FIG
#include <Ast.h>
#include <State.h>
#include <Property.h>
#include <Precondition.h>


namespace parser
{

using std::vector;

/**
 * Formatted container for a Property in Disjunctive Normal Form
 *
 * This class was designed for the construction of the concrete "split"
 * Importance vectors used by ImportanceFunctionConcreteSplit.
 * The idea is to offer an easy and fast projection of the Property's
 * clauses over each individual module's variables.
 *
 * @warning The \ref fig::Property "property" to parse must be in DNF.
 */
class DNFclauses
{
	/// Index of the last property with which the instance was populated()
	unsigned propIdx_;

public:

	typedef fig::Precondition Clause;  // DNF clause: (l1 && l2 && ... && ln)
	typedef fig::State< fig::STATE_INTERNAL_TYPE > State;
	typedef vector< vector< AST* > > DNF;

private:  // Attributes

	/// Clauses corresponding to the rare events identification
	DNF rares_;

	/// Clauses corresponding to stopping/reference/etc events identification
	DNF others_;

public:  // Ctor/Dtor

	/// Default empty ctor
	DNFclauses();

	/// Build and populate() with passed propery
	DNFclauses(const fig::Property& property);

	/// Free internal memory
	~DNFclauses();

public:  // Utils

	/// Fill in this instance with the contents of the passed property.
	/// If the same property had been last used for population, nothing is done.
	void populate(const fig::Property& property);

	/// Project our DNF clauses over the variables set of the given local state
	/// @return pair.first: projected clauses corresponding to the rare event<br>
	///         pair.second: projected clauses corresponding to the stopping/reference/etc event
	/// @throw FigException if the instance hasn't been populated yet
	std::pair <
		vector< Clause >,
		vector< Clause >
	> project(const State& localState) const;
};

} // namespace parser

#endif // DNFCLAUSES_H
