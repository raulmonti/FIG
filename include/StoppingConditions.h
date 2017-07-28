//==============================================================================
//
//  StoppingConditions.h
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

#ifndef STOPPINGCONDITIONS_H
#define STOPPINGCONDITIONS_H

#include <tuple>
#include <vector>
#include <utility>
#include <iterator>     // std::begin, std::end
#include <type_traits>  // std::is_same<>

// ADL
using std::begin;
using std::end;


namespace fig
{

/**
 * @brief Criteria to stop simulations
 *
 *        There are two basic ways to tell a simulation has run long enough:
 *        either it has achieved some desired confidence criterion, or it has
 *        reached the wall time limit imposed by the user.
 *        The first we call "value simulations" since the goal is to estimate
 *        the property's value with a specified accuracy, regardless of how
 *        long that may take.
 *        The second we call "time simulations" for obvious reasons.
 *
 *        A single instance of the StoppingConditions class can hold several
 *        end-of-simulation criteria, but either of the "value" or of the "time"
 *        kind, not a Mischung.
 */
class StoppingConditions
{
	/// List of confidence coefficients and precision values to experiment with
	std::vector< std::tuple< double, double, bool > > confidenceCriteria_;

	/// List of wall clock time values (in seconds) to experiment with
	std::vector< unsigned long > timeBudgets_;

public:  // Ctors/Dtor

	/// Empty ctor, still undecided if for value or time simulations
	/// @see StoppingConditions
	StoppingConditions() {}

	/**
	 * @brief Data ctor for confidence criteria tuples from lvalue container
	 *
	 *        This ctor builds the StoppingConditions for "value simulations".
	 *        The first component of each element in the container
	 *        is interpreted as a confidence coefficient (in the open range
	 *        (0.0,1.0)), the second as the matching precision, and the third
	 *        as whether the precision is expressed as a percentage of the
	 *        estimate.
	 *
	 * @param confidenceCriteria Container with the confidence criteria tuples
	 *
	 * @see StoppingConditions
	 */
	template< template< typename... > class Container,
			  typename... OtherContainerArgs >
	StoppingConditions(const Container< std::tuple<double,double,bool>,
										OtherContainerArgs...
									  >& confidenceCriteria);

	/**
	 * @brief Data ctor for time budgets from lvalue container
	 *
	 *        This ctor builds the StoppingConditions for "time simulations".
	 *        The passed data is interpreted as wall clock time values,
	 *        in seconds, which will bound the simulations running time.
	 *
	 * @param timeBudgets Container with the time budgets, in seconds
	 *
	 * @see StoppingConditions
	 */
	template< template< typename... > class Container,
			  typename... OtherContainerArgs >
	StoppingConditions(const Container< unsigned long,
										OtherContainerArgs...
									  >& timeBudgets);

	/**
	 * @brief Data ctor for confidence criteria tuples from iterator range
	 *
	 *        This ctor builds the StoppingConditions for "value simulations".
	 *        The first component of each element is interpreted as a confidence
	 *        coefficient (in the open range (0.0,1.0)), the second as the
	 *        matching precision, and the third as whether the precision is
	 *        expressed as a percentage of the estimate.
	 *
	 * @param from   Iterator to  the first confidence criterion tuple
	 * @param to     Iterator past the last confidence criterion tuple
	 *
	 * @see StoppingConditions
	 */
	template< template< typename... > class Iterator,
			  typename... OtherIteratorArgs >
	StoppingConditions(Iterator<std::tuple<double,double,bool>,
								OtherIteratorArgs...> from,
					   Iterator<std::tuple<double,double,bool>,
								OtherIteratorArgs...> to);

	/**
	 * @brief Data ctor for time budgets from iterator range
	 *
	 *        This ctor builds the StoppingConditions for "time simulations".
	 *        The passed data is interpreted as wall clock time values,
	 *        in seconds, which will bound the simulations running time.
	 *
	 * @param from   Iterator to  the first time budget, in seconds
	 * @param to     Iterator past the last time budget, in seconds
	 *
	 * @see StoppingConditions
	 */
	template< template< typename... > class Iterator,
			  typename... OtherIteratorArgs >
	StoppingConditions(Iterator<unsigned long,
								OtherIteratorArgs...> from,
					   Iterator<unsigned long,
								OtherIteratorArgs...> to);

