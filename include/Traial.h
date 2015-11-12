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
// FIG
#include <State.h>
#include <Clock.h>


namespace fig
{

extern GlobalState< STATE_INTERNAL_TYPE > gState;
extern std::vector< Clock >               gClocks;

// class Transition;  // Wird diese Deklaration nötig?
class ModuleInstance;

/**
 * @brief Simulation kernel (or 'trial trail')
 *
 *        Holds the state of the variables and the clocks values,
 *        i.e. all that is needed to run a simulation through the user's
 *        system model.
 *        Traials should be administered through a
 *        <a href="https://sourcemaking.com/design_patterns/object_pool">
 *        resources pool</a>, to offer very fast creation/release of the
 *        instances.
 *
 * @note  This class assumes a GlobalState variable named 'gState'
 *        was defined somewhere within the fig namespace.
 *        Such instance may be needed for initializations on creation.
 *
 * @note  This class assumes a global std::vector<Clock> named 'gClocks'
 *        was defined somewhere within the fig namespace.
 *        Such instance may be needed for initializations on creation.
 */
class Traial
{
	friend class Transition;

public:  // Types and attributes

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
	};

	/// Variables values instantiation (same order as in GlobalState 'gState')
	State state;

protected:

	/// Clocks values instantiation (same order as in 'gClocks')
	std::vector< Timeout > clocks_;

private:

	/// Time-increasing-ordered view of 'clocks_' vector.
	/// Access for friends is safely granted through next_timeout()
	std::vector< const Timeout* > timeouts_;  // yeah baby, deep down we like it raw

	/// Pointer to first not-null \ref Clock "clock" in timeouts_.
	/// Negative if all are null.
	int firstNotNull_;

public:  // Ctors/Dtor

	/// Void ctor for resources pool
	Traial();

	/**
	 * @brief Data ctor
	 *
	 * @param initState   Whether to initialize our state with gState info
	 * @param initClocks  Whether to initialize some clocks
	 * @param whichClocks Which clocks to initialize if initClocks is true
	 */
	Traial(bool initState = false,
		   bool initClocks = false,
		   Bitflag whichClocks = static_cast<Bitflag>(0u));

	/// @todo: TODO define all ctors
 ///
 ///  This has to be lightning fast, many will be created/destroyed.
 ///  Suggestion: use resource pool design, see class doxygen.
 ///

	~Traial() { timeouts_.clear(); clocks_.clear(); }

protected:  // Utils

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
			return *timeouts_[firstNotNull_];
		}

private:

	/**
	 * @brief Sort our clocks in increasing-value order for next_timeout()
	 * @note <b>Complexity:</b> <i>O(m log(m))</i>, where
	 *       m is the length of the global vector 'gClocks'
	 */
	void
	reorder_clocks();
};

} // namespace fig

#endif // TRAIAL_H
