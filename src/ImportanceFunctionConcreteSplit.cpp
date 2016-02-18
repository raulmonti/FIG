//==============================================================================
//
//  ImportanceFunctionConcreteSplit.cpp
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
#include <iterator>  // std::begin(), std::end()
#include <algorithm>
// FIG
#include <ImportanceFunctionConcreteSplit.h>

// ADL
using std::begin;
using std::end;


namespace
{

/// @brief Remove whitespace from begin and end of string
/// @note Taken from <a href="http://stackoverflow.com/a/17976541">
///       this answer in SO by David G</a>.
inline std::string trim(const std::string &s)
{
	auto wsfront = std::find_if_not(s.begin(),
									s.end(),
									[](int c){return std::isspace(c);});
	return std::string(wsfront,
					   std::find_if_not(s.rbegin(),
										std::string::const_reverse_iterator(wsfront),
										[](int c){return std::isspace(c);}).base());
}

} // namespace



namespace fig
{

// Static variables initialization

const std::array< std::string, 4 >
ImportanceFunctionConcreteSplit::mergeOperands =
{{
	"+", "*", "max", "min"
	// If options here are modified revise immediately
	// the "compose_merge_function()" class member function
}};

std::vector< std::string > ImportanceFunctionConcreteSplit::modulesNames_;

PositionsMap ImportanceFunctionConcreteSplit::modulesMap_;



// ImportanceFunctionConcreteSplit class member functions

ImportanceFunctionConcreteSplit::ImportanceFunctionConcreteSplit(
	const ModuleNetwork &model) :
		ImportanceFunctionConcrete("concrete_split"),
		localValues_(model.modules.size()),
		localStatesCopies_(model.modules.size()),
		globalVarsIPos_(model.modules.size()),
		importance2threshold_()
{
	bool initialize(false);  // initialize (non-const) static class members?
	if (modulesNames_.size() == 0ul) {
		initialize = true;
		modulesNames_.resize(model.modules.size());
		modulesMap_.reserve(model.modules.size());
	}
	for (size_t i = 0ul ; i < localStatesCopies_.size() ; i++) {
		assert(model.modules[i]->global_index() == static_cast<int>(i));
		localStatesCopies_[i] = model.modules[i]->local_state();
		globalVarsIPos_[i] = 0ul == i ? 0u
									  : globalVarsIPos_[i-1]
										+ model.modules[i-1]->state_size();
		if (initialize) {
			modulesNames_[i] = model.modules[i]->name;
			modulesMap_[modulesNames_[i]] = i;
		}
	}
	assert(modulesNames_.size() == model.modules.size());
	assert(modulesMap_.size() == model.modules.size());
}


ImportanceFunctionConcreteSplit::~ImportanceFunctionConcreteSplit()
{
	std::vector< ImportanceValue >().swap(localValues_);
	std::vector< State<STATE_INTERNAL_TYPE> >().swap(localStatesCopies_);
	std::vector< unsigned short >().swap(globalVarsIPos_);
	std::vector< ImportanceValue >().swap(importance2threshold_);
	ImportanceFunctionConcrete::clear();
}


ImportanceValue
ImportanceFunctionConcreteSplit::info_of(const StateInstance& state) const
{
#ifndef NDEBUG
	if (!has_importance_info())
		throw_FigException(std::string("importance function \"")
						   .append(name()).append("\" doesn't ")
						   .append("hold importance information."));
#endif
	Event e(EventType::NONE);
	for (size_t i = 0ul ; i < localStatesCopies_.size() ; i++) {
		auto& localState = localStatesCopies_[i];
#ifndef NDEBUG
		localState.extract_from_state_instance(state, globalVarsIPos_[i], true);
#else
		localState.extract_from_state_instance(state, globalVarsIPos_[i], false);
#endif
		localValues_[i] = modulesConcreteImportance[i][localState.encode()];
		e |= MASK(localValues_[i]);
	}
	return e | userFun_(localValues_);
}



ImportanceValue
ImportanceFunctionConcreteSplit::importance_of(const StateInstance& state) const
{
#ifndef NDEBUG
	if (!has_importance_info())
		throw_FigException(std::string("importance function \"")
						   .append(name()).append("\" doesn't ")
						   .append("hold importance information."));
#endif
	for (size_t i = 0ul ; i < localStatesCopies_.size() ; i++) {
		auto& localState = localStatesCopies_[i];
#ifndef NDEBUG
		localState.extract_from_state_instance(state, globalVarsIPos_[i], true);
#else
		localState.extract_from_state_instance(state, globalVarsIPos_[i], false);
#endif
		localValues_[i] = modulesConcreteImportance[i][localState.encode()];
	}
	return userFun_(localValues_);
}


void
ImportanceFunctionConcreteSplit::set_merge_fun(std::string mergeFunExpr)
{
	if (mergeFunExpr.size() <= 3)  // given an operand, make it a function
		mergeFunExpr = compose_merge_function(mergeFunExpr);
	try {
		userFun_.set(mergeFunExpr, modulesNames_, modulesMap_);
	} catch (std::out_of_range& e) {
		throw_FigException(std::string("something went wrong while setting ")
						   .append(" the merge function \"").append(mergeFunExpr)
						   .append("\" for auto split importance assessment: ")
						   .append(e.what()));
	}

	/// @todo TODO: finish implementation
}


std::string
ImportanceFunctionConcreteSplit::compose_merge_function(
	const std::string& mergeOperand) const
{
	if (std::find(begin(mergeOperands), end(mergeOperands), trim(mergeOperand))
			== end(mergeOperands))
		throw_FigException(std::string("invalid merge operand passed (\"")
						   .append(mergeOperand).append("\") to combine the ")
						   .append("importance values in split importance ")
						   .append("function. See valid options in Importance")
						   .append("FunctionConcreteSplit::mergeOperands"));
	std::string mergeFun;
	if (mergeOperand.size() == 1ul) {
		// Must be either '+' or '*'
		const std::string op(std::string(" ")
							 .append(trim(mergeOperand))
							 .append(" "));
		for (const auto& mName: modulesNames_)
			mergeFun.append(mName).append(op);
		mergeFun.resize(mergeFun.size()-2ul);
	} else {
		// Must be either 'max' or 'min'
		assert(mergeOperand.size() == 3ul);
		const std::string op(trim(mergeOperand).append("("));
		for (const auto& mName: modulesNames_)
			mergeFun.append(op).append(mName).append(", ");
		// mergeFun == "min(name1, min(name2, ..., min(nameN, "
		mergeFun.resize(mergeFun.size() - std::string("min(, ").length()
										- modulesNames_.back().length());
		mergeFun.append(modulesNames_.back());
		for (size_t i = 0ul ; i < modulesNames_.size()-1ul ; i++)
			mergeFun.append(")");
	}
	return mergeFun;
}

} // namespace fig
