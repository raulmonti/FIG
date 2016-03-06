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
#include <PropertySat.h>
#include <Module.h>

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
using ImportanceVec = fig::ImportanceFunction::ImportanceVec;
typedef ImportanceVec EventVec;
typedef unsigned STATE_T;


/**
 * @brief Build reversed edges of a Module
 *
 *        Starting from the initial state of "module" and following its
 *        transitions, compute all reachable edges and store them reversed.
 *
 * @param module Module with all transitions information
 * @param visits Vector used to mark visited states <b>(modified)</b>
 *
 * @return Concrete states' graph reversed adjacency list, viz. a vector
 *         the size of the concrete state space, whose i-th position has all
 *         the states that reach the i-th concrete state in "module"
 *
 * @note "visits" should be provided empty, and is reallocated
 *       to the size of "state.concrete_size()"
 *
 * @warning <b>Memory complexity</b>: <i>O(N*M)</i>, where
 *          <ul>
 *          <li><i>N</i> is the \ref State.concrete_size() "concrete state
 *                       space size" of "module" and</li>
 *          <li><i>M</i> is the number of <b>concrete edges</b> of "module"</li>
 *          </ul>
 *          Thus in the worst case scenario this allocates
 *          <b>sizeof</b>(unsigned)*N<sup>3</sup> bytes.
 *          However that'd require "module" to have a dense transition matrix,
 *          which is seldom the case.
 */
