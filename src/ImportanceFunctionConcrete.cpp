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
 * @return Vector of size "state.concrete_size()", whose i'th position has all
 *         the states that reach the i'th concrete state according to "trans"
 *
 * @note "visits" should be provided empty, and is reallocated
 *       to the size of "state.concrete_size()"
 *
 * @warning <b>Memory complexity</b>: <i>O(N*M)</i>, where
 *          <ul>
 *          <li><i>N</i> = state.concrete_size() and</li>
 *          <li><i>M</i> is the number of edges of the module</li>
 *          </ul>
 *          Thus in the worst case scenario this allocates
 *          N<sup>3</sup>*<b>sizeof</b>(STATE_T) bytes.
 *          However that'd require the Module to have a dense transition matrix,
 *          which is seldom the case.
 */
std::vector< std::forward_list< STATE_T > >
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
	std::vector< std::forward_list< STATE_T > > rEdges(state.concrete_size());
	ImportanceVec(state.concrete_size(), NOT_VISITED).swap(visits);

	// DFS
	while (!toVisit.empty()) {
		STATE_T currentState = toVisit.front();
		toVisit.pop_front();
		visits[currentState] = VISITED;
		state.decode(currentState);
		auto nextStatesList = adjacent_states(state, trans);
		for (const auto& s: nextStatesList)
			rEdges[s].push_front(currentState);  // s <-- currentState
		// Push into the 'toVisit' stack only the new unvisited states
		for (auto it = nextStatesList.begin(),
				  prev = nextStatesList.before_begin()
			; it != nextStatesList.end()
			; prev++, it++) {
			if (VISITED == visits[*it]) {
				nextStatesList.remove(*it);  // it was invalidated
				it = prev;
				it++;
			}
		}
		toVisit.splice_after(toVisit.before_begin(), std::move(nextStatesList));
	}

	return rEdges;
}


/**
 * @brief Compute distance from all states to the rare set.
 *
 *        Using 'reverseEdges' to perform a backwards reachability search,
 *        label all concrete states in 'distanceVec' with their distance
 *        to the nearest rare state.<br>
 *        All states in 'raresQueue' should be marked as rare in 'distanceVec',
 *        viz. for (auto s: raresQueue) assert(fig::IS_RARE_EVENT(distanceVec[s]))<br>
 *        BFS search stops as soon as the 'initialState' is encountered in the
 *        backwards search. All unreached states are left in 'distanceVec'
 *        with a value strictly greater than the maximum distance returned.
 *
 * @param reverseEdges Reversed edges of the Module, as built by reversed_edges_DFS()
 * @param raresQueue   Queue with all the (concrete) rare states of the Module <b>(modified)</b>
 * @param initialState Single (concrete) initial state of the Module
 * @param distanceVec  Concrete states vector where distances are stored <b>(modified)</b>
 *
 * @return Maximum distance from any state to the rare set,
 *         i.e. distance from initialState to the nearest rare state
 */
ImportanceValue
rares_distance_BFS(const std::vector< std::forward_list< STATE_T > >& reverseEdges,
				   std::queue< STATE_T >& raresQueue,
				   const size_t initialState,
				   ImportanceVec& distanceVec)
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
	for (STATE_T s = 0u ; s < distanceVec.size() ; s++) {
		if (fig::IS_RARE_EVENT(distanceVec[s]))
			distanceVec[s] = 0 | fig::MASK(distanceVec[s]);
		else
			distanceVec[s] = NOT_VISITED | fig::MASK(distanceVec[s]);
	}

	// BFS
	bool initialReached(false);
	std::queue< STATE_T >& statesToCheck = raresQueue;
	while (!(initialReached || statesToCheck.empty())) {
		STATE_T s = statesToCheck.front();
		statesToCheck.pop();
		ImportanceValue levelBFS = distanceVec[s] + 1;
		// For each state reaching 's'...
		for (const STATE_T& reachingS: reverseEdges[s]) {
			// ...if we're visiting it for the first time...
			if (NOT_VISITED == distanceVec[reachingS]) {
				// ...label it with distance from rare set...
				distanceVec[reachingS] = levelBFS | fig::MASK(distanceVec[reachingS]);
				// ...and enqueue it, unless we're done.
				if (initialState == reachingS)
					initialReached = true;
				else
					statesToCheck.push(reachingS);
			}
		}
	}

	assert(initialReached);
	const ImportanceValue maxDepth(distanceVec[initialState]);
	// Is ImportanceValue bit-representation big enough?
	if (maxDepth & ALL_MASKS || maxDepth >= NOT_VISITED)
		throw_FigException("too many importance levels were found");

	// Free memory
	std::queue< STATE_T >().swap(raresQueue);

	return maxDepth;
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
void
assess_importance_flat(State state,
					   ImportanceVec& impVec,
					   const Property& property)
{
	// Build vector the size of the concrete state space ...
	ImportanceVec(state.concrete_size()).swap(impVec);
	// ... and label according to the property
	label_states(state, impVec, property);
}


/**
 * Assign automatic importance for reachable states in concrete vector
 *
 * @param state    Symbolic state of this Module, at its initial valuation
 * @param trans    All the transitions of this Module
 * @param impVec   Vector where the importance will be stored <b>(modified)</b>
 * @param property Property identifying the special states
 *
 * @note "impVec" should be provided empty, and is reallocated
 *       to the size of "state.concrete_size()"
 */
void
assess_importance_auto(const State& state,
					   const std::vector< std::shared_ptr< Transition > >& trans,
					   ImportanceVec& impVec,
					   const Property& property)
{
	assert(impVec.size() == 0u);

	// Step 1: run DFS from initial state to compute reachable reversed edges
	std::vector< std::forward_list< STATE_T > > reverseEdges =
			reversed_edges_DFS(state, trans, impVec);

	// Step 2: label concrete states according to the property
	auto raresQueue = label_states(state, impVec, property, true);

	// Step 3: run BFS to compute distance from rare states using reversed edges
	auto maxDistance = rares_distance_BFS(reverseEdges, raresQueue, state.encode(), impVec);

	// Step 4: invert values in "impVec" to obtain states importance
	#pragma omp parallel for default(shared)
	for (STATE_T s = 0u ; s < impVec.size() ; s++) {
		ImportanceValue dist = fig::UNMASK(impVec[s]);
		impVec[s] = fig::MASK(impVec[s])
				  | (dist >= maxDistance ? static_cast<ImportanceValue>(0u)
										 : static_cast<ImportanceValue>(maxDistance - dist));
	}
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
