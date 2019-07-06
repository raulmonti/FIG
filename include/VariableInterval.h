//==============================================================================
//
//  VariableInterval.h
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


#ifndef VARIABLEINTERVAL_H
#define VARIABLEINTERVAL_H

// C++
#include <string>
#include <tuple>
#include <type_traits>  // std::is_integral<>
// C
#include <cassert>
// FIG
#include <core_typedefs.h>
#include <Variable.h>
#include <FigException.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif


namespace fig
{

/**
 * @brief Variable implementation defined by the closed interval:
 *        [ min_value , max_value ]
 *
 *        This implementation can only handle discrete types,
 *        so the template parameter must be integral.
 *
 * @see VariableSet
 */
template< typename T_ >
class VariableInterval : public Variable< T_ >
{
	static_assert(std::is_integral<T_>::value,
				  "ERROR: class VariableInterval<T> can only be instantiated "
				  "with integral types, e.g. int, short, unsigned.");

public:  // Ctors/Dtor

	// Fresh variable (aka unnamed)
	VariableInterval() {}
	// Named variable
	VariableInterval(const std::string& thename, const T_& min, const T_& max);
	VariableInterval(const std::string& thename, const T_& min, const T_& max, const T_& val);
	VariableInterval(std::string&& thename, const T_& min, const T_& max);
	VariableInterval(std::string&& thename, const T_& min, const T_& max, const T_& val);
	VariableInterval(const VariableDeclaration< T_ >& dec);
	VariableInterval(const VariableDefinition< T_ >& def);
	VariableInterval(VariableDeclaration< T_ >&& dec);
	VariableInterval(VariableDefinition< T_ >&& def);

	// It should be safe to use compiler's both move and copy ctors
	VariableInterval(const VariableInterval& that) = default;
	VariableInterval(VariableInterval&& that) = default;

	/**
	 * @brief Copy assignment with copy&swap
	 * @note Only applicable to fresh variables, see Variable::operator=()
	 */
	VariableInterval<T_>& operator=(VariableInterval<T_> that);
	VariableInterval<T_>& operator=(VariableDeclaration<T_> dec);
	VariableInterval<T_>& operator=(VariableDefinition<T_> def);

	virtual ~VariableInterval() {}

public:  // Accessors

	inline T_ val() const noexcept override final
		{ return Variable<T_>::min_ + static_cast<T_>(Variable<T_>::offset_); }
	inline T_ val(const size_t& offset) const override final
		{ return Variable<T_>::min_ + static_cast<T_>(offset); }

public:  // Modifiers

	inline VariableInterval& operator=(const T_& value) override final
		{
#ifndef NDEBUG
			if (Variable<T_>::name_.empty())
				throw_FigException("can't assign value to a fresh variable (\""
								   + Variable<T_>::name_ + "\")");
#endif
			Variable<T_>::offset_ = value - Variable<T_>::min_;
			return *this;
		}

	void assign(const T_& value) override final;

public:  // Relational operators

	inline bool operator==(const VariableInterval<T_>& that) const
	    {
		    return Variable<T_>::name_ == that.name_ &&
			       Variable<T_>::min_ == that.min_ &&
			       Variable<T_>::max_ == that.max_ &&
			       Variable<T_>::ini_ == that.ini_ &&
			       Variable<T_>::offset_ == that.offset_;
	    }

	inline bool operator<=(const VariableInterval<T_>& that) const
	    {
		    return Variable<T_>::name_ == that.name_ &&
			       Variable<T_>::min_ == that.min_ &&
			       Variable<T_>::max_ == that.max_ &&
			       Variable<T_>::ini_ == that.ini_ &&
			       Variable<T_>::offset_ <= that.offset_;
	    }

	inline bool operator==(const Variable<T_>& that) const override
	    {
		    auto pthat = dynamic_cast<const VariableInterval<T_>*>(&that);
			return nullptr != pthat && (*this) == (*pthat);
	    }

	inline bool operator<=(const Variable<T_>& that) const override
	    {
		    auto pthat = dynamic_cast<const VariableInterval<T_>*>(&that);
			return nullptr != pthat && (*this) <= (*pthat);
	    }

	inline bool is_valid_value(const T_& val) const override final
		// http://stackoverflow.com/a/19954164
		{ return Variable<T_>::min_ <= val && val <= Variable<T_>::max_; }

// Same invariant as in base class
};

} // namesace fig

#endif // VARIABLEINTERVAL_H
