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
#include <tuple>
// FIG
#include <ImportanceFunctionConcreteSplit.h>
#include <ThresholdsBuilder.h>
#include <ThresholdsBuilderAdaptive.h>
#include <ModuleInstance.h>
#include <ModuleNetwork.h>
#include <PropertyProjection.h>
#include <string_utils.h>

// ADL
using std::begin;
using std::end;


namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

using fig::ImportanceVec;
using fig::ImportanceValue;
using Formula = fig::ImportanceFunction::Formula;
using ExtremeValues = fig::ImportanceFunctionConcrete::ExtremeValues;
using CompositionType = fig::ImportanceFunctionConcreteSplit::CompositionType;

/// ImportanceValue extremes per Module
typedef std::vector< ExtremeValues > ExtremeValuesVec;


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
bool advance(const ExtremeValuesVec& moduleValues, ImportanceVec& values)
{
	unsigned pos(0ul);
	// Find next value (according to 'moduleValues' order)
	// which can be incremented
	for (const auto& e: moduleValues) {
		if (values[pos] < std::get<1>(e))
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
			values[i++] = std::get<0>(e);
	        // XXX If you really want the "i++" out of the vector access,
	        //	 *please* put braces for the scopes of the "for" and the "if"
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
 * @param f  Algebraic formula used to compose the modules importance values
 * @param moduleValues Extreme values of every module, i.e. (min, max, minRare)
 *
 * @return (min,max,minRare) evaluations of 'f' for all possible combination
 *         of values from the [min,max] ranges provided per module
 *
 * @note <b>Complexity:</b> too complicated to explain in docstring. Much less
 *                          than <i>O(globalState.concrete_size())</i> anyway.
 */
ExtremeValues
find_extreme_values(const Formula& f, const ExtremeValuesVec& moduleValues)
{
	ImportanceValue min(std::numeric_limits<ImportanceValue>::max());
	ImportanceValue max(std::numeric_limits<ImportanceValue>::min());
	ImportanceVec values(moduleValues.size());

	// initialise importance values to the minimal value of each Module
	// Notice the order in 'values' will follow that of 'moduleValues'
	size_t i(0ul);
	for (const auto& e: moduleValues)
		values[i++] = std::get<0>(e);

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
 *        for the specified composition strategy and algebraic formula.
 *
 * @param f Algebraic formula used to compose the modules importance values
 * @param moduleValues Extreme values (i.e. (min, max, minRare)) of every module
 * @param compStrategy CompositionType used to compose the modules importance values
 *
 * @return (min,max,minRare) evaluations of 'f' for all possible combination
 *         of importance values of the modules
 *
 * @throw FigException for invalid CompositionType
 * @throw FigException if overflow detected
 */
ExtremeValues
find_extreme_values(const Formula& f,
                    const ExtremeValuesVec& moduleValues,
					const CompositionType& compStrategy)
{
	ImportanceValue min, max, minR;
	static auto check_for_overflow =
			[] (const ImportanceValue& min, const ImportanceValue& max)
			{ if (min > max) throw_FigException("overflow detected while "
							 "post-processing extreme importance values"); };

	switch (compStrategy) {

	case CompositionType::SUMMATION:
		min = static_cast<ImportanceValue>(0u);
		max = static_cast<ImportanceValue>(0u);
		minR = static_cast<ImportanceValue>(0u);
		for (const auto& e: moduleValues) {
			min += std::get<0>(e);
			max += std::get<1>(e);
			minR += std::get<2>(e);
			check_for_overflow(min,max);
		}
		break;

	case CompositionType::PRODUCT:
		min = static_cast<ImportanceValue>(1u);
		max = static_cast<ImportanceValue>(1u);
		minR = static_cast<ImportanceValue>(1u);
		for (const auto& e: moduleValues) {
			min *= std::get<0>(e);
			max *= std::get<1>(e);
			minR *= std::get<2>(e);
			check_for_overflow(min,max);
		}
		break;

	case CompositionType::MAX:
		min = std::numeric_limits<ImportanceValue>::min();
		max = std::numeric_limits<ImportanceValue>::min();
		minR = std::numeric_limits<ImportanceValue>::min();
		for (const auto& e: moduleValues) {
			min = std::max(min, std::get<0>(e));
			max = std::max(max, std::get<1>(e));
			minR = std::max(minR, std::get<2>(e));
		}
		break;

	case CompositionType::MIN:
		min = std::numeric_limits<ImportanceValue>::max();
		max = std::numeric_limits<ImportanceValue>::max();
		minR = std::numeric_limits<ImportanceValue>::max();
		for (const auto& e: moduleValues) {
			min = std::min(min, std::get<0>(e));
			max = std::min(max, std::get<1>(e));
			minR = std::min(minR, std::get<2>(e));
		}
		break;

	case CompositionType::AD_HOC:
        return find_extreme_values(f, moduleValues);

	default:
		throw_FigException("unrecognized composition function strategy: "
						   + std::to_string(compStrategy));
	}

	return std::make_tuple(min, max, minR);
}

/// Nasty hack to avoid importing the full ModelSuite.h header
fig::StateInstance systemInitialValuation;

} // namespace  // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

// Static variables initialization

const std::array< std::string, 4 >
ImportanceFunctionConcreteSplit::compositionOperands =
{{
	"+", "*", "max", "min"
	// If options here are modified revise immediately
	// the "compose_comp_function()" class member function
}};

// std::vector< unsigned > ImportanceFunctionConcreteSplit::globalVarsIPos;



// ImportanceFunctionConcreteSplit class member functions

ImportanceFunctionConcreteSplit::ImportanceFunctionConcreteSplit(
	const ModuleNetwork &model) :
		ImportanceFunctionConcrete("concrete_split", model.global_state()),
		modules_(model.modules),
		numModules_(model.modules.size()),
		isRelevant_(numModules_, false),
        globalVarsIPos(numModules_),
		localValues_(numModules_),
		localStatesCopies_(numModules_),
		compositionStrategy_(CompositionType::INVALID),
		userMinValue_(0u),
		userMaxValue_(0u),
		neutralElement_(0u),
		concreteSimulation_(true)
{
	model.initial_state().to_state_instance().swap(systemInitialValuation);
	assert(0ul < numModules_);
	assert(globalVarsIPos.size() == numModules_);
	assert(!systemInitialValuation.empty());
	for (size_t i = 0ul ; i < numModules_ ; i++) {
		assert(modules_[i]->global_index() == static_cast<int>(i));
		localStatesCopies_[i] = modules_[i]->local_state();
		globalVarsIPos[i] = static_cast<unsigned>(modules_[i]->first_var_gpos());
	}
}


ImportanceFunctionConcreteSplit::~ImportanceFunctionConcreteSplit()
{
    ImportanceVec().swap(localValues_);
    std::vector< State< STATE_INTERNAL_TYPE > >().swap(localStatesCopies_);
	ThresholdsVec().swap(importance2threshold_);
	ImportanceFunctionConcrete::clear();
}


ImportanceValue
ImportanceFunctionConcreteSplit::info_of(const StateInstance& state) const
{
#ifndef NDEBUG
	if (!has_importance_info())
		throw_FigException("importance function \"" + name() + "\" "
		                   "doesn't hold importance information");
	assert(isRelevant_.size() == numModules_);
	assert(localValues_.size() == numModules_);
	assert(globalVarsIPos.size() == numModules_);
	assert(localStatesCopies_.size() == numModules_);
	assert(modulesConcreteImportance.size() == numModules_);
#endif
	Event e(EventType::NONE);
    // Gather the local ImportanceValue of each module
	for (size_t i = 0ul ; i < numModules_ ; i++) {
		auto& localState = localStatesCopies_[i];
#ifndef NDEBUG
		localState.extract_from_state_instance(state, globalVarsIPos.at(i), true);
#else
		localState.extract_from_state_instance(state, globalVarsIPos[i], false);
#endif
		const auto& val = modulesConcreteImportance[i][localState.encode()];
		e |= MASK(val);  // events are marked per-module but affect the global model
		localValues_[i] = isRelevant_[i] ? UNMASK(val) : neutralElement_;
    }
	// Combine those values with the user-defined composition function
	return e | (ready() ? level_of(userFun_(localValues_))
						: userFun_(localValues_));
}



ImportanceValue
ImportanceFunctionConcreteSplit::importance_of(const StateInstance& state) const
{
#ifndef NDEBUG
	if (!has_importance_info())
		throw_FigException("importance function \"" + name() + "\" "
						   "doesn't hold importance information");
	assert(isRelevant_.size() == numModules_);
	assert(localValues_.size() == numModules_);
	assert(globalVarsIPos.size() == numModules_);
	assert(localStatesCopies_.size() == numModules_);
	assert(modulesConcreteImportance.size() == numModules_);
#endif
	for (size_t i = 0ul ; i < numModules_ ; i++) {
		if (!isRelevant_[i]) {
			localValues_[i] = neutralElement_;
		} else {
			auto& localState = localStatesCopies_[i];
#ifndef NDEBUG
			localState.extract_from_state_instance(state, globalVarsIPos.at(i), true);
#else
			localState.extract_from_state_instance(state, globalVarsIPos[i], false);
#endif
			localValues_[i] = UNMASK(modulesConcreteImportance[i][localState.encode()]);
		}
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
		if (impVec.empty())
			out << " <nodata>";
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
			out << " (" << i << ":" << importance2threshold_[i].first << ")";
    }
    out << std::endl;
}


void
ImportanceFunctionConcreteSplit::set_composition_fun(
	std::string compFunExpr,
	const ImportanceValue& nullVal,
	const ImportanceValue& minVal,
	const ImportanceValue& maxVal)
{
	std::vector< std::string > modulesNames(numModules_);
	PositionsMap modulesMap;
	modulesMap.reserve(numModules_);
	for (size_t i=0ul ; i < numModules_ ; i++) {
		const std::string& name = modules_[i]->name;
		modulesNames[i] = name;
		modulesMap[name] = i;
	}
	// Clean unescaped quotation marks
	delete_substring(compFunExpr, "\"");
	delete_substring(compFunExpr, "'");
	if (compFunExpr.length() <= 3ul) {
		// An operand was specified => make it a function
		compFunExpr = compose_comp_function(modulesNames, compFunExpr);
	} else {
		// A fully defined function was specified
		compositionStrategy_ = CompositionType::AD_HOC;
		neutralElement_ = nullVal;
	}
	assert(CompositionType::NUM_TYPES > compositionStrategy_);
	try {
		userFun_.set(compFunExpr, modulesNames, modulesMap);
	} catch (std::out_of_range& e) {
		throw_FigException("something went wrong while setting the composition "
						   "function \"" + compFunExpr + "\" for auto split "
						   "importance assessment: " + e.what());
	}
	if (minVal < maxVal) {
		// Set the user defined extreme values for the composition function
		userMinValue_ = minVal;
		userMaxValue_ = maxVal;
		userDefinedData = true;
	}
}


std::string
ImportanceFunctionConcreteSplit::compose_comp_function(
	const std::vector< std::string >& modulesNames,
	const std::string& compOperand)
{
	if (std::find(begin(compositionOperands), end(compositionOperands), trim(compOperand))
			== end(compositionOperands))
		throw_FigException("invalid composition operand passed (\"" + compOperand
						   + "\") to combine the importance values in split "
						   "importance function. See valid options in "
						   "ImportanceFunctionConcreteSplit::compositionOperands");
	std::string compFun;
	if (compOperand.length() == 1ul) {
		// Must be either '+' or '*'
		if (compOperand == "+") {
			compositionStrategy_ = CompositionType::SUMMATION;
			neutralElement_ = static_cast<ImportanceValue>(0u);
		} else if (compOperand == "*") {
			compositionStrategy_ = CompositionType::PRODUCT;
			neutralElement_ = static_cast<ImportanceValue>(1u);
		} else
			throw_FigException("invalid composition operand passed (\""
							   + compOperand + "\")");
		const std::string op(std::string(" ")
							 .append(trim(compOperand))
							 .append(" "));
		for (const auto& mName: modulesNames)
			compFun.append(mName).append(op);
		compFun.resize(compFun.size() - op.length());
	} else {
		// Must be either 'max' or 'min'
		if (compOperand == "max") {
			compositionStrategy_ = CompositionType::MAX;
			neutralElement_ = UNMASK(std::numeric_limits<ImportanceValue>::min());
		} else if (compOperand == "min") {
			compositionStrategy_ = CompositionType::MIN;
			neutralElement_ = UNMASK(std::numeric_limits<ImportanceValue>::max());
		} else
			throw_FigException("invalid composition operand passed (\""
							   + compOperand + "\")");
		const std::string op(trim(compOperand).append("("));
		for (const auto& mName: modulesNames)
			compFun.append(op).append(mName).append(", ");
		// compFun == "min(name1, min(name2, ..., min(nameN, "
		compFun.resize(compFun.size() - op.length()
									  - modulesNames.back().length()
									  - std::string(", ").length());
		compFun.append(modulesNames.back());
		compFun.append(modulesNames.size()-1ul, ')');
	}
	return compFun;
}


void
ImportanceFunctionConcreteSplit::assess_importance(const Property& prop,
												   const std::string& strategy,
												   const PostProcessing& postProc)
{
	if ("flat" == strategy)
		set_composition_fun("+");
	if (hasImportanceInfo_)
		ImportanceFunctionConcrete::clear();
	modulesConcreteImportance.resize(numModules_);

	// Assess each module importance individually from the rest
	concreteSimulation_ = false;
	unsigned numRelevantModules(0u);
	propertyClauses.populate(prop);
	ExtremeValuesVec moduleValues;
	moduleValues.reserve(numModules_);
	for (size_t i = 0ul ; i < numModules_ ; i++) {
		const bool moduleIsRelevant =
		    ImportanceFunctionConcrete::assess_importance(*modules_[i],
														  prop,
														  strategy,
		                                                  i,
														  propertyClauses);
		assert(minValue_ <= initialValue_);
		assert(initialValue_ <= minRareValue_);
		assert(minRareValue_ <= maxValue_);
		moduleValues.emplace_back(minValue_, maxValue_, minRareValue_);
		isRelevant_[i] = moduleIsRelevant;
		numRelevantModules += moduleIsRelevant ? 1u : 0u;
	}
	hasImportanceInfo_ = true;
	strategy_ = strategy;

	// If the rare event depends on the state of more than one module,
	// global rarity can't be encoded split in vectors for later simulations
	concreteSimulation_ = numRelevantModules < 2u;
	// Apply post processing (shift, exponentiation, etc.)
	if ("flat" != strategy) {
		post_process(postProc, moduleValues);
		if (postProc.type == PostProcessing::EXP)
			neutralElement_ = static_cast<ImportanceValue>(1);
	}

	// Find extreme importance values for current assessment
	if ("flat" == strategy) {
		// Little to find out
		minValue_ = importance_of(systemInitialValuation);
		maxValue_ = minValue_;
		minRareValue_ = minValue_;
	} else if (userDefinedData) {
		// Trust blindly in the user-defined extreme values
		minValue_ = userMinValue_;
		maxValue_ = userMaxValue_;
		minRareValue_ = userMinValue_;  // play it safe
	} else if (globalStateCopy.concrete_size() > uint128::uint128_0 &&
			   globalStateCopy.concrete_size() < (1ul<<20ul)) {
		// A brute force, full-state-space scan is affordable
		find_extreme_values(globalStateCopy, prop);
	} else {
		// Concrete state space is too big, resort to smarter ways
		std::tie(minValue_, maxValue_, minRareValue_) =
				::find_extreme_values(userFun_, moduleValues, compositionStrategy_);

	}
	initialValue_ = importance_of(systemInitialValuation);

	assert(minValue_ <= initialValue_);
	assert(initialValue_ <= minRareValue_);
	assert(minRareValue_ <= maxValue_);
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