fig::AdjacencyList
reversed_edges_DFS(const fig::Module& module,
				   ImportanceVec& visits)
{
	const ImportanceValue NOT_VISITED(fig::EventType::STOP);
	const ImportanceValue     VISITED(0);

	assert(NOT_VISITED != VISITED);
	assert(visits.size() == 0u);

	// STL's forward_list is the perfect stack
	std::forward_list< size_t > toVisit;
	toVisit.push_front(module.initial_concrete_state());
	fig::AdjacencyList rEdges(module.concrete_state_size());
	ImportanceVec(module.concrete_state_size(), NOT_VISITED).swap(visits);

	// DFS
	while (!toVisit.empty()) {
		size_t currentState = toVisit.front(); toVisit.pop_front();
		if (VISITED == visits[currentState])
			continue;
		// Visiting currentState
		visits[currentState] = VISITED;
		auto nextStatesList = module.adjacent_states(currentState);
        for (const auto& s: nextStatesList)
            rEdges[s].push_front(currentState);  // s <-- currentState
		// Push into the 'toVisit' stack only the new unvisited states
		nextStatesList.remove_if( [&] (const size_t& s) -> bool
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
    const size_t numRares = raresQueue.size();
	std::queue< STATE_T >& statesToCheck = raresQueue;

	while (!initialReached && !statesToCheck.empty()) {
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

    assert(initialReached || cStates.size() == numRares);
    const ImportanceValue maxDistance(fig::UNMASK(cStates[initialState]));
	// Is ImportanceValue bit-representation big enough?
	if (maxDistance & ALL_MASKS || maxDistance >= NOT_VISITED)
		throw_FigException(std::string("too many importance levels were found (")
						   .append(std::to_string(maxDistance)).append(")"));

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
 * @param globalState Any valid State of the Module
 * @param cStates     Concrete states vector to label with Event masks <b>(modified)</b>
 * @param property    Property identifying the special states
 * @param returnRares Whether to return a queue with all rare concrete states
 *
 * @note cState will be left of size "globalState.concrete_size()",
 *       and its content will be modified to contain only those values
 *       defined in fig::EventType.
 *
 * @warning Intended for global State, i.e. parameter 'globalState' should have
 *          the variables of the whole \ref fig::ModuleNetwork "user model",
 *          not just from an \ref fig::ModuleInstance "individual module".
 */
std::queue< STATE_T >
label_states(State globalState,
			 EventVec& cStates,
			 const Property& property,
			 bool returnRares = false)
{
	cStates.resize(globalState.concrete_size());
	std::queue< STATE_T > raresQueue;

	// Mark conditions according to the property
	switch (property.type) {

	case fig::PropertyType::TRANSIENT: {
		auto transientProp = static_cast<const fig::PropertyTransient&>(property);
		for (size_t i = 0ul ; i < globalState.concrete_size() ; i++) {
			cStates[i] = fig::EventType::NONE;
			const StateInstance valuation(globalState.decode(i).to_state_instance());
			if ( transientProp.expr2(valuation)) {
				fig::SET_RARE_EVENT(cStates[i]);
				if (returnRares)
					raresQueue.push(i);
			}
			if (!transientProp.expr1(valuation))
				fig::SET_STOP_EVENT(cStates[i]);
		}
		} break;

	case fig::PropertyType::THROUGHPUT:
	case fig::PropertyType::RATE:
	case fig::PropertyType::RATIO:
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
 * @brief Label concrete states with Event masks corresponding to the Property
 *
 *        This is specifically intended for ImporatanceFunctionConcreteSplit,
 *        where the Property is to be evaluated locally in each of the modules,
 *        i.e. in a local state with only a subset of the variables that may
 *        appear in the Property.<br>
 *        SAT solving is used to evaluate the Property in these "incomplete
 *        local states", marking any which could add to a global rare state.
 *
 * @param localState Any valid State of the Module
 * @param cStates    Concrete states vector to label with Event masks <b>(modified)</b>
 * @param property   Property identifying the special states
 *
 * @return Queue with all rare concrete states
 *
 * @note cState will be left of size "localState.concrete_size()",
 *       and its content will be modified to contain only those values
 *       defined in fig::EventType.
 */
std::queue< STATE_T >
label_states_with_SAT(State localState,
					  EventVec& cStates,
					  const Property& property)
{
	cStates.resize(localState.concrete_size());
	std::queue< STATE_T > raresQueue;
	fig::PropertySat reducedProperty(property.index(), localState.varnames());

	// Mark conditions according to the property
	switch (property.type) {

	case fig::PropertyType::TRANSIENT:
		for (size_t i = 0ul ; i < localState.concrete_size() ; i++) {
			cStates[i] = fig::EventType::NONE;
			const StateInstance valuation(localState.decode(i).to_state_instance());
			if (reducedProperty.sat(1, valuation)) {
                fig::SET_RARE_EVENT(cStates[i]);
                raresQueue.push(i);
            }
            if (reducedProperty.nsat(0, valuation))  // expression's negation SAT
				fig::SET_STOP_EVENT(cStates[i]);
		}
		break;

	case fig::PropertyType::THROUGHPUT:
	case fig::PropertyType::RATE:
	case fig::PropertyType::RATIO:
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
assess_importance_flat(const State& state,
					   ImportanceVec& impVec,
					   const Property& property)
{
    assert(state.size() > 0ul);
    assert(impVec.size() == 0ul);

	// Build vector the size of concrete state space filled with zeros ...
	ImportanceVec(state.concrete_size()).swap(impVec);
	// ... and label according to the property
	label_states(state, impVec, property);

	return static_cast<ImportanceValue>(0u);
}


/**
 * Assign automatic importance to reachable states in concrete vector
 *
 * @param module   Module whose states' will have their importance assessed
 * @param impVec   Vector where the importance will be stored <b>(modified)</b>
 * @param property Property identifying the special states
 * @param split    Whether we're working for ImportanceFunctionConcreteSplit
 *
 * @note "impVec" should be provided empty, and is reallocated
 *       to the size of "state.concrete_size()"
 *
 * @return Maximum importance, i.e. importance of any rare state
 */
ImportanceValue
assess_importance_auto(const fig::Module& module,
					   ImportanceVec& impVec,
					   const Property& property,
					   bool split)
{
    assert(impVec.size() == 0ul);

	// Step 1: run DFS from initial state to compute reachable reversed edges
	fig::AdjacencyList reverseEdges = reversed_edges_DFS(module, impVec);
	/// @todo NOTE: there's a more memory friendly way,
	///       without holding the whole adjacency list in a variable.
	///       It requires to build the reaching states for each concrete state
	///       on each step of build_importance_BFS(), on demand,
	///       and discard them when the BFS step is finished.

	// Step 2: label concrete states according to the property
	std::queue< STATE_T > raresQueue;
	if (split)
		raresQueue = label_states_with_SAT(module.initial_state(), impVec, property);
	else
		raresQueue = label_states(module.initial_state(), impVec, property, true);

	// Step 3: run BFS to compute importance of every concrete state
	ImportanceValue maxImportance = build_importance_BFS(reverseEdges,
														 raresQueue,
														 module.initial_concrete_state(),
														 impVec);
	fig::AdjacencyList().swap(reverseEdges);  // free mem!

	return maxImportance;
}

} // namespace



namespace fig
{

ImportanceFunctionConcrete::ImportanceFunctionConcrete(
	const std::string& name,
	const State<STATE_INTERNAL_TYPE>& globalState) :
		ImportanceFunction(name),
		modulesConcreteImportance(1u),
		globalStateCopy(globalState)
{ /* Not much to do around here */ }


ImportanceFunctionConcrete::~ImportanceFunctionConcrete()
{
	clear();
}


void
ImportanceFunctionConcrete::assess_importance(
	const Module& module,
	const Property& property,
	const std::string& strategy,
	const unsigned& index)
{
	if (modulesConcreteImportance.size() <= index)
		modulesConcreteImportance.resize(index+1);
    else if (modulesConcreteImportance[index].size() > 0ul)
		throw_FigException(std::string("importance info already exists at ")
						   .append(" position").append(std::to_string(index)));

	// Compute importance according to the chosen strategy
	if ("flat" == strategy) {
		maxValue_ =
			assess_importance_flat(module.initial_state(),
								   modulesConcreteImportance[index],
								   property);
        // Invariant of flat importance function:
		minValue_ = maxValue_;
		minRareValue_ = maxValue_;

	} else if ("auto" == strategy) {
		maxValue_ =
			assess_importance_auto(module,
								   modulesConcreteImportance[index],
								   property,
								   name().find("split") != std::string::npos);
        // For auto importance functions the initial state has always
        // the lowest importance, and all rare states have the highest:
		minValue_ = UNMASK(
				modulesConcreteImportance[index][module.initial_concrete_state()]);
		minRareValue_ = maxValue_;  // should we check?

	} else if ("adhoc" == strategy) {
        throw_FigException("importance strategy \"adhoc\" requires a user "
                           "defined formula expression; this routine should "
                           "not have been invoked for such strategy.");
	} else {
		throw_FigException(std::string("unrecognized importance assessment ")
						   .append("strategy \"").append(strategy).append("\"")
						   .append(". See available options with ModelSuite::")
						   .append("available_importance_strategies()"));
	}
}


void
ImportanceFunctionConcrete::clear() noexcept
{
	for (unsigned i = 0u ; i < modulesConcreteImportance.size() ; i++)
		ImportanceVec().swap(modulesConcreteImportance[i]);
		// Clear vector and ensure reallocation to 0 capacity
		// http://www.cplusplus.com/reference/vector/vector/clear/
	std::vector<ImportanceVec>().swap(modulesConcreteImportance);
	ImportanceFunction::clear();
}

} // namespace fig
