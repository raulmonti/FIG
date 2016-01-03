//==============================================================================
//
//  StoppingConditions.cpp
//
//  Copyleft 2015-
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
#include <iterator>  // std::begin, std::end
#include <utility>   // std::swap
// FIG
#include <StoppingConditions.h>
#include <FigException.h>

// ADL
using std::begin;
using std::end;


namespace fig
{

template< template< typename... > class Container,
		  typename... OtherContainerArgs >
StoppingConditions::StoppingConditions(const Container< std::pair<double,double>,
														OtherContainerArgs...
													  >& confidenceCriteria)
{
	confidenceCriteria_.insert(begin(confidenceCriteria_),
							   begin(confidenceCriteria),
							   end(confidenceCriteria));
}

// Value StoppingConditions can only be created from the following containers
template StoppingConditions::StoppingConditions(const std::set<std::pair<double,double>>&);
template StoppingConditions::StoppingConditions(const std::list<std::pair<double,double>>&);
template StoppingConditions::StoppingConditions(const std::deque<std::pair<double,double>>&);
template StoppingConditions::StoppingConditions(const std::vector<std::pair<double,double>>&);
template StoppingConditions::StoppingConditions(const std::forward_list<std::pair<double,double>>&);
// template StoppingConditions::StoppingConditions(const std::unordered_set<std::pair<double,double>>&);


template< template< typename... > class Container,
		  typename... OtherContainerArgs >
StoppingConditions::StoppingConditions(const Container< unsigned long,
														OtherContainerArgs...
													  >& timeBudgets)
{
	timeBudgets_.insert(begin(timeBudgets_),
						begin(timeBudgets),
						end(timeBudgets));
}

// Time StoppingConditions can only be created from the following containers
template StoppingConditions::StoppingConditions(const std::set<unsigned long>&);
template StoppingConditions::StoppingConditions(const std::list<unsigned long>&);
template StoppingConditions::StoppingConditions(const std::deque<unsigned long>&);
template StoppingConditions::StoppingConditions(const std::vector<unsigned long>&);
template StoppingConditions::StoppingConditions(const std::forward_list<unsigned long>&);
template StoppingConditions::StoppingConditions(const std::unordered_set<unsigned long>&);


StoppingConditions& StoppingConditions::operator=(StoppingConditions that)
{
	if (that.is_value())
		std::swap(confidenceCriteria_, that.confidenceCriteria_);
	else
		std::swap(timeBudgets_, that.timeBudgets_);
	return *this;
}


void
StoppingConditions::add_confidence_criterion(const std::pair<double, double>& criterion)
{
	if (!timeBudgets_.empty())
#ifndef NDEBUG
		throw FigException("this was a time stopping condition, "
						   "can't mix that with confidence criteria");
#else
		return;
#endif
	confidenceCriteria_.push_back(criterion);
}


void
StoppingConditions::add_confidence_criterion(const double& confCo,
											 const double& prec)
{
	if (!timeBudgets_.empty())
#ifndef NDEBUG
		throw FigException("this was a time stopping condition, "
						   "can't mix that with confidence criteria");
#else
		return;
#endif
	confidenceCriteria_.emplace_back(confCo, prec);
}


void
StoppingConditions::add_time_budget(const unsigned long& seconds)
{
	if (!confidenceCriteria_.empty())
#ifndef NDEBUG
		throw FigException("this was a value stopping condition, "
						   "can't mix that with time budgets");
#else
		return;
#endif
	timeBudgets_.push_back(seconds);
}

} // namespace fig
