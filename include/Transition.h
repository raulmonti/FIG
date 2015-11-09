//==============================================================================
//
//  Transition.h
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


#ifndef TRANSITION_H
#define TRANSITION_H

// C++
#include <string>
// C
#include <cassert>
// FIG
#include <core_typedefs.h>
#include <Label.h>
#include <Traial.h>
#include <Precondition.h>
#include <Postcondition.h>


namespace fig
{

/**
 * @brief IOSA module transition
 *
 *        A Transition consists of an input or output label,
 *        a precondition on variable values and maybe a clock enabling it, and
 *        a postcondition with variables updates and a set of clocks to reset
 *        when the transition is taken.
 *        For a formal definition visit http://dsg.famaf.unc.edu.ar.
 *
 * @note  This class assumes a global std::vector<Clock> named 'gClocks'
 *        was defined somewhere within the fig namespace.
 *        Such instance is needed for reseting the clock values
 *        with the appropiate distributions.
 */
class Transition
{
	/// Transition label, could also be tau (viz. empty)
	Label label_;

	/// Clock regulating transition applicability (empty for input transitions)
	std::string triggeringClock_;

	/// Precondition regulating transition applicability
	Precondition pre_;

	/// Updates to perform when transition is taken
	Postcondition pos_;

	/// Clocks to reset when transition is taken
	Bitflag resetClocks_;

public:  // Ctors

	/// TODO
 ///
 ///  Fill up
 ///

public:  // Accessors

	/// TODO
 ///
 ///  Fill up
 ///

	/// Clocks to reset when transition is taken
    inline const Bitflag& resetClocks() const noexcept { return resetClocks_; }

public:  // Utils

	/**
	 * @brief Reset and/or make time elapse in specified range of clocks
	 *
	 * @param traial      Traial whose clock values will be affected
	 * @param firstClock  First affected clock
	 * @param numClocks   Number of clocks to visit
	 * @param timeLapse   Amount of time elapsed for the non-reseting clocks
	 *
	 * @note <b>Complexity:</b> <i>O(numClocks)</i>
	 * @throw FigException if NTIMECHK was not defined and some clock
	 *        was assigned a negative value
	 */
	void handle_clocks(Traial&         traial,
					   const unsigned& firstClock,
					   const unsigned& numClocks,
					   const float&    timeLapse) const;

private:  // Utils

	/// Is the clock at position 'pos' marked for reset?
    inline bool must_reset(const unsigned& pos) const
        {
            assert(pos < 8*sizeof(Bitflag));  // check for overflow
            return static_cast<bool>(
                       resetClocks_ & ((static_cast<Bitflag>(1)) << pos));
        }
};

} // namespace fig

#endif // TRANSITION_H
