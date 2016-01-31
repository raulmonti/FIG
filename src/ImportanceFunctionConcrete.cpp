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
#include <tuple>
#include <queue>
#include <vector>
#include <forward_list>
#include <algorithm>
// FIG
#include <ImportanceFunctionConcrete.h>
#include <FigException.h>
#include <Transition.h>
#include <Property.h>
#include <PropertyTransient.h>
#include <ModelSuite.h>

// ADL
using std::begin;
using std::end;


namespace
{

using fig::Property;
using fig::Transition;
using fig::StateInstance;
using fig::ImportanceValue;
using State = fig::State< fig::STATE_INTERNAL_TYPE >;
using ImportanceVec = fig::ImportanceFunctionConcrete::ImportanceVec;
typedef ImportanceVec EventVec;
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
 * @brief Build reversed edges of a Module
 *
 *        Starting from the initial "state" provided and following the
 *        given list of transitions "trans", compute the reachable edges
 *        of the inherent Module and store them reversed.
 *
 * @param state  Symbolic state with the initial valuation of the Module
 * @param trans  All the transitions of the Module, order unimportant
 * @param visits Vector used to mark visited states <b>(modified)</b>
 *
 * @return Concrete states' graph reversed adjacency list, viz. a vector
 *         the size of the concrete state space, whose i-th position has all
 *         the states that reach the i-th concrete state according to "trans"
 *
 * @note "visits" should be provided empty, and is reallocated
 *       to the size of "state.concrete_size()"
 *
 * @warning <b>Memory complexity</b>: <i>O(N*M)</i>, where
 *          <ul>
 *          <li><i>N</i> is the \ref State.concrete_size() "concrete size" of "state" and</li>
 *          <li><i>M</i> is the number of <b>concrete edges</b> of the module</li>
 *          </ul>
 *          Thus in the worst case scenario this allocates
 *          <b>sizeof</b>(unsigned)*N<sup>3</sup> bytes.
 *          However that'd require the Module to have a dense transition matrix,
 *          which is seldom the case.
 */
fig::AdjacencyList
reversed_edges_DFS(State state,
				   const std::vector< std::shared_ptr< Transition > >& trans,
				   ImportanceVec& visits)
{
	const ImportanceValue NOT_VISITED(fig::EventType::STOP);
	const ImportanceValue     VISITED(0);

	assert(NOT_VISITED != VISITED);
	assert(visits.size() == 0u);

	// STL's forward_list is the perfect stack
	std::forward_list< STATE_T > toVisit;
	toVisit.push_front(state.encode());  // initial state
	fig::AdjacencyList rEdges(state.concrete_size());
	ImportanceVec(state.concrete_size(), NOT_VISITED).swap(visits);

	// DFS
	while (!toVisit.empty()) {
		STATE_T currentState = toVisit.front();
		toVisit.pop_front();
		if (VISITED == visits[currentState])
			continue;
		visits[currentState] = VISITED;  // visiting currentState
		state.decode(currentState);
		auto nextStatesList = adjacent_states(state, trans);
		for (const auto& s: nextStatesList)
			rEdges[s].push_front(currentState);  // s <-- currentState
		// Push into the 'toVisit' stack only the new unvisited states
		nextStatesList.remove_if( [&] (const STATE_T& s) -> bool
								  { return VISITED == visits[s]; } );
		toVisit.splice_after(toVisit.before_begin(), std::move(nextStatesList));
	}

	return rEdges;
}


/**
 * @brief Compute importance of all concrete states in the Module.
 *
 *        Using 'reverseEdges' to perform a backwards reachability search,
 *        compute the distance from every concrete state in 'cStates'
 *        to the nearest rare state. The inversion of those values is
 *        taken as the importance of the states.<br>
 *
 * @pre  All states in 'raresQueue' should be marked as rare in 'cStates',
 *       viz. for (auto s: raresQueue) assert(fig::IS_RARE_EVENT(cStates[s]))
 *
 * @param reverseEdges Reversed edges of the Module, as built by reversed_edges_DFS()
 * @param raresQueue   Queue with all the (concrete) rare states of the Module <b>(modified)</b>
 * @param initialState Single (concrete) initial state of the Module
 * @param cStates      Concrete states vector where the importance is stored <b>(modified)</b>
 *
 * @note BFS search stops as soon as the 'initialState' is encountered in the
 *       backwards search, since no state can have a lower importance than the
 *       initial one.
 *
 * @return Maximum importance, i.e. importance of any rare state
 */
ImportanceValue
build_importance_BFS(const fig::AdjacencyList& reverseEdges,
					 std::queue< STATE_T >& raresQueue,
					 const size_t initialState,
					 ImportanceVec& cStates)
{
	assert(raresQueue.size() > 0);
//	assert(std::find(begin(raresQueue), end(raresQueue), initialState)
//		   == end(raresQueue));

	const ImportanceValue ALL_MASKS (fig::EventType::RARE
									|fig::EventType::STOP
									|fig::EventType::REFERENCE
									|fig::EventType::THR_UP
									|fig::EventType::THR_DOWN);
	const ImportanceValue NOT_VISITED (static_cast<ImportanceValue>(~ALL_MASKS));

	// Initially: 0 distance for rare states
	//            maximum representable distance for the rest
	for (STATE_T s = 0u ; s < cStates.size() ; s++) {
		if (fig::IS_RARE_EVENT(cStates[s]))
			cStates[s] = 0 | fig::MASK(cStates[s]);
		else
			cStates[s] = NOT_VISITED | fig::MASK(cStates[s]);
	}

	// BFS
	bool initialReached(false);
	std::queue< STATE_T >& statesToCheck = raresQueue;
	while (!(initialReached || statesToCheck.empty())) {
		STATE_T s = statesToCheck.front();
		statesToCheck.pop();
		ImportanceValue levelBFS = fig::UNMASK(cStates[s]) + 1;
		// For each state reaching 's'...
		for (const STATE_T& reachingS: reverseEdges[s]) {
			// ...if we're visiting it for the first time...
			if (NOT_VISITED == cStates[reachingS]) {
				// ...label it with distance from rare set...
				cStates[reachingS] = levelBFS | fig::MASK(cStates[reachingS]);
				// ...and enqueue it, unless we're done.
				if (initialState == reachingS)
					initialReached = true;
				else
					statesToCheck.push(reachingS);
			}
		}
	}
	std::queue< STATE_T >().swap(statesToCheck);  // free memory

	assert(initialReached);
	const ImportanceValue maxDistance(cStates[initialState]);
	// Is ImportanceValue bit-representation big enough?
	if (maxDistance & ALL_MASKS || maxDistance >= NOT_VISITED)
		throw_FigException("too many importance levels were found");

	// Invert values in 'cStates' to obtain the importance
	#pragma omp parallel for default(shared)
	for (STATE_T s = 0u ; s < cStates.size() ; s++) {
		ImportanceValue dist = fig::UNMASK(cStates[s]);
		cStates[s] = fig::MASK(cStates[s])
				   | (dist >= maxDistance ? static_cast<ImportanceValue>(0u)
										  : static_cast<ImportanceValue>(maxDistance - dist));
	}

	return maxDistance;
}


/**
 * Label concrete states with the Event masks corresponding to the Property
 *
 * @param state    Any valid State of the Module
 * @param cStates  Concrete states vector to label with Event masks <b>(modified)</b>
 * @param property Property identifying the special states
 * @param returnRares Whether to return a queue with all rare concrete states
 *
 * @note cState will be left of size "state.concrete_size()",
 *       and its content will be modified to contain only those values
 *       defined in fig::EventType.
 */
std::queue< STATE_T >
label_states(State state,
			 EventVec& cStates,
			 const Property& property,
			 bool returnRares = false)
{
	std::queue< STATE_T > raresQueue;

	cStates.resize(state.concrete_size());

	// First mark rares
	for (size_t i = 0u ; i < state.concrete_size() ; i++) {
		cStates[i] = fig::EventType::NONE;
		if (property.is_rare(state.decode(i))) {
			fig::SET_RARE_EVENT(cStates[i]);
			if (returnRares)
				raresQueue.push(i);
		}
	}

	// Then mark other special conditions according to the property
	switch (property.type) {

	case fig::PropertyType::TRANSIENT: {
		auto transientProp = static_cast<const fig::PropertyTransient&>(property);
		for (size_t i = 0u ; i < state.concrete_size() ; i++)
			if (transientProp.is_stop(state.decode(i)))
				fig::SET_STOP_EVENT(cStates[i]);
		} break;

	case fig::PropertyType::THROUGHPUT:
	case fig::PropertyType::RATE:
	case fig::PropertyType::PROPORTION:
	case fig::PropertyType::BOUNDED_REACHABILITY:
		throw_FigException("property type isn't supported yet");
		break;

	default:
		throw_FigException("invalid property type");
		break;
	}

	return raresQueue;
}


/**
 * @brief Assign null importance for all states in concrete vector
 * @param state    Symbolic state of this Module, in any valuation
 * @param impVec   Vector where the importance will be stored <b>(modified)</b>
 * @param property Property identifying the special states
 */
ImportanceValue
assess_importance_flat(State state,
					   ImportanceVec& impVec,
					   const Property& property)
{
	// Build vector the size of concrete state space filled with zeros ...
	ImportanceVec(state.concrete_size()).swap(impVec);
	// ... and label according to the property
	label_states(state, impVec, property);
	return static_cast<ImportanceValue>(0u);
}


/**
 * Assign automatic importance to reachable states in concrete vector
 *
 * @param state    Symbolic state of this Module, at its initial valuation
 * @param trans    All the transitions of this Module
 * @param impVec   Vector where the importance will be stored <b>(modified)</b>
 * @param property Property identifying the special states
 *
 * @note "impVec" should be provided empty, and is reallocated
 *       to the size of "state.concrete_size()"
 *
 * @return Maximum importance, i.e. importance of any rare state
 */
ImportanceValue
assess_importance_auto(const State& state,
					   const std::vector< std::shared_ptr< Transition > >& trans,
					   ImportanceVec& impVec,
					   const Property& property)
{
	assert(state.size() > 0u);
	assert(trans.size() > 0u);
	assert(impVec.size() == 0u);

	// Step 1: run DFS from initial state to compute reachable reversed edges
	fig::AdjacencyList reverseEdges = reversed_edges_DFS(state, trans, impVec);
	/// @todo NOTE: there's a more memory friendly way,
	///       without holding the whole adjacency list in a variable.
	///       It requires to build the reaching states for each concrete state
	///       on each step of build_importance_BFS(), on demand,
	///       and discard them when the BFS step is finished.

	// Step 2: label concrete states according to the property
	auto raresQueue = label_states(state, impVec, property, true);

	// Step 3: run BFS to compute importance of every concrete state
	ImportanceValue maxImportance = build_importance_BFS(reverseEdges,
														 raresQueue,
														 state.encode(),
														 impVec);
	fig::AdjacencyList().swap(reverseEdges);  // free mem!

	return maxImportance;
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
		maxImportance_ =
			assess_importance_flat(symbState,
								   modulesConcreteImportance[index],
								   property);
		minRareImportance_ = static_cast<ImportanceValue>(0u);

	} else if ("auto" == strategy) {
		maxImportance_ =
			assess_importance_auto(symbState,
								   trans,
								   modulesConcreteImportance[index],
								   property);
		minRareImportance_ = maxImportance_;  // should we check?

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
