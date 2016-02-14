//==============================================================================
//
//  ImportanceFunctionAlgebraic.cpp
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
#include <list>
#include <deque>
#include <vector>
#include <forward_list>
#include <unordered_set>
// FIG
#include <ImportanceFunctionAlgebraic.h>
#include <ThresholdsBuilder.h>
#include <Property.h>


namespace fig
{

ImportanceFunctionAlgebraic::ImportanceFunctionAlgebraic() :
	ImportanceFunction("algebraic"),
	importance2threshold_()
{ /* Not much to do around here */ }


ImportanceFunctionAlgebraic::~ImportanceFunctionAlgebraic()
{
	clear();
}


ImportanceValue
ImportanceFunctionAlgebraic::importance_of(const StateInstance& state) const
{
#ifndef NDEBUG
	if (!has_importance_info())
		throw_FigException(std::string("importance function \"")
						   .append(name()).append("\" doesn't ")
						   .append("hold importance information."));
#endif
	return adhocFun_(state);
}


ImportanceValue
ImportanceFunctionAlgebraic::level_of(const StateInstance& state) const
{
#ifndef NDEBUG
	if (!ready())
		throw_FigException(std::string("importance function \"")
						   .append(name()).append("\" isn't ")
						   .append("ready for simulations."));
#endif
	return importance2threshold_[adhocFun_(state)];
}


ImportanceValue
ImportanceFunctionAlgebraic::level_of(const ImportanceValue& val) const
{
#ifndef NDEBUG
	if (!ready())
		throw_FigException(std::string("importance function \"")
						   .append(name()).append("\" isn't ")
						   .append("ready for simulations."));
#endif
	assert(val >= min_importance());
	assert(val <= max_importance());
	return importance2threshold_[val];
}


template< template< typename... > class Container, typename... OtherArgs >
void
ImportanceFunctionAlgebraic::set_formula(
	const std::string& strategy,
	const std::string& formulaExprStr,
	const Container<std::string, OtherArgs...>& varnames,
	const State<STATE_INTERNAL_TYPE>& gState,
	const Property& property)
{
    if ("auto" == strategy)
        throw_FigException("importance strategy \"auto\" can only be be used "
                           "with concrete importance functions; this routine "
                           "should not have been invoked for such strategy.");
    try {
        adhocFun_.set(formulaExprStr, varnames, gState);
	} catch (std::out_of_range& e) {
		throw_FigException(std::string("something went wrong while setting ")
						   .append(" the function \"").append(formulaExprStr)
						   .append("\" for ad hoc importance assessment: ")
						   .append(e.what()));
	}

	// Find extreme importance values for this ad hoc function
	auto gStateCopy = gState;
	if (strategy.empty() || "flat" == strategy) {
		ImportanceValue importance = adhocFun_(gStateCopy.to_state_instance());
		minImportance_ = importance;
		maxImportance_ = importance;
		minRareImportance_ = importance;
	} else {
		ImportanceValue imin = std::numeric_limits<ImportanceValue>::max();
		ImportanceValue imax = std::numeric_limits<ImportanceValue>::min();
		ImportanceValue iminRare = std::numeric_limits<ImportanceValue>::max();
		#pragma omp parallel for reduction(min:imin) private(gStateCopy)
		for (size_t i = 0ul ; i < gState.concrete_size() ; i++) {
			const StateInstance symbState = gStateCopy.decode(i).to_state_instance();
			const ImportanceValue importance = adhocFun_(symbState);
			minImportance_ = importance < minImportance_ ? importance
														 : minImportance_;
			if (property.is_rare(symbState) && importance < minRareImportance_)
				minRareImportance_ = importance;
		}
		#pragma omp parallel for reduction(max:imax) private(gStateCopy)
		for (size_t i = 0ul ; i < gState.concrete_size() ; i++) {
			auto importance = adhocFun_(gStateCopy.decode(i).to_state_instance());
			maxImportance_ = importance > maxImportance_ ? importance
														 : maxImportance_;
		}
		minImportance_ = imin;
		maxImportance_ = imax;
		minRareImportance_ = iminRare;
	}

	assert(minImportance_ <= minRareImportance_);
	assert(minRareImportance_ <= maxImportance_);
	hasImportanceInfo_ = true;
	strategy_ = strategy;
}

// ImportanceFunctionAlgebraic::set_formula() can only be invoked
// with the following containers
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::set<std::string>&,
    const State<STATE_INTERNAL_TYPE>&, const Property&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::list<std::string>&,
    const State<STATE_INTERNAL_TYPE>&, const Property&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::deque<std::string>&,
    const State<STATE_INTERNAL_TYPE>&, const Property&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::vector<std::string>&,
    const State<STATE_INTERNAL_TYPE>&, const Property&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::forward_list<std::string>&,
    const State<STATE_INTERNAL_TYPE>&, const Property&);
template void ImportanceFunctionAlgebraic::set_formula(
    const std::string&, const std::string&, const std::unordered_set<std::string>&,
    const State<STATE_INTERNAL_TYPE>&, const Property&);


void
ImportanceFunctionAlgebraic::build_thresholds(
    ThresholdsBuilder& tb,
    const unsigned& splitsPerThreshold)
{
	if (!has_importance_info())
		throw_FigException(std::string("importance function \"").append(name())
						   .append("\" doesn't yet have importance information"));
	std::vector< ImportanceValue >().swap(importance2threshold_);
	importance2threshold_ = tb.build_thresholds(splitsPerThreshold, *this);
	assert(!importance2threshold_.empty());
	thresholdsTechnique_ = tb.name;
	readyForSims_ = true;
}


void
ImportanceFunctionAlgebraic::print_out(std::ostream& out,
									   State<STATE_INTERNAL_TYPE> s) const
{
	if (!has_importance_info()) {
		out << "\nImportance function \"" << name() << "\" doesn't yet have "
			   "any importance information to print." << std::endl;
		return;
	}
	auto print_state = [&out](const StateInstance& symbState) {
		out << "/";
		for (const auto& val: symbState)
			out << val << ",";
		out << "\b\\";
	};
	out << "\nPrinting importance function \"" << name() << "\" values.\n";
	out << "Legend: (/symbolic,state,valuation\\ importance_value)\n";
	out << "Values:" << std::endl;
	for (size_t i = 0ul ; i < s.concrete_size() ; i++) {
		const StateInstance symbState = s.decode(i).to_state_instance();
		out << "(";
		print_state(symbState);
		out << " " << importance_of(symbState) << ") ";
		out.flush();
	}
	out << "\b" << std::endl;
}


void
ImportanceFunctionAlgebraic::clear() noexcept
{
	adhocFun_.reset();
	strategy_ = "";
	hasImportanceInfo_ = false;
	std::vector< ImportanceValue >().swap(importance2threshold_);
	thresholdsTechnique_ = "";
	readyForSims_ = false;
	minImportance_ = static_cast<ImportanceValue>(0u);
	maxImportance_ = static_cast<ImportanceValue>(0u);
	minRareImportance_ = static_cast<ImportanceValue>(0u);
}

} // namespace fig