	/// Default copy ctor
	StoppingConditions(const StoppingConditions& that) = default;

	/// Default move ctor
	StoppingConditions(StoppingConditions&& that) = default;

	/// Copy assignment with copy&swap
	StoppingConditions& operator=(StoppingConditions that);

	/// Default dtor
	~StoppingConditions() = default;

public:  // Populating facilities

	/**
	 * @brief Include one more confidence criterion to experiment with
	 * @param criterion New confidence criterion to add, whose first component
	 *                  will be interpreted as a confidence coefficient (in the
	 *                  open range (0.0,1.0)), second component as the desired
	 *                  precision, and third component as whether the precision
	 *                  is actually expressed as a percentage of the estimate.
	 * @warning Only applicable if we hold \ref is_value() "value conditions"
	 *          or if the instance is still empty of any condition (empty ctor)
	 * \ifnot NDEBUG
	 *   @throw FigException if this instance is already holding
	 *                       \ref is_value() "time stopping conditions"
	 * \endif
	 */
	void add_confidence_criterion(const std::tuple<double,double,bool>& criterion);

	/**
	 * @brief Include one more confidence criterion to experiment with
	 * @param confCo  Confidence coefficient (in the open range (0.0,1.0))
	 * @param prec    Precision
	 * @param dynPrec Is the precision expressed as a percentage of the estimate?
	 * @warning Only applicable if we hold \ref is_value() "value conditions"
	 *          or if the instance is still empty of any condition (empty ctor)
	 * \ifnot NDEBUG
	 *   @throw FigException if this instance is already holding
	 *                       \ref is_value() "time stopping conditions"
	 * \endif
	 */
	void add_confidence_criterion(const double& confCo,
								  const double& prec,
	                              const bool& dynPrec = true);

	/**
	 * @brief Include one wall time limit more to experiment with
	 * @param seconds New wall time limit to add (in, you guessed, seconds!)
	 * @warning Only applicable if we hold \ref is_time() "time conditions"
	 *          or if the instance is still empty of any condition (empty ctor)
	 * \ifnot NDEBUG
	 *   @throw FigException if this instance is already holding
	 *                       \ref is_value() "value stopping conditions"
	 * \endif
	 */
	void add_time_budget(const unsigned long& seconds);

public:  // Utils

	/// Number of conditions
	inline size_t size() const noexcept { return confidenceCriteria_.size() + timeBudgets_.size(); }

	/// Are these stopping conditions for "value simulations"?
	/// @see StoppingConditions
	inline bool is_value() const noexcept { return !confidenceCriteria_.empty(); }

	/// Alias for is_value()
	inline bool is_confidence_criteria() const noexcept { return is_value(); }

	/// Are these stopping conditions for "time simulations"?
	/// @see StoppingConditions
	inline bool is_time()  const noexcept { return !timeBudgets_.empty(); }

	/// Alias for is_time()
	inline bool is_time_budgets() const noexcept { return is_time(); }

	/// @return Value stopping conditions, or "confidence criteria".
	///         Empty if this instance holds time budgets.
	inline const std::vector< std::tuple<double,double,bool> >&
	confidence_criteria() const noexcept { return confidenceCriteria_; }

	/// @return Time stopping conditions, or "time budgets".
	///         Empty if this instance holds confidence criteria.
	inline const std::vector< unsigned long >&
	time_budgets() const noexcept { return timeBudgets_; }
};

// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< template< typename... > class Iterator,
		  typename... OtherIteratorArgs >
StoppingConditions::StoppingConditions(
	Iterator<std::tuple<double,double,bool>, OtherIteratorArgs...> from,
	Iterator<std::tuple<double,double,bool>, OtherIteratorArgs...> to)
{
	confidenceCriteria_.insert(begin(confidenceCriteria_), from, to);
}


template< template< typename... > class Iterator,
		  typename... OtherIteratorArgs >
StoppingConditions::StoppingConditions(
	Iterator<unsigned long, OtherIteratorArgs...> from,
	Iterator<unsigned long, OtherIteratorArgs...> to)
{
	timeBudgets_.insert(begin(timeBudgets_), from, to);
}

} // namespace fig

#endif // STOPPINGCONDITIONS_H
