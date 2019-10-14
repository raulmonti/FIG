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
#include <set>
#include <tuple>
#include <deque>
#include <queue>
#include <vector>
#include <forward_list>
#include <algorithm>  // std::fill(), std::remove_if()
// FIG
#include <ImportanceFunctionConcrete.h>
#include <FigLog.h>
#include <FigException.h>
#include <Transition.h>
#include <Property.h>
#include <PropertyRate.h>
#include <PropertyTBoundSS.h>
#include <PropertyTransient.h>
#include <PropertyProjection.h>
#include <Module.h>

// ADL
using std::begin;
using std::end;
using std::find;


namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

using uint128::uint128_t;
using fig::Property;
using fig::Transition;
using fig::StateInstance;
using fig::ImportanceVec;
using fig::ImportanceValue;
using parser::PropertyProjection;
using Clause = parser::PropertyProjection::Clause;
using State = fig::State< fig::STATE_INTERNAL_TYPE >;
using Indices = fig::ImportanceFunctionConcrete::Indices;
typedef ImportanceVec EventVec;
typedef unsigned STATE_T;


/**
 * @brief Impose a limit on the amount of memory the user can request.
 * @param concreteStateSize Size of the concrete state space of the module
 *                          and thus of the vector to be allocated
 * @param moduleName Name of the module
 * @note There's <a href="http://stackoverflow.com/a/2513561">no portable way of
 *       measuring the system's available RAM</a>, thus the limit is arbitrary
 * @throw FifException if more memory than allowed is requested
 */
