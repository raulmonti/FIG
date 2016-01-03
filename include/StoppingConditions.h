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

	/// Template ctor from generic container, partially specialized below
	template< template< typename, typename... > class Container,
			  typename ValueType,
			  typename... OtherContainerArgs >
	StoppingConditions(const Container<ValueType, OtherContainerArgs...>&);

	/**
	 * @brief Data ctor for confidence criteria pairs from lvalue container
	 *
	 *        This ctor builds the StoppingConditions for "value simulations".
	 *        The first component of each element in the container
	 *        is interpreted as confidence coefficients (in the open range
	 *        (0.0,1.0)), and the second component as the matching precision.
	 *
	 * @param confidenceCriteria Container with the confidence criteria pairs
	 *
	 * @see StoppingConditions
	 */
	template< template< typename... > class Container,
			  typename... OtherContainerArgs >
	StoppingConditions(const Container< std::pair<double,double>,
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

	/// Template ctor from generic iterators, partially specialized below
	template< template< typename, typename... > class Iterator,
			  typename ValueType,
			  typename... OtherIteratorArgs >
	StoppingConditions(Iterator<ValueType, OtherIteratorArgs...>,
					   Iterator<ValueType, OtherIteratorArgs...>);

	/**
	 * @brief Data ctor for confidence criteria pairs from iterator range
	 *
	 *        This ctor builds the StoppingConditions for "value simulations".
	 *        The first component of each element is interpreted as confidence
	 *        coefficients (in the open range (0.0,1.0)), and the second
	 *        component as the matching precision.
	 *
	 * @param from   Iterator to  the first confidence criterion pair
	 * @param to     Iterator past the last confidence criterion pair
	 *
	 * @see StoppingConditions
	 */
	template< template< typename... > class Iterator,
			  typename... OtherIteratorArgs >
	StoppingConditions(Iterator<std::pair<double,double>,
								OtherIteratorArgs...> from,
					   Iterator<std::pair<double,double>,
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

	/// Alias for is_value()
	inline bool is_confidence_criteria() const noexcept { return is_value(); }

	/// Are these stopping conditions for "time simulations"?
	/// @see StoppingConditions
	inline bool is_time()  const noexcept { return !timeBudgets_.empty(); }

	/// Alias for is_time()
	inline bool is_time_budgets() const noexcept { return is_time(); }

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

// Here to easily cope with partial template specialization

template< template< typename, typename... > class Container,
		  typename ValueType,
		  typename... OtherContainerArgs >
StoppingConditions::StoppingConditions(const Container< ValueType,
														OtherContainerArgs...
													  >&)
{
	static_assert(std::is_same<ValueType, unsigned long>::value ||
				  std::is_same<ValueType, std::pair<double, double>>::value,
				  "ERROR: type missmatch. StoppingConditions can only be "
				  "built from containers with either unsigned long or "
				  "std::pair<double,double> elements.");
}


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


template< template< typename, typename... > class Iterator,
		  typename ValueType,
		  typename... OtherIteratorArgs >
StoppingConditions::StoppingConditions(Iterator<ValueType,OtherIteratorArgs...>,
									   Iterator<ValueType,OtherIteratorArgs...>)
{
	static_assert(std::is_same<ValueType, unsigned long>::value ||
				  std::is_same<ValueType, std::pair<double, double>>::value,
				  "ERROR: type missmatch. StoppingConditions can only be "
				  "built from iterators pointing to either unsigned long or "
				  "std::pair<double,double> elements.");
}


template< template< typename... > class Iterator,
		  typename... OtherIteratorArgs >
StoppingConditions::StoppingConditions(
	Iterator<std::pair<double,double>, OtherIteratorArgs...> from,
	Iterator<std::pair<double,double>, OtherIteratorArgs...> to)
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
