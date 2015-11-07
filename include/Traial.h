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
// Project code
#include <State.h>


namespace fig
{

class Transition;

/**
 * @brief Simulation kernel (or 'trial trail')
 *
 *        Holds the state of the variables and the set clocks values,
 *        i.e. all that is needed to run through the user's model.
 */
class Traial
{
	friend class Transition;

public:  // Types and attributes

	/// Paraphernalia needed on clock expiration
	struct Timeout
	{
		/// Module where the expired clock exists
		std::shared_ptr< const SymbolicModule > module;
		/// Clock's name
		const std::string& name;
		/// Clock's time value
		float value;
	};

	/// Variables values instantiation (same order as in GlobalState 'gState')
	State state;

private:

	/// Clocks values instantiation (same order as in 'gClocks')
	std::vector< Timeout > clocks_;

	/// Time-increasing-ordered view of 'clocks_' vector
	std::vector< std::shared_ptr< const Timeout > > timeouts_;

public:  // Ctors

	/// TODO
 ///
 ///  Fill up
 ///
 ///  This has to be lightning fast, many will be created/destroyed
 ///

protected:  // Utils

	/**
	 * @brief Retrieve next not-null expiring clock
	 * @param reorder  Whether to reorder internal clocks prior the retrieval
	 * @note <b>Complexity:</b> <i>O(m log(m))</i> if reorder, <i>O(m)</i>
	 *       otherwise, where 'm' is the number of clocks in the system.
	 */
	const Timeout&
	next_timeout(bool reorder = true);

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
