//==============================================================================
//
//  Property.h
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


#ifndef PROPERTY_H
#define PROPERTY_H

// C++
#include <string>
#include <atomic> // keeping instance_id thread safe.

// FIG
#include <core_typedefs.h>
#include <Precondition.h>
#include <State.h>

using std::shared_ptr;

namespace fig
{

/**
 * @brief Abstract base logical property
 *
 *        Properties are what the user wants to study about the system model
 *        provided. There are different kinds of them but in general they
 *        speak about the propability of certain events chain taking place.
 *        FIG estimates these values through efficient simulation.
 *
 * @see PropertyType
 */
class Property
{
    
protected:

    /// Used to generate a unique id for each instance.
    static std::atomic<int> current_id;
    /// Instance id.
    const int instance_id;

public:  // Attributes

//	/// Property per se in string form
//	const std::string expression;

    /// Which type of property the expression represents
    const PropertyType type;

public:  // Ctors

    Property(PropertyType thetype) :
	    instance_id(++current_id),
	    type(thetype)
	{}

    // Copy/Move constructor deleted to avoid dealing with the unique id.
    Property(const Property& that) = delete;
    Property(Property&& that)      = delete;

    /// Can't have empty ctor due to const data members
    Property()                                = delete;
    /// Can't copy assign due to const data members
    Property& operator=(const Property& that) = delete;
    /// Can't move assign due to const data members
    Property& operator=(Property&& that)      = delete;

public:  // Utils

	/// @brief Is this state considered "rare" for importance simulation?
	virtual bool is_rare(const StateInstance&) const = 0;

	/// @copydoc is_rare()
	virtual bool is_rare(const State<STATE_INTERNAL_TYPE>&) const = 0;

	/// @brief Should simulations be truncated when reaching this state?
	/// @note Only relevant for transient-like properties,
	///       i.e. types TRANSIENT and BOUNDED_REACHABILITY
	virtual inline bool is_stop(const StateInstance&) const { return false; }

	/// @copydoc is_stop()
	virtual inline bool is_stop(const State<STATE_INTERNAL_TYPE>&) const { return false; }

	/// Is the property satisfied by the given state?
	inline bool operator()(const StateInstance& s) const
	    { return !is_stop(s) && is_rare(s); }

	/// @copydoc operator ()()
	inline bool operator()(const State<STATE_INTERNAL_TYPE>& s) const
	    { return !is_stop(s) && is_rare(s); }

    /// Get instance unique id
    int get_id() const noexcept { return instance_id; }

    /// String representation of property
    virtual std::string to_string() const = 0;

    virtual void prepare(const State<STATE_INTERNAL_TYPE>& state) = 0;
    virtual void prepare(const PositionsMap& posMap) = 0;


public: // Debug
    virtual void print_info(std::ostream &out) const = 0;
};

} // namespace fig

#endif // PROPERTY_H