void
check_mem_limits(const uint128_t& concreteStateSize, const std::string& moduleName)
{
	static const size_t MAX_MEM = fig::ImportanceFunction::MAX_MEM_REQ;  // take value!
	static const std::string MAX_GIGAS(std::to_string(MAX_MEM/(1ul<<30ul)));
	if (concreteStateSize > MAX_MEM)
		throw_FigException("the concrete state space of \"" + moduleName +
						   "\" is too big to hold it in a vector (it's greater "
						   "than " + MAX_GIGAS + " GB)");
}


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
	const size_t NUM_CONCRETE_STATES(module.concrete_state_size());

	assert(NOT_VISITED != VISITED);
	assert(visits.empty());
	if (module.concrete_state_size().upper() > 0ul)
		throw_FigException("This concrete state space is too big to build "
						   "an importance function for it -- Aborting.");

	// STL's forward_list is the perfect stack
	std::forward_list< size_t > toVisit;
	toVisit.push_front(module.initial_concrete_state());
	fig::AdjacencyList rEdges(NUM_CONCRETE_STATES);
	ImportanceVec(NUM_CONCRETE_STATES, NOT_VISITED).swap(visits);

	// DFS
	while (!toVisit.empty()) {
		const size_t currentState = toVisit.front();
		toVisit.pop_front();
		assert(visits.size() > currentState);
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
	if (raresQueue.empty())
		return static_cast<ImportanceValue>(0u);
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
#ifndef NDEBUG
	const size_t numRares = raresQueue.size();
#endif

	while (!initialReached && !statesToCheck.empty()) {
		STATE_T s = statesToCheck.front();
		statesToCheck.pop();
		ImportanceValue levelBFS = fig::UNMASK(cStates[s]) + 1;
		// For each state reaching 's'...
		for (const STATE_T& reachingS: reverseEdges[s]) {
            // ...if we're visiting it for the first time...
			if (NOT_VISITED == fig::UNMASK(cStates[reachingS])) {
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
		throw_FigException("too many importance levels were found "
						   "(" + std::to_string(maxDistance) + ")");

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
 * Mask as RARE event all concrete states which satisfy any of the clauses
 * @param s           Copy of the local State being labelled
 * @param cStates     Vector of concrete states corresponding to 's'
 * @param rareClauses DNF clauses identifying the states to mask as RARE
 * @param reset       Whether cStates should be nullified before computations
 * @return Set of concrete states found to satisfy any of the clauses,
 *         viz. set of rare concrete states
 */
std::set< STATE_T >
label_local_rares(State s,
				  EventVec& cStates,
				  const std::vector< Clause >& rareClauses,
				  const bool reset = false)
{
	std::set< STATE_T > rares;
	if (reset)
		std::fill(begin(cStates), end(cStates), fig::EventType::NONE);
	for (const auto& clause: rareClauses) {
		for (size_t i = 0ul ; i < cStates.size() ; i++) {
			if (clause(s.decode(i).to_state_instance())) {
				cStates[i] |= fig::EventType::RARE;
				rares.emplace(i);
			}
		}
	}
	return rares;
}


/**
 * Mask as EVENT all concrete states which satisfy any of the clauses
 * @param s            Copy of the local State being labelled
 * @param cStates      Vector of concrete states corresponding to 's'
 * @param otherClauses DNF clauses identifying the states to mask as EVENT
 * @param reset        Whether cStates should be nullified before computations
 * @param negate       Whether the *negation of the clauses* should be used
 *                     when masking the concrete states
*/
void
label_local_others(State s,
				   EventVec& cStates,
				   const std::vector< Clause >& otherClauses,
				   const fig::EventType& EVENT,
				   const bool reset = false,
				   const bool negate = false)
{
	if (reset)
		std::fill(begin(cStates), end(cStates), fig::EventType::NONE);
	for (const auto& clause: otherClauses)
		for (size_t i = 0ul ; i < cStates.size() ; i++)
			if (negate != clause(s.decode(i).to_state_instance()))
				cStates[i] |= EVENT;
}


/**
 * @brief Label concrete states with Event masks corresponding to the Property
 *
 *        This is specifically intended for ImportanceFunctionConcreteSplit,
 *        where the Property is to be evaluated locally in each of the modules,
 *        i.e. in a local state with only a subset of the variables that may
 *        appear in the Property.
 *
 * @param localState   State of an individual module (in any valuation)
 * @param cStates      Concrete states vector to label with Event masks <b>(modified)</b>
 * @param propertyType Type of the property identifying the special states
 * @param rareClauses  Property parsed as a DNF list of clauses
 * @param rareClauses  DNF clauses, projected from the property into this
 *                     Module's variables space, identifying the rare states
 * @param otherClauses DNF clauses, projected from the property into this
 *                     Module's variables space, identifying the rare states
 *                   (used only by ImportanceFunctionConcreteSplit for 'auto')
 *
 * @return Queue with all rare concrete states found
 *
 * @note cState will be left of size "localState.concrete_size()",
 *       and its content will be modified to contain only those values
 *       defined in fig::EventType.
 *
 * @warning Intended for modules' local states, i.e. parameter 'localState'
 *          should have the variables of a single \ref fig::ModuleInstance
 *          "individual module".
 */
std::queue< STATE_T >
label_local_states(const State& localState,
				   EventVec& cStates,
				   const fig::PropertyType& propertyType,
                   std::vector< Clause >& rareClauses,
                   std::vector< Clause >& otherClauses)
{
	std::set< STATE_T > rares;
	assert(localState.concrete_size() < (1ul<<30ul));  // bad_alloc warning bell
	cStates.resize(localState.concrete_size().lower());

	// Mark events according to the clauses
	switch (propertyType) {

	case fig::PropertyType::TRANSIENT:
		rares = label_local_rares(localState, cStates, rareClauses, true);
		label_local_others(localState, cStates, otherClauses,
						   fig::EventType::STOP, false, true);
		break;

	case fig::PropertyType::RATE:
	case fig::PropertyType::TBOUNDED_SS:
		rares = label_local_rares(localState, cStates, rareClauses, true);
		break;

	case fig::PropertyType::RATIO:
	case fig::PropertyType::BOUNDED_REACHABILITY:
		throw_FigException("property type isn't supported yet");
	}

	return std::queue<STATE_T>(std::deque<STATE_T>(begin(rares), end(rares)));
}


/**
 * Label concrete states with the Event masks corresponding to the Property
 *
 * @param globalState State of the whole model (in any valuation)
 * @param cStates     Concrete states vector to label with Event masks <b>(modified)</b>
 * @param property    Property identifying the special states
 * @param returnRares Whether to return a queue with all rare concrete states
 *
 * @return Queue with all rare concrete states found if 'returnRares' is true;
 *         empty queue otherwise.
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
label_global_states(State globalState,
					EventVec& cStates,
					const Property& property,
					bool returnRares = false)
{
	assert(globalState.concrete_size().upper() == 0ul);
	cStates.resize(globalState.concrete_size().lower());
	std::queue< STATE_T > raresQueue;

	// Mark conditions according to the property
	switch (property.type) {

	case fig::PropertyType::TRANSIENT:
	    {
		    const auto& transientProp = static_cast<const fig::PropertyTransient&>(property);
			for (auto i = 0u ; i < cStates.size() ; i++) {
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
	    }
		break;

	case fig::PropertyType::RATE:
	case fig::PropertyType::TBOUNDED_SS:
	    {
		    for (auto i = 0u ; i < cStates.size() ; i++) {
				cStates[i] = fig::EventType::NONE;
				const StateInstance valuation(globalState.decode(i).to_state_instance());
				if (property.is_rare(valuation)) {
					fig::SET_RARE_EVENT(cStates[i]);
					if (returnRares)
						raresQueue.push(i);
				}
			}
	    }
		break;

	case fig::PropertyType::RATIO:
	case fig::PropertyType::BOUNDED_REACHABILITY:
		throw_FigException("property type isn't supported yet");
	}

	return raresQueue;
}



/**
 * Assign automatic importance to reachable states in concrete vector
 *
 * @param module   Module whose states' will have their importance assessed
 * @param impVec   Vector where the importance will be stored <b>(modified)</b>
 * @param property Property identifying the special states
 * @param clauses  Property parsed as a DNF list of clauses
 *                 (used only by ImportanceFunctionConcreteSplit)
 * @param split    Whether we're working for ImportanceFunctionConcreteSplit
 * @param relevant Indices (from impVec) of concrete states that,
 *                 regardless of the property, are considered rare
 *                 <i>but only for importance construction purposes</i>.
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
					   const PropertyProjection& clauses,
                       const bool split,
                       const Indices& relevant = Indices())
{
	assert(impVec.empty());
	ImportanceValue maxImportance(0u);
	std::vector< Clause > rareClauses, otherClauses;

	// Step 0: skip computations if module is irrelevant for importance splitting
	if (split) {
		// Project the property for this Module's local variables
		std::tie(rareClauses, otherClauses) = clauses.project(module.initial_state());
		if (rareClauses.empty() && otherClauses.empty() && relevant.empty()) {
			// utterly irrelevant module, skip computations
			return maxImportance;
		}
		check_mem_limits(module.concrete_state_size(), module.id());
		if (rareClauses.empty() && relevant.empty()) {
			// module is irrelevant for importance but may hold other info
			label_local_states(module.initial_state(), impVec, property.type,
			                   rareClauses, otherClauses);
			return maxImportance;  // skip (futile) importance computation
		}
	}

	// Step 1: run DFS from initial state to compute reachable reversed edges
	fig::AdjacencyList reverseEdges = reversed_edges_DFS(module, impVec);
	/// @todo NOTE: there's a more memory friendly way,
	///       without holding the whole adjacency list in a variable.
	///       It requires to build the reaching states for each concrete state
	///       on each step of build_importance_BFS(), on demand,
	///       and discard them when the BFS step is finished.

	// Step 2: label concrete states according to the property
	std::queue< STATE_T > rares;
	if (split)
		rares = label_local_states(module.initial_state(), impVec, property.type,
								   rareClauses, otherClauses);
	else
		rares = label_global_states(module.initial_state(), impVec, property, true);

	// Step 3: run BFS to compute importance of every concrete state
	Indices skipReset;
	skipReset.reserve(relevant.size());
	for (const auto& idx: relevant) {  // include enforced relevant states
		rares.push(idx);
		if (fig::IS_RARE_EVENT(impVec[idx]))
		   skipReset.emplace_back(idx);  // already considered for importance
		else
		   fig::SET_RARE_EVENT(impVec[idx]);  // mark relevant for importance
	}
	maxImportance = build_importance_BFS(reverseEdges,
	                                     rares,
	                                     module.initial_concrete_state(),
	                                     impVec);
	fig::AdjacencyList().swap(reverseEdges);  // free mem!
	for (const auto& idx: relevant)
		if (find(begin(skipReset), end(skipReset), idx) == end(skipReset))
			impVec[idx] &= ~fig::EventType::RARE;  // clean marking

	return maxImportance;
}


/**
 * @brief Assign null importance to all states in a concrete vector
 * @param state    Symbolic state of this Module, in any valuation
 * @param impVec   Vector where the importance will be stored <b>(modified)</b>
 * @param property Property identifying the special states
 * @param clauses  Property parsed as a DNF list of clauses, for split storage
 * @param split    Whether we're working for ImportanceFunctionConcreteSplit
 * @note Did I mention using this function would be completely idiotic?
 */
ImportanceValue
assess_importance_flat(const State& state,
					   ImportanceVec& impVec,
					   const Property& property,
					   const PropertyProjection& clauses,
					   const bool split)
{
	assert(impVec.empty());
	assert(!state.empty());
	assert(state.concrete_size().upper() == 0ul);
	// Build vector the size of concrete state space filled with zeros ...
	ImportanceVec(state.concrete_size().lower()).swap(impVec);
	// ... and label according to the property
	if (split) {
		auto clausesPair = clauses.project(state);
		label_local_states(state, impVec, property.type,
						   clausesPair.first, clausesPair.second);
	} else
		label_global_states(state, impVec, property);
	return static_cast<ImportanceValue>(0u);
}

} // namespace  // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

const std::array<std::string, ImportanceFunctionConcrete::NUM_POST_PROCESSINGS>&
ImportanceFunctionConcrete::post_processings() noexcept
{
	static const std::array< std::string, NUM_POST_PROCESSINGS > postProcessings =
	{{
		// See pp_shift()
		"shift",
		// See pp_exponentiate()
		"exp"
		/// @note Changes here must be reflected in post_process()
	}};
	return postProcessings;
}


PostProcessing
ImportanceFunctionConcrete::interpret_post_processing(
		const std::pair<std::string, float>& pp) noexcept
{
	if (pp.first.empty())
		return PostProcessing(PostProcessing::NONE, "", 0.0f);
	else if ("shift" == pp.first)
		return PostProcessing(PostProcessing::SHIFT, "shift", pp.second);
	else if ("exp" == pp.first)
		return PostProcessing(PostProcessing::EXP, "exp", pp.second);
	else
		return PostProcessing(PostProcessing::INVALID, "INVALID", 0.0f);
}


ImportanceFunctionConcrete::ImportanceFunctionConcrete(
	const std::string& name,
	const State<STATE_INTERNAL_TYPE>& globalState) :
		ImportanceFunction(name),
		modulesConcreteImportance(1u),
		globalStateCopy(globalState),
		userDefinedData(false)
{ /* Not much to do around here */ }


ImportanceFunctionConcrete::~ImportanceFunctionConcrete()
{
	clear();
}


PostProcessing
ImportanceFunctionConcrete::post_processing() const noexcept
{
	return postProc_;
}


bool ImportanceFunctionConcrete::assess_importance(
    const Module& module,
    const Property& property,
    const std::string& strategy,
    const unsigned& index,
    const PropertyProjection& clauses,
    const Indices &relevant)
{
	if (modulesConcreteImportance.size() <= index)
		modulesConcreteImportance.resize(index+1);
	else if (!modulesConcreteImportance[index].empty())
		throw_FigException("importance info already exists at position "
						  + std::to_string(index));
	bool moduleIsRelevant(false);
	const bool split(name().find("split") != std::string::npos);

	// Compute importance according to the chosen strategy
	if ("flat" == strategy) {
		check_mem_limits(module.concrete_state_size(), module.id());
		maxValue_ = assess_importance_flat(module.initial_state(),
										   modulesConcreteImportance[index],
										   property,
										   clauses,
										   split);
        // Invariant of flat importance function:
		minValue_ = maxValue_;
		initialValue_ = maxValue_;
		minRareValue_ = maxValue_;

	} else if ("auto" == strategy) {
		maxValue_ = assess_importance_auto(module,
										   modulesConcreteImportance[index],
										   property,
										   clauses,
		                                   split,
		                                   relevant);
		if (static_cast<ImportanceValue>(0u) < maxValue_ ) {
			moduleIsRelevant = true;
			// For auto importance functions the initial state has always
			// the lowest importance, and all rare states have the highest:
			minValue_ = UNMASK(
					modulesConcreteImportance[index][module.initial_concrete_state()]);
			initialValue_ = minValue_;
			minRareValue_ = maxValue_;  // should we check?
		} else {
			// This module is irrelevant for importance computation
			minValue_ = maxValue_;
			initialValue_ = maxValue_;
			minRareValue_ = maxValue_;
		}

	} else if ("adhoc" == strategy) {
        throw_FigException("importance strategy \"adhoc\" requires a user "
                           "defined formula expression; this routine should "
                           "not have been invoked for such strategy.");
	} else {
		throw_FigException("unrecognized importance assessment strategy \""
						   + strategy + "\". See available options with "
						   "ModelSuite::available_importance_strategies()");
	}

	return moduleIsRelevant;
}


void
ImportanceFunctionConcrete::post_process(const PostProcessing& postProc,
                                         std::vector<ExtremeValues>& extrVals)
{
	if (!has_importance_info())
#ifndef NDEBUG
		throw_FigException("importance function \"" + name() + "\" "
						   "doesn't yet have importance information");
#else
		return;
#endif
	postProc_ = postProc;
	if (postProc.type != PostProcessing::NONE)
		figTechLog << "Applying post-process \"" << postProc.name << "\" "
				   << "to the importance values computed.\n";
	switch (postProc.type)
	{
	case (PostProcessing::NONE):
		return;  // meh, called for nothing
		break;
	case (PostProcessing::SHIFT):
		pp_shift(extrVals, std::round(postProc.value));
		break;
	case (PostProcessing::EXP):
		pp_exponentiate(extrVals, postProc.value);
		break;
	default:
		throw_FigException("invalid post-processing specified (\""
						  + postProc.name + "\")");
		break;
	}
}


void
ImportanceFunctionConcrete::pp_shift(std::vector<ExtremeValues>& extrVals,
									 const int& offset)
{
	assert(has_importance_info());
	if (ready())
		figTechLog << "[WARNING] changing importance values after choosing thresholds;\n"
		           << "          you may need to choose them again!\n";

	// Update extreme values first
	std::string flow = (offset>=0) ? "over" : "under";
	auto shiftOK = (offset>=0)
	        ? ([] (const ImportanceValue& oldVal, const ImportanceValue& newVal)
	           { return oldVal <= newVal; })
	        : ([] (const ImportanceValue& oldVal, const ImportanceValue& newVal)
	           { return oldVal > newVal; });
	if (userDefinedData)
		goto main_loop_pp_shift;  // user took care, screw this
	for (ExtremeValues& ev: extrVals) {
		ImportanceValue& min  = std::get<0>(ev);
		ImportanceValue& max  = std::get<1>(ev);
		ImportanceValue& minR = std::get<2>(ev);
		// Extreme values are sensitive to {under,over}flows
		if (!shiftOK(min, min+offset))
			throw_FigException(flow+"flow while post-processing min importance value");
		else if (!shiftOK(max, max+offset))
			throw_FigException(flow+"flow while post-processing max importance value");
		min  += offset;
		max  += offset;
		minR += offset;
	}

main_loop_pp_shift:
	// Now shift importance values (disregard {under,over}flows here)
	for (ImportanceVec& vec: modulesConcreteImportance)
		for (ImportanceValue& val: vec)
			val = MASK(val) | (UNMASK(val)+offset);
}


void
ImportanceFunctionConcrete::pp_exponentiate(std::vector<ExtremeValues>& extrVals,
											const float b)
{
	using std::round;
	using std::pow;
	assert(has_importance_info());

	if (b <= 1.0f)
		throw_FigException("the \"exponentiation\" post-processing requires "
		                   "a base greater than 1.0");
	else if (ready())
		figTechLog << "[WARNING] changing importance values after choosing thresholds;\n"
		           << "          you may need to choose them again!\n";

	// Update extreme values first
	auto expOK = [&b] (const ImportanceValue& imp)
		{ auto tmp = static_cast<ImportanceValue>(round(pow(b,imp)));
		  return tmp > static_cast<ImportanceValue>(0) && !IS_SOME_EVENT(tmp); };
	if (userDefinedData)
		goto main_loop_pp_exp;  // user took care, screw this
	for (ExtremeValues& ev: extrVals) {
		ImportanceValue& min  = std::get<0>(ev);
		ImportanceValue& max  = std::get<1>(ev);
		ImportanceValue& minR = std::get<2>(ev);
		// Extreme values are the most sensitive to numerical problems
		if (!expOK(min))
			throw_FigException("error post-processing min importance value");
		else if (!expOK(max))
			throw_FigException("error post-processing max importance value");
		min  = static_cast<ImportanceValue>(round(pow(b, min)));
		max  = static_cast<ImportanceValue>(round(pow(b, max)));
		minR = static_cast<ImportanceValue>(round(pow(b, minR)));
	}

main_loop_pp_exp:
	// Now exponentiate all the importance values stored
	for (ImportanceVec& vec: modulesConcreteImportance)
		for (ImportanceValue& val: vec)
			val = MASK(val) |
				  static_cast<ImportanceValue>(round(pow(b,UNMASK(val))));
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

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
