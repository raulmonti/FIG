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
#include <iterator>   // std::begin(), std::end()
#include <algorithm>  // find_if_not()
// FIG
#include <ImportanceFunctionConcreteSplit.h>
#include <ThresholdsBuilder.h>
#include <string_utils.h>

// ADL
using std::begin;
using std::end;


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

std::vector< unsigned > ImportanceFunctionConcreteSplit::globalVarsIPos_;



// ImportanceFunctionConcreteSplit class member functions

ImportanceFunctionConcreteSplit::ImportanceFunctionConcreteSplit(
	const ModuleNetwork &model) :
		ImportanceFunctionConcrete("concrete_split",
								   model.global_state()),
		modules_(model.modules),
		numModules_(model.modules.size()),
		localValues_(numModules_),
		localStatesCopies_(numModules_),
		importance2threshold_()
{
	bool initialize(false);  // initialize (non-const) static class members?
	if (globalVarsIPos_.size() == 0ul) {
		initialize = true;
		globalVarsIPos_.resize(numModules_);
	}
	for (size_t i = 0ul ; i < numModules_ ; i++) {
		assert(modules_[i]->global_index() == static_cast<int>(i));
		localStatesCopies_[i] = modules_[i]->local_state();
		if (initialize)
			globalVarsIPos_[i] = static_cast<unsigned>(modules_[i]->first_var_gpos());
	}
}


