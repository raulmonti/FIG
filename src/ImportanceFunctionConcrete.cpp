//==============================================================================
//
//  ImportanceFunctionConcrete.cpp
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
#include <vector>
#include <forward_list>
// FIG
#include <ImportanceFunctionConcrete.h>
#include <FigException.h>
#include <Transition.h>
#include <Property.h>


namespace
{

using fig::Property;
using fig::Transition;
using fig::StateInstance;
using fig::ImportanceValue;
using State = fig::State< fig::STATE_INTERNAL_TYPE >;
using ImportanceVec = fig::ImportanceFunctionConcrete::ImportanceVec;
typedef unsigned STATE_T;


/**
 * @brief Return all (concrete) states that could be reached in one jump
 *        from "state", according to the transitions vector "trans".
 * @note <b>Complexity:</b> <i>O(size(trans) * size(state)<sup>2</sup>)</i>
 */
std::forward_list< STATE_T >
adjacent_states(const State& state,
				const std::vector< std::shared_ptr< Transition > >& trans)
{
	std::forward_list< STATE_T > adjacentStates;
	StateInstance si(state.to_state_instance());
	State s(state);
	for (const auto tr_ptr: trans) {
		if (tr_ptr->precondition()(si)) {
			tr_ptr->postcondition()(si);
			s.copy_from_state_instance(si);
			adjacentStates.push_front(s.encode());
			state.copy_to_state_instance(si);  // restore original state
		}
	}
	return adjacentStates;
}


/**
 * @brief Assign null importance for all states in concrete vector
 * @param state    Symbolic state of this Module, in any valuation
 * @param impVec   Vector where the importance will be stored
 * @param property Property identifying the special states
 */
void
assess_importance_flat(State state,
					   ImportanceVec& impVec,
					   const Property& property)
{
	ImportanceVec(state.concrete_size(),
				  static_cast<ImportanceValue>(0)).swap(impVec);
	for (size_t i = 0 ; i < state.concrete_size() ; i++)
		if (property.is_rare(state.decode(i)))
			fig::SET_RARE_EVENT(impVec[i]);
}


/**
 * @brief Assign automatic importance for reachable states in concrete vector
 * @param state    Symbolic state of this Module, at its initial valuation
 * @param trans    All the transitions of this Module
 * @param ivec     Vector where the importance will be stored
 * @param property Property identifying the special states
 */
void
assess_importance_auto(State state,
					   const std::vector< std::shared_ptr< Transition > >& trans,
					   ImportanceVec& impVec,
					   const Property& property)
{
	const ImportanceValue NOT_VISITED(fig::EventType::STOP);
	const ImportanceValue     VISITED(0);

	assert(NOT_VISITED != VISITED);
	assert(NOT_VISITED != fig::EventType::RARE);
	assert(impVec.size() == 0u);

	ImportanceVec& visitedStates = impVec;
	ImportanceVec(state.concrete_size(), NOT_VISITED).swap(visitedStates);

	std::forward_list< STATE_T > toVisit;
	toVisit.push_front(state.encode());  // initial state
	std::vector< std::forward_list< STATE_T > > rEdges(state.concrete_size());

	// Step 1: run DFS to store reversed edges in "rEdges[]"
	while (!toVisit.empty()) {
		STATE_T currentState = toVisit.front();
		toVisit.pop_front();
		visitedStates[currentState] = VISITED;
		state.decode(currentState);
		auto nextStatesList = adjacent_states(state, trans);
		for (const auto& s: nextStatesList)
			rEdges[s].push_front(currentState);  // s <-- currentState
		// Push into the 'toVisit' stack only the new unvisited states
		for (auto it = nextStatesList.begin(),
				  prev = nextStatesList.before_begin()
			; it != nextStatesList.end()
			; prev++, it++) {
			if (VISITED == visitedStates[*it]) {
				nextStatesList.remove(*it);  // invalidates it
				it = prev;
				it++;
			}
		}
		toVisit.splice_after(toVisit.before_begin(), std::move(nextStatesList));
	}

	// Step 2: mark rare states, the foundings of importance assessment
	std::forward_list< STATE_T > rareStates;
	for (STATE_T i = 0 ; i < state.concrete_size() ; i++) {
		if (property.is_rare(state.decode(i))) {
			impVec[i] = 0;
			rareStates.push_front(i);
		} else {
			impVec[i] = NOT_VISITED;
		}
		/// @todo TODO: switch per 'property' and mark other events than rare?
	}

	// Step 3: run BFS to store distances in "ivec[]" using "rEdges[]"

	/// @todo TODO implement algorithm from sheet,
	///       the one with 'state[]' and 'redges[]' arrays.
	///       "ivec[]" would be the 'state[]'

}

} // namespace


namespace fig
{

ImportanceFunctionConcrete::ImportanceFunctionConcrete(const std::string& name) :
	ImportanceFunction(name),
	modulesConcreteImportance(1u)
{ /* Not much to do around here */ }


ImportanceFunctionConcrete::~ImportanceFunctionConcrete()
{
	clear();
}


void
ImportanceFunctionConcrete::assess_importance(
	const State<STATE_INTERNAL_TYPE>& symbState,
	const std::vector< std::shared_ptr<Transition> >& trans,
	const Property& property,
	const std::string& strategy,
	const unsigned& index)
{
	if (modulesConcreteImportance.size() <= index)
		modulesConcreteImportance.resize(index);
	else if (modulesConcreteImportance[index].size() > 0u)
		throw_FigException(std::string("importance info already exists at ")
						   .append(" position").append(std::to_string(index)));

	// Compute importance according to the chosen strategy
	if ("" == strategy || "flat" == strategy) {
		assess_importance_flat(symbState,
							   modulesConcreteImportance[index],
							   property);

	} else if ("auto" == strategy) {
		assess_importance_auto(symbState,
							   trans,
							   modulesConcreteImportance[index],
							   property);

	} else if ("adhoc" == strategy) {
		throw_FigException(std::string("importance strategy \"").append(strategy)
						   .append("\" isn't supported yet"));
	} else
		throw_FigException(std::string("unrecognized importance strategy \"")
						   .append(strategy).append("\""));
}


void
ImportanceFunctionConcrete::clear() noexcept
{
	for (unsigned i = 0u ; i < modulesConcreteImportance.size() ; i++)
		this->clear(i);
	std::vector<ImportanceVec>().swap(modulesConcreteImportance);
}


void
ImportanceFunctionConcrete::clear(const unsigned& index) noexcept
{
	if (modulesConcreteImportance.size() > index)
		ImportanceVec().swap(modulesConcreteImportance[index]);
		// Clear vector and ensure reallocation to 0 capacity
		// http://www.cplusplus.com/reference/vector/vector/clear/
}

} // namespace fig
