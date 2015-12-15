//==============================================================================
//
//  Traial.h
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


#ifndef TRAIAL_H
#define TRAIAL_H

// C++
#include <vector>
#include <memory>     // std::shared_ptr<>
#include <algorithm>  // std::swap()
#include <utility>    // std::move()
// FIG
#include <core_typedefs.h>
#include <State.h>
#include <Clock.h>


namespace fig
{

class ModuleInstance;

/**
 * @brief Simulation kernel (or 'trial trail')
 *
 *        Holds the state of the variables and the clocks values,
 *        i.e. all that is needed to run a simulation through the user's
 *        system model.
 *        Traials should be handled with the TraialPool, to ensure fast
 *        acquisition/release (instead of creation/destruction) of the
 *        instances.
 */
class Traial
{
	friend class Transition;

protected:

	/// Paraphernalia needed on clock expiration
	struct Timeout
	{
		/// Module where the expired clock exists
		std::shared_ptr<const ModuleInstance> module;
		/// Clock's name
		const std::string& name;
		/// Clock's time value
		float value;
		/// Data ctor
		Timeout(std::shared_ptr<const ModuleInstance> themodule,
				const std::string& thename,
				const float& thevalue) :
			module(themodule), name(thename), value(thevalue) {}
		// Other ctors
		Timeout(const Timeout& that)            = default;
		Timeout(Timeout&& that)                 = default;
		Timeout& operator=(const Timeout& that) = default;
		Timeout& operator=(Timeout&& that)      = default;
	};

public:  // Attributes

	/// \ref Variable "Variables" values instantiation
	/// (same order as in the system global state)
	StateInstance state;

protected:

	/// \ref Clock "Clocks" values instantiation
	/// (order given by each \ref ModuleInstance "module" internals,
	/// and in which order these were added to the network)
	std::vector< Timeout > clocks_;

private:

	/// Time-increasing-ordered view of 'clocks_' vector.
	/// Access for friends is safely granted through next_timeout()
	std::vector< unsigned > orderedIndex_;

	/// Position of smallest not-null Clock value in clocks_.
	/// Negative if all are null.
	int firstNotNull_;

public:  // Ctors/Dtor

	/**
	 * @brief Void ctor for resources pool
	 * @param stateSize Symbolic size of the global state,
	 *                  i.e. # of variables counting from all modules
	 * @param numClocks Number of clocks in the whole system (from all modules)
	 */
	Traial(const size_t& stateSize, const size_t& numClocks);

	/**
	 * @brief Data ctor
	 *
	 * @param stateSize     Symbolic size of the global state
	 * @param numClocks     Number of clocks in the whole system
	 * @param initClocks    Whether to initialize some clocks
	 * @param whichClocks   Which clocks to initialize if initClocks is true
	 * @param orderTimeouts Whether to order the timeouts after initializations
	 *
	 * @warning By default, and regardless of clocks initialization,
	 *          the timeouts won't be ordered. To force ordering call with
	 *          last parameter set to <b>true</b>.
	 */
	Traial(const size_t& stateSize,
		   const size_t& numClocks,
		   bool initClocks,
		   Bitflag whichClocks = static_cast<Bitflag>(0u),
		   bool orderTimeouts = false);

	/// Copy ctor
	inline Traial(const Traial& that) :
		state(that.state),
		clocks_(that.clocks_),
		orderedIndex_(that.orderedIndex_),
		firstNotNull_(that.firstNotNull_)
		{}

	/// Move ctor
	inline Traial(Traial&& that) :
		state(std::move(that.state)),
		clocks_(std::move(that.clocks_)),
		orderedIndex_(std::move(that.orderedIndex_)),
		firstNotNull_(std::move(that.firstNotNull_))
		{}

	/// Copy assignment with copy&swap idiom
	inline Traial& operator=(Traial that)
		{
			std::swap(state, that.state);
			std::swap(clocks_, that.clocks_);
			std::swap(orderedIndex_, that.orderedIndex_);
			std::swap(firstNotNull_, that.firstNotNull_);
			return *this;
		}

	~Traial();

//protected:  // Utils
public:  // Public only for testing

	/**
	 * @brief Retrieve next not-null expiring clock
	 * @param reorder  Whether to reorder internal clocks prior the retrieval
	 * @note  <b>Complexity:</b> <i>O(m log(m))</i> if reorder, <i>O(1)</i>
	 *        otherwise, where 'm' is the number of clocks in the system.
	 * @throw FigException if all our clocks have null value
	 */
	inline const Timeout&
	next_timeout(bool reorder = true)
		{
			if (reorder)
				reorder_clocks();
			if (0 > firstNotNull_)
				throw FigException("all clocks are null!");
			return clocks_[firstNotNull_];
		}

private:

	/**
	 * @brief Sort our clocks in increasing-value order for next_timeout()
	 * @note <b>Complexity:</b> <i>O(m log(m))</i>, where
	 *       m is the total number of clocks in the system
	 */
	void
	reorder_clocks();
};

} // namespace fig

#endif // TRAIAL_H
