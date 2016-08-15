/* Leonardo Rodríguez */

#ifndef PROPERTY_PROJ_H
#define PROPERTY_PROJ_H

// C++
#include <vector>
#include <utility>  // std::pair<>
// FIG
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
 * importance vectors used by ImportanceFunctionConcreteSplit.
 * The idea is to offer an easy and fast projection of the Property's
 * clauses over each individual module's variables.
 *
 * @warning The \ref fig::Property "property" to parse must be in DNF.
 */
class PropertyProjection {
	/// Index of the last property with which the instance was populated()
	unsigned propIdx_;
public:
	typedef fig::Precondition Clause;  // DNF clause: (l1 && l2 && ... && ln)
	typedef fig::State< fig::STATE_INTERNAL_TYPE > State;

private:  // Attributes
	/// Clauses corresponding to the rare events identification
	/// DNF rares_;

	/// Clauses corresponding to stopping/reference/etc events identification
	/// DNF others_;
    
public:  // Ctor/Dtor

	/// Default empty ctor
    PropertyProjection();

	/// Build and populate() with passed propery
    PropertyProjection(const fig::Property& property);

	/// Dtor frees internal memory
    ~PropertyProjection();

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

#endif
