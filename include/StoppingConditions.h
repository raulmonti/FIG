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

#include <vector>
#include <utility>
#include <iterator>     // std::begin, std::end

// ADL
using std::begin;
using std::end;


namespace fig
{

/**
 * @brief Criteria to stop simulations
 *
 *        There are two basic ways to tell a simulation has run long enough:
 *        either it has achieved the desired confidence criteria, or it has
 *        reached the wall time limit imposed by the user.
 *        The first we call "value simulations", since the goal is to estimate
 *        the property's value with a specified accuracy, regardless of how
 *        long that may take.
 *        The second we call "time simulations" for obvious reasons.
 *
 *        A single instance of the StoppingConditions class can hold several
 *        end-of-simulation criteria, but either of the "value" or the "time"
 *        kind, not a Mischung.
 */
class StoppingConditions
{
	/// List of confidence coefficients and precision values to experiment with
	std::vector< std::pair< double, double > > confidenceCriteria_;

	/// List of wall clock time values (in seconds) to experiment with
	std::vector< unsigned long > timeBudgets_;

public:  // Ctors/Dtor

	/// Empty ctor, still undecided if for value or time simulations
	/// @see StoppingConditions
	StoppingConditions() {}

	/**
	 * @brief Data ctor for confidence criteria pairs from lvalue container
	 *
	 *        This ctor builds the StoppingConditions for "value simulations".
	 *        The passed data is interpreted as std::pair<double,double>.
	 *        The first components are interpreted as confidence coefficients
	 *        (in the open range (0.0,1.0)), and the second components are
	 *        interpreted as the matching precisions.
	 *
	 * @param confidenceCriteria Container with the confidence criteria pairs
	 *
	 * @see StoppingConditions
	 */
	template< template< typename PAIR_, typename... > class Container,
			  typename PAIR_,
			  typename... OtherContainerArgs >
	StoppingConditions(const Container< std::pair<double,double>, OtherContainerArgs... >& confidenceCriteria);

	/**
	 * @brief Data ctor for confidence criteria pairs from iterator range
	 *
	 *        This ctor builds the StoppingConditions for "value simulations".
	 *        The passed data is interpreted as std::pair<double,double>.
	 *        The first components are interpreted as confidence coefficients
	 *        (in the open range (0.0,1.0)), and the second components are
	 *        interpreted as the matching precisions.
	 *
	 * @param from   Iterator to  the first confidence criterion pair
	 * @param to     Iterator past the last confidence criterion pair
	 *
	 * @see StoppingConditions
	 */
	template< template< typename PAIR_, typename... > class Iterator,
			  typename PAIR_,
			  typename... OtherIteratorArgs >
	StoppingConditions(Iterator<PAIR_, OtherIteratorArgs...> from,
					   Iterator<PAIR_, OtherIteratorArgs...> to);

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
	template< template< typename ULONG_, typename... > class Container,
			  typename ULONG_,
			  typename... OtherContainerArgs >
	StoppingConditions(const Container< unsigned long, OtherContainerArgs... >& timeBudgets);

//	/**
//	 * @brief Data ctor for time budgets from iterator range
//	 *
//	 *        This ctor builds the StoppingConditions for "time simulations".
//	 *        The passed data is interpreted as wall clock time values,
//	 *        in seconds, which will bound the simulations running time.
//	 *
//	 * @param from   Iterator to  the first time budget, in seconds
//	 * @param to     Iterator past the last time budget, in seconds
//	 *
//	 * @see StoppingConditions
//	 */
//	template< template< typename ULONG_, typename... > class Iterator,
//			  typename ULONG_,
//			  typename... OtherIteratorArgs >
//	StoppingConditions(Iterator<ULONG_, OtherIteratorArgs...> from,
//					   Iterator<ULONG_, OtherIteratorArgs...> to);

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
	 *                  open range (0.0,1.0)), and whose second component will
	 *                  be interpreted as the desired precision.
	 * @warning Only applicable if we hold \ref is_value() "value conditions"
	 *          or if the instance is still empty of any condition (empty ctor)
	 * \ifnot NDEBUG
	 *   @throw FigException if this instance is already holding
	 *                       \ref is_value() "time stopping conditions"
	 * \endif
	 */
	void add_confidence_criterion(const std::pair<double, double>& criterion);

	/**
	 * @brief Include one more confidence criterion to experiment with
	 * @param confCo Confidence coefficient (in the open range (0.0,1.0))
	 * @param prec   Precision
	 * @warning Only applicable if we hold \ref is_value() "value conditions"
	 *          or if the instance is still empty of any condition (empty ctor)
	 * \ifnot NDEBUG
	 *   @throw FigException if this instance is already holding
	 *                       \ref is_value() "time stopping conditions"
	 * \endif
	 */
	void add_confidence_criterion(const double& confCo, const double& prec);

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

	/// Are these stopping conditions for "value simulations"?
	/// @see StoppingConditions
	inline bool is_value() const noexcept { return !confidenceCriteria_.empty(); }

	/// Are these stopping conditions for "time simulations"?
	/// @see StoppingConditions
	inline bool is_time()  const noexcept { return !timeBudgets_.empty(); }

	/// @return Value stopping conditions, or "confidence criteria".
	///         Empty if this instance holds time budgets.
	inline const std::vector< std::pair<double,double> >&
	confidence_criteria() const noexcept { return confidenceCriteria_; }

	/// @return Time stopping conditions, or "time budgets".
	///         Empty if this instance holds confidence criteria.
	inline const std::vector< unsigned long >&
	time_budgets() const noexcept { return timeBudgets_; }
};

// // // // // // // // // // // // // // // // // // // // // // // // // // //

// Template definitions

// If curious about its presence here take a look at the end of VariableSet.cpp

template< template< typename PAIR_, typename... > class Iterator,
		  typename PAIR_,
		  typename... OtherIteratorArgs >
StoppingConditions::StoppingConditions(
	Iterator<PAIR_, OtherIteratorArgs...> from,
	Iterator<PAIR_, OtherIteratorArgs...> to)
{
	static_assert(std::is_constructible<std::pair<double,double>, PAIR_>::value,
				  "ERROR: type missmatch. StoppingConditions for confidence "
				  "criteria takes iterators pointing to std::pair<double,double>");
	confidenceCriteria_.insert(begin(confidenceCriteria_), from, to);
}

// template< template< typename ULONG_, typename... > class Iterator,
// 		  typename ULONG_,
// 		  typename... OtherIteratorArgs >
// StoppingConditions::StoppingConditions(
// 	Iterator<ULONG_, OtherIteratorArgs...> from,
// 	Iterator<ULONG_, OtherIteratorArgs...> to)
// {
// 	static_assert(std::is_constructible<std::pair<double,double>, PAIR_>::value,
// 				  "ERROR: type missmatch. StoppingConditions for time budgets "
// 				  "takes iterators pointing to unsigned long data");
// 	timeBudgets_.insert(begin(timeBudgets_), from, to);
// }

} // namespace fig

#endif // STOPPINGCONDITIONS_H
