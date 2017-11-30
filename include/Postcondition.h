//==============================================================================
//
//  Postcondition.h
//
//  Copyleft 2015-
//  Authors:
//  - Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
//  - Leonardo Rodríguez
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


#ifndef POSTCONDITION_H
#define POSTCONDITION_H

// C++
#include <vector>
#include <iterator>     // std::distance(), std::begin()
#include <algorithm>    // std::copy() ranges
#include <type_traits>  // std::is_constructible<>
// FIG
#include <ExpStateUpdater.h>
#include <State.h>
#include <ModelAST.h>

// ADL
using std::copy;
using std::begin;
using std::end;
using std::shared_ptr;


namespace fig {

using AssignmentContainer = std::vector<shared_ptr<Assignment>>;

class Postcondition : public ExpStateUpdater {
private:

    static LocationContainer
    updateLocations(AssignmentContainer assignments) {
        LocationContainer locs;
        locs.resize(assignments.size());
        size_t i = 0;
        for (shared_ptr<Assignment>& assign : assignments) {
            locs[i] = assign->get_effect_location();
            i++;
        }
        return (locs);
    }

    static ExpContainer updateExps(AssignmentContainer assignments) {
        ExpContainer exps;
        exps.resize(assignments.size());
        size_t i = 0;
        for (shared_ptr<Assignment>& assign : assignments) {
            exps[i] = assign->get_rhs();
            i++;
        }
        return (exps);
    }

public:

    Postcondition(const std::vector<shared_ptr<Assignment>>& assignments) :
	    ExpStateUpdater(std::move(updateLocations(assignments)),
	                    std::move(updateExps(assignments)))
    {}

    /// Default copy ctor
    Postcondition(const Postcondition& that) = default;
    /// Default move ctor
    Postcondition(Postcondition&& that) = default;

public:

    void operator()(State<STATE_INTERNAL_TYPE>& state) const;
    void operator()(StateInstance& state) const;

public: //Debug
    void print_info(std::ostream& out) const;
};

} // namespace fig

#endif // POSTCONDITION_H
