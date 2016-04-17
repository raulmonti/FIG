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
#include <limits>     // std::numeric_limits<>
#include <iterator>   // std::begin(), std::end()
#include <algorithm>  // find_if_not()
#include <unordered_map>
#include <tuple>
// FIG
#include <ImportanceFunctionConcreteSplit.h>
#include <ThresholdsBuilder.h>
#include <ThresholdsBuilderAdaptive.h>
#include <ModuleInstance.h>
#include <ModuleNetwork.h>
#include <DNFclauses.h>
#include <string_utils.h>

/// @todo TODO erase debug include
#include <ModelSuite.h>

// ADL
using std::begin;
using std::end;


namespace
{

using fig::ImportanceValue;
using Formula = fig::ImportanceFunction::Formula;
using ImportanceVec = fig::ImportanceFunction::ImportanceVec;
using MergeType = fig::ImportanceFunctionConcreteSplit::MergeType;

/// ImportanceValue extremes: (minValue_, maxValue_, minRareValue_)
typedef std::tuple< ImportanceValue, ImportanceValue, ImportanceValue >
	ExtremeValues;
/// ImportanceValue extremes per Module
typedef std::unordered_map< std::string, ExtremeValues >
	ModulesExtremeValues;


/**
 * @brief Increment the next possible ImportanceValue
 *
 *        Following the order of the values defined in 'moduleValues',
 *        find the first one which isn't maximal and increment it.
 *        All bypassed values (whose value was maximal) are reset
 *        to their minimal expression. This represents a single increment
 *        in the "concrete value of 'values'".
 *
 * @return Whether a value could be incremented
 */
bool advance(const ModulesExtremeValues& moduleValues, ImportanceVec& values)
{
	unsigned pos(0ul);
	// Find next value (according to 'moduleValues' order)
	// which can be incremented
	for (const auto& e: moduleValues) {
		if (values[pos] < std::get<1>(e.second))
			break;
		pos++;
	}
	if (pos >= moduleValues.size())
		return false;
	// Increment it
	values[pos]++;
	// Reset previous values (according to 'moduleValues' order) to minimums
	unsigned i(0u);
	for (const auto& e: moduleValues)
		if (i < pos)
			values[i++] = std::get<0>(e.second);
	return true;
}


/**
 * @brief Find extreme values of given \ref fig::ImportanceFunction::Formula
 *        "algebraic formula"
 *
 *        All possible combination of values in the [min,max] ranges provided
 *        per module are tested. From the resulting evaluations of the Formula
 *        'f' the minimal value is returned as the first component of the tuple,
 *        the maximal value as the second component and the minRare value as
 *        the third component.
 *
 * @param f  Algebraic formula used to merge the modules importance values
 * @param moduleValues Extreme values (i.e. (min, max, minRare)) of every module
 *
 * @return (min,max,minRare) evaluations of 'f' for all possible combination
 *         of values from the [min,max] ranges provided per module
 *
 * @note <b>Complexity:</b> too complicated to explain in docstring. Much less
 *                          than <i>O(globalState.concrete_size())</i> anyway.
 */
ExtremeValues
find_extreme_values(const Formula& f, const ModulesExtremeValues& moduleValues)
{
	ImportanceValue min(std::numeric_limits<ImportanceValue>::max());
	ImportanceValue max(std::numeric_limits<ImportanceValue>::min());
	ImportanceVec values(moduleValues.size());

	// Initialize importance values to the minimal value of each Module
	// Notice the order in 'values' will follow that of 'moduleValues'
	size_t i(0ul);
	for (const auto& e: moduleValues)
		values[i++] = std::get<0>(e.second);

	// Test all values combinations for the relevant variables
	do {
		ImportanceValue imp = f(values);
		min = std::min(min, imp);
		max = std::max(max, imp);
	} while (advance(moduleValues, values));

	// Play it safe and assume minRareValue_ == minValue_
	return std::make_tuple(min, max, min);

	/// @todo TODO The general solution is to use ILP on userFun_
	///            That'd also compute the real minRareValue_ (and fast!)
	///            Use <a href="http://dlib.net/">dlib</a> maybe?
}


/**
 * @brief Find extreme importance values (i.e. min, max and minRare)
 *        of the composition of the values computed for each Module,
 *        for the specified merge strategy and algebraic formula.
 *
 * @param f Algebraic formula used to merge the modules importance values
 * @param moduleValues Extreme values (i.e. (min, max, minRare)) of every module
 * @param mergeStrategy MergeType used to merge the modules importance values
 *
 * @return (min,max,minRare) evaluations of 'f' for all possible combination
 *         of importance values of the modules
 *
 * @throw FigException for invalid MergeType
 */
ExtremeValues
find_extreme_values(const Formula& f,
					const ModulesExtremeValues& moduleValues,
					const MergeType& mergeStrategy)
{
	ImportanceValue min, max, minR;

	switch (mergeStrategy) {

	case MergeType::SUMMATION:
		min = static_cast<ImportanceValue>(0u);
		max = static_cast<ImportanceValue>(0u);
		minR = static_cast<ImportanceValue>(0u);
		for (const auto& e: moduleValues) {
			min += std::get<0>(e.second);
			max += std::get<1>(e.second);
			minR += std::get<2>(e.second);
		}
		break;

	case MergeType::PRODUCT:
		min = static_cast<ImportanceValue>(1u);
		max = static_cast<ImportanceValue>(1u);
		minR = static_cast<ImportanceValue>(1u);
		for (const auto& e: moduleValues) {
			min *= std::get<0>(e.second);
			max *= std::get<1>(e.second);
			minR *= std::get<2>(e.second);
		}
		break;

	case MergeType::MAX:
		min = std::numeric_limits<ImportanceValue>::min();
		max = std::numeric_limits<ImportanceValue>::min();
		minR = std::numeric_limits<ImportanceValue>::min();
		for (const auto& e: moduleValues) {
			min = std::max(min, std::get<0>(e.second));
			max = std::max(max, std::get<1>(e.second));
			minR = std::max(minR, std::get<2>(e.second));
		}
		break;

	case MergeType::MIN:
		min = std::numeric_limits<ImportanceValue>::max();
		max = std::numeric_limits<ImportanceValue>::max();
		minR = std::numeric_limits<ImportanceValue>::max();
		for (const auto& e: moduleValues) {
			min = std::min(min, std::get<0>(e.second));
			max = std::min(max, std::get<1>(e.second));
			minR = std::min(minR, std::get<2>(e.second));
		}
		break;

	case MergeType::AD_HOC:
		return find_extreme_values(f, moduleValues);

	default:
		throw_FigException("unrecognized merge function strategy: "
						   + std::to_string(mergeStrategy));
	}

	return std::make_tuple(min, max, minR);
}

/// Nasty hack to avoid importing the full ModelSuite.h header
fig::StateInstance systemInitialValuation;

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

std::vector< unsigned > ImportanceFunctionConcreteSplit::globalVarsIPos;



// ImportanceFunctionConcreteSplit class member functions

ImportanceFunctionConcreteSplit::ImportanceFunctionConcreteSplit(
	const ModuleNetwork &model) :
		ImportanceFunctionConcrete("concrete_split", model.global_state()),
		modules_(model.modules),
		numModules_(model.modules.size()),
		localValues_(numModules_),
		localStatesCopies_(numModules_),
		mergeStrategy_(MergeType::NONE)
{
	bool initialize(false);  // initialize (non-const) static class members?
	if (globalVarsIPos.size() == 0ul) {
		initialize = true;
		globalVarsIPos.resize(numModules_);
		systemInitialValuation = model.initial_state().to_state_instance();
	}
	for (size_t i = 0ul ; i < numModules_ ; i++) {
		assert(modules_[i]->global_index() == static_cast<int>(i));
		localStatesCopies_[i] = modules_[i]->local_state();
		if (initialize)
			globalVarsIPos[i] = static_cast<unsigned>(modules_[i]->first_var_gpos());
	}
}


ImportanceFunctionConcreteSplit::~ImportanceFunctionConcreteSplit()
{
    ImportanceVec().swap(localValues_);
    std::vector< State< STATE_INTERNAL_TYPE > >().swap(localStatesCopies_);
	std::vector< ImportanceValue >().swap(importance2threshold_);
	ImportanceFunctionConcrete::clear();
}


ImportanceValue
ImportanceFunctionConcreteSplit::info_of(const StateInstance& state) const
{
#ifndef NDEBUG
	if (!has_importance_info())
		throw_FigException("importance function \"" + name() + "\" "
						   "doesn't hold importance information");
#endif
	Event e(EventType::NONE);
    // Gather the local ImportanceValue of each module
	for (size_t i = 0ul ; i < numModules_ ; i++) {
		auto& localState = localStatesCopies_[i];
#ifndef NDEBUG
		localState.extract_from_state_instance(state, globalVarsIPos[i], true);
#else
		localState.extract_from_state_instance(state, globalVarsIPos[i], false);
#endif
        const auto& val = modulesConcreteImportance[i][localState.encode()];
		e |= MASK(val);  // events are marked per-module but affect the global model
		/// @todo TODO erase debug print
		if (IS_RARE_EVENT(val))
			std::cerr << "Rare: state " << localState.encode() << " in module " << i << std::endl;
		/////////////////////////////////
        localValues_[i] = UNMASK(val);
    }
    // Combine those values with the user-defined merge function
	return e | (ready() ? importance2threshold_[userFun_(localValues_)]
						: userFun_(localValues_));
}



ImportanceValue
ImportanceFunctionConcreteSplit::importance_of(const StateInstance& state) const
{
#ifndef NDEBUG
	if (!has_importance_info())
		throw_FigException("importance function \"" + name() + "\" "
						   "doesn't hold importance information");
#endif
	for (size_t i = 0ul ; i < numModules_ ; i++) {
		auto& localState = localStatesCopies_[i];
#ifndef NDEBUG
		localState.extract_from_state_instance(state, globalVarsIPos[i], true);
#else
		localState.extract_from_state_instance(state, globalVarsIPos[i], false);
#endif
		localValues_[i] = UNMASK(modulesConcreteImportance[i][localState.encode()]);
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
	delete_substring(mergeFunExpr, "\"");
	if (mergeFunExpr.length() <= 3ul)  // given an operand => make it a function
		mergeFunExpr = compose_merge_function(modulesNames, mergeFunExpr);
	else
		mergeStrategy_ = MergeType::AD_HOC;
	assert(MergeType::NONE != mergeStrategy_);
	try {
		userFun_.set(mergeFunExpr, modulesNames, modulesMap);
	} catch (std::out_of_range& e) {
		throw_FigException("something went wrong while setting the merge "
						   "function \"" + mergeFunExpr + "\" for auto split "
						   "importance assessment: " + e.what());
	}
}


std::string
ImportanceFunctionConcreteSplit::compose_merge_function(
	const std::vector< std::string >& modulesNames,
	const std::string& mergeOperand) const
{
	if (std::find(begin(mergeOperands), end(mergeOperands), trim(mergeOperand))
			== end(mergeOperands))
		throw_FigException("invalid merge operand passed (\"" + mergeOperand
						   + "\") to combine the importance values in split "
						   "importance function. See valid options in "
						   "ImportanceFunctionConcreteSplit::mergeOperands");
	std::string mergeFun;
	if (mergeOperand.length() == 1ul) {
		// Must be either '+' or '*'
		if (mergeOperand == "+")
			mergeStrategy_ = MergeType::SUMMATION;
		else if (mergeOperand == "*")
			mergeStrategy_ = MergeType::PRODUCT;
		else
			throw_FigException("invalid merge operand passed (\""
							   + mergeOperand + "\")");
		const std::string op(std::string(" ")
							 .append(trim(mergeOperand))
							 .append(" "));
		for (const auto& mName: modulesNames)
			mergeFun.append(mName).append(op);
		mergeFun.resize(mergeFun.size() - op.length());
	} else {
		// Must be either 'max' or 'min'
		if (mergeOperand == "max")
			mergeStrategy_ = MergeType::MAX;
		else if (mergeOperand == "min")
			mergeStrategy_ = MergeType::MIN;
		else
			throw_FigException("invalid merge operand passed (\""
							   + mergeOperand + "\")");
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
		throw_FigException("can't assess importance in function \"" + name()
						   + "\" since current merging function is invalid (\""
						   + userFun_.expression() + "\")");
	if (hasImportanceInfo_)
		ImportanceFunctionConcrete::clear();
	modulesConcreteImportance.resize(numModules_);

	/// @todo TODO erase debug print
//	std::cerr << "Initial rare states in the model: ";
//	State<STATE_INTERNAL_TYPE> globalState(ModelSuite::get_instance().modules_network()->global_state());
//	for (size_t i = 0ul ; i < globalState.concrete_size() ; i++) {
//		auto valuation = globalState.decode(i).to_state_instance();
//		if (prop.is_rare(valuation))
//			std::cerr << i << ", ";
//	}
//	std::cerr << "\b\b  \n";
	/////////////////////////////////

	// Assess each module importance individually from the rest
	const parser::DNFclauses dnfClauses(prop);
	ModulesExtremeValues moduleValues(numModules_);
	for (size_t index = 0ul ; index < numModules_ ; index++) {
		ImportanceFunctionConcrete::assess_importance(*modules_[index],
													  prop,
													  strategy,
													  index,
													  dnfClauses);
		/// @todo TODO erase debug print
//		std::cerr << "Final rare states for this module: ";
//		const auto& impVec(modulesConcreteImportance[index]);
//		for (size_t i = 0ul ; i < impVec.size() ; i++)
//			if (fig::IS_RARE_EVENT(impVec[i]))
//				std::cerr << i << " (" << fig::UNMASK(impVec[i]) << "), ";
//		std::cerr << "\b\b  \n\n";
		////////////////////////////////
		assert(minValue_ <= initialValue_);
		assert(initialValue_ <= minRareValue_);
		assert(minRareValue_ <= maxValue_);
		moduleValues[modules_[index]->name] =
				std::make_tuple(minValue_, maxValue_, minRareValue_);
	}
	hasImportanceInfo_ = true;
	strategy_ = strategy;

	/// @todo TODO erase debug print
//	std::cerr << "Resulting rare states in the model: ";
//	for (size_t i = 0ul ; i < globalState.concrete_size() ; i++) {
//		auto val = info_of(globalState.decode(i).to_state_instance());
//		if (IS_RARE_EVENT(val))
//			std::cerr << i << ", ";
//	}
//	std::cerr << "\b\b  \n";
	/////////////////////////////////

	// Find extreme importance values for current assessment
	initialValue_ = importance_of(systemInitialValuation);
	if ("flat" == strategy) {
		minValue_ = initialValue_;
		maxValue_ = initialValue_;
		minRareValue_ = initialValue_;
	} else if (globalStateCopy.concrete_size() < (1ul<<20ul)) {
		// We can afford a full-state-space scan
		find_extreme_values(globalStateCopy, prop);
	} else {
		// Concrete state space is too big, resort to faster ways
		std::tie(minValue_, maxValue_, minRareValue_) =
				::find_extreme_values(userFun_, moduleValues, mergeStrategy_);
	}
	assert(minValue_ <= initialValue_);
	assert(initialValue_ <= minRareValue_);
	assert(minRareValue_ <= maxValue_);
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

} // namespace fig