ImportanceFunctionConcreteSplit::~ImportanceFunctionConcreteSplit()
{
	std::vector< ImportanceValue >().swap(localValues_);
	std::vector< State<STATE_INTERNAL_TYPE> >().swap(localStatesCopies_);
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
    Event e(EventType::RARE | EventType::STOP | EventType::REFERENCE);
    // Gather the local ImportanceValue of each module
	for (size_t i = 0ul ; i < numModules_ ; i++) {
		auto& localState = localStatesCopies_[i];
#ifndef NDEBUG
		localState.extract_from_state_instance(state, globalVarsIPos_[i], true);
#else
		localState.extract_from_state_instance(state, globalVarsIPos_[i], false);
#endif
        const auto& val = modulesConcreteImportance[i][localState.encode()];
        e &= MASK(val);
        localValues_[i] = UNMASK(val);
    }
    // Combine those values with the user-defined merge function
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
	for (size_t i = 0ul ; i < numModules_ ; i++) {
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
ImportanceFunctionConcreteSplit::print_out(std::ostream& out,
										   State<STATE_INTERNAL_TYPE>) const
{
    if (!has_importance_info()) {
        out << "\nImportance function \"" << name() << "\" doesn't yet have "
               "any importance information to print." << std::endl;
        return;
    }
    out << "\nPrinting importance function \"" << name() << "\" values.";
    out << "\nImportance assessment strategy: " << strategy();
    out << "\nImportance merging function: " << userFun_.expression();
    out << "\nLegend: ( concrete_state[*~^] , importance_value )";
    out << "\nwhere"
        << "\n      *  denotes a state is RARE,"
        << "\n      ~  denotes a state is STOP,"
        << "\n      ^  denotes a state is REFERENCE.";
    for (size_t i = 0ul ; i < numModules_ ; i++) {
		out << "\nValues for module \"" << modules_[i]->name << "\":";
        const ImportanceVec& impVec = modulesConcreteImportance[i];
        for (size_t i = 0ul ; i < impVec.size() ; i++) {
            out << " (" << i;
            out << (IS_RARE_EVENT     (impVec[i]) ? "*" : "");
            out << (IS_STOP_EVENT     (impVec[i]) ? "~" : "");
            out << (IS_REFERENCE_EVENT(impVec[i]) ? "^" : "");
            out << "," << UNMASK(impVec[i]) << ")";
        }
        out.flush();
    }
    if (ready()) {
        out << "\nImportanceValue to threshold level conversion:";
        for (size_t i = 0ul ; i < importance2threshold_.size() ; i++)
            out << " (" << i << ":" << importance2threshold_[i] << ")";
    }
    out << std::endl;
}


void
ImportanceFunctionConcreteSplit::set_merge_fun(std::string mergeFunExpr)
{
	static std::vector< std::string > modulesNames;
	static PositionsMap modulesMap;
	if (modulesNames.empty()) {
		modulesNames.resize(numModules_);
		modulesMap.reserve(numModules_);
		for (size_t i=0ul ; i < numModules_ ; i++) {
			const std::string& name = modules_[i]->name;
			modulesNames[i] = name;
			modulesMap[name] = i;
		}
	}
	if (mergeFunExpr.length() <= 3ul)  // given an operand => make it a function
		mergeFunExpr = compose_merge_function(modulesNames, mergeFunExpr);
	try {
		userFun_.set(mergeFunExpr, modulesNames, modulesMap);
	} catch (std::out_of_range& e) {
		throw_FigException(std::string("something went wrong while setting ")
						   .append(" the merge function \"").append(mergeFunExpr)
						   .append("\" for auto split importance assessment: ")
						   .append(e.what()));
	}
}


std::string
ImportanceFunctionConcreteSplit::compose_merge_function(
	const std::vector< std::string >& modulesNames,
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
	if (mergeOperand.length() == 1ul) {
		// Must be either '+' or '*'
		const std::string op(std::string(" ")
							 .append(trim(mergeOperand))
							 .append(" "));
		for (const auto& mName: modulesNames)
			mergeFun.append(mName).append(op);
		mergeFun.resize(mergeFun.size() - op.length());
	} else {
		// Must be either 'max' or 'min'
		assert(mergeOperand.length() == 3ul);
		const std::string op(trim(mergeOperand).append("("));
		for (const auto& mName: modulesNames)
			mergeFun.append(op).append(mName).append(", ");
		// mergeFun == "min(name1, min(name2, ..., min(nameN, "
		mergeFun.resize(mergeFun.size() - op.length()
										- modulesNames.back().length()
										- std::string(", ").length());
		mergeFun.append(modulesNames.back());
		mergeFun.append(modulesNames.size()-1ul, ')');
	}
	return mergeFun;
}


void
ImportanceFunctionConcreteSplit::assess_importance(const Property& prop,
												   const std::string& strategy)
{
	if ("flat" == strategy)
		set_merge_fun("+");
	if (userFun_.expression().length() < numModules_)
		throw_FigException(std::string("can't assess importance in function \"")
						   .append(name()).append("\" since current merging ")
						   .append(" function is invalid (\"")
						   .append(userFun_.expression()).append("\")"));
	if (hasImportanceInfo_)
		ImportanceFunctionConcrete::clear();
	modulesConcreteImportance.resize(numModules_);

	// Assess each module importance individually from the rest
	for (size_t index = 0ul ; index < numModules_ ; index++) {
		auto& localState = localStatesCopies_[index];
		localState.extract_valuation_from(globalState);  // set at local initial valuation
		ImportanceFunctionConcrete::assess_importance(localState,
													  modules_[index]->transitions(),
													  prop,
													  strategy,
													  index);
		assert(minImportance_ <= minRareImportance_);
		assert(minRareImportance_ <= maxImportance_);
	}
	hasImportanceInfo_ = true;
	strategy_ = strategy;

	// Find extreme importance values for current assessment
	if ("flat" == strategy) {
		const ImportanceValue importance =
				importance_of(globalState.to_state_instance());
		minImportance_ = importance;
		maxImportance_ = importance;
		minRareImportance_ = importance;
	} else {
        find_extreme_values(globalState, prop);  // *very* CPU intensive
	}
	assert(minImportance_ <= minRareImportance_);
	assert(minRareImportance_ <= maxImportance_);
}


void
ImportanceFunctionConcreteSplit::assess_importance(
	const Property&,
	const std::string&,
	const std::vector<std::string>&)
{
	throw_FigException("TODO: ad hoc assessment and split concrete storage");
	/// @todo TODO: implement concrete ifun with ad hoc importance assessment
}


void
ImportanceFunctionConcreteSplit::build_thresholds(
	ThresholdsBuilder& tb,
	const unsigned& splitsPerThreshold)
{
	if (!has_importance_info())
		throw_FigException(std::string("importance function \"").append(name())
						   .append("\" doesn't yet have importance information"));

	std::vector< ImportanceValue >().swap(importance2threshold_);
	importance2threshold_ = tb.build_thresholds(splitsPerThreshold, *this);
	assert(!importance2threshold_.empty());
	assert(importance2threshold_[0] == static_cast<ImportanceValue>(0u));
	assert(importance2threshold_[0] <= importance2threshold_.back());

	numThresholds_ = importance2threshold_.back();
	thresholdsTechnique_ = tb.name;
	readyForSims_ = true;
}

} // namespace fig
