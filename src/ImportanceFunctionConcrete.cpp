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
#include <stack>
#include <vector>
#include <forward_list>
// FIG
#include <ImportanceFunctionConcrete.h>
#include <Property.h>
#include <FigException.h>


namespace
{

using fig::State;
using fig::Property;
using fig::ImportanceValue;
using ImportanceVec = fig::ImportanceFunctionConcrete::ImportanceVec;

/**
 * @brief Assign null importance for all states in concrete vector
 * @param concreteSize Concrete size of the state space
 * @param ivec         Vector where the importance will be stored
 */
void
assess_importance_flat(const size_t& concreteSize, ImportanceVec& ivec)
{
	ImportanceVec(concreteSize, static_cast<ImportanceValue>(0)).swap(ivec);
}

/**
 * @brief Assign automatic importance for reachable states in concrete vector
 * @param state     Symbolic state of this module/model
 * @param ivec      Vector where the importance will be stored
 * @param property  Property identifying the rare states
 */
void
assess_importance_auto(const State<fig::STATE_INTERNAL_TYPE>& state,
					   ImportanceVec& ivec,
					   const Property& property)
{
	/// @todo TODO implement algorithm from sheet,
	///       the one with 'state[]' and 'redges[]' arrays.
	///       "ivec[]" would be the 'state[]'

	typedef unsigned STATE;
	const ImportanceValue NOT_VISITED(-1);
	ImportanceVec& visitedStates = ivec;

	// Step 1: store reversed edges in "rEdges[]" with DFS
	ImportanceVec(state.concrete_size(),
				  static_cast<ImportanceValue>(NOT_VISITED)).swap(visitedStates);
	std::vector< std::forward_list< STATE > > rEdges(state.concrete_size());

	/// @todo TODO: how do we build the concrete states adjacency?
	///             Do we traverse the model's transitions?
	///             HELP!

	// Step 2: clean "visitedStates[]" and use it to mark the rare states
	State s(state);
	for (size_t i = 0 ; i < s.concrete_size() ; i++)
		ivec[i] = property.is_rare(s.decode_state(i)) ? 0 : NOT_VISITED;

	// Step 3: store distances in "ivec[]" with reverse BFS using "rEdges[]"
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
ImportanceFunctionConcrete::assess_importance(const State<STATE_INTERNAL_TYPE>& s,
											  const Property& property,
											  const std::string& strategy,
											  const unsigned& index)
{
	if (modulesConcreteImportance.size() <= index)
		modulesConcreteImportance.resize(index);
	else if (modulesConcreteImportance[index].size() > 0u)
		throw_FigException(std::string("importance info already exists at ")
						   .append(" position").append(std::to_string(index)));

	// Compute importance following strategy
	if ("" == strategy || "flat" == strategy) {
		assess_importance_flat(s.concrete_size(),
							   modulesConcreteImportance[index]);

	} else if ("auto" == strategy) {
		assess_importance_auto(s.concrete_size(),
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
