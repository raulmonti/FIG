//==============================================================================
//
//  Label.h
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


#ifndef LABEL_H
#define LABEL_H

// C++
#include <string>
// FIG
#include <FigException.h>


namespace fig
{

/**
 * @brief Synchronization labels between modules
 *
 *        Labels are immutable strings which exist in an input/output dichotomy.
 *        Output labels represent active  transitions which press progress.
 *        Input  labels represent passive transitions which wait for an
 *        homonymous output forcing its progress.
 *
 * @note Would've inherited from std::string, but http://stackoverflow.com/a/6007040
 */
class Label
{
public:  // Attributes

	/// Label per se
	const std::string str;

	/// Create output labels by default
	static constexpr bool default_output = true;

private:

	/// Label type. Inputs are "non-outputs"
	bool output_;

public: // Ctors

	/// Taus (aka empty labels) are silent outputs
	Label() : output_(true) {}

	/**
	 * @brief Copy data ctor
	 * @note  Notice output labels can not be empty
	 * @throw FigException if str.empty() and isOutput
	 */
	Label(const std::string& str, bool isOutput = default_output) :
		str(str),
		output_(isOutput)
		{
			if (!isOutput && this->str.empty())
				throw FigException("can't construct an empty input label");
		}

	/**
	 * @brief Move data ctor
	 * @note  Notice output labels can not be empty
	 * @throw FigException if str.empty() and isOutput
	 */
	Label(std::string&& str, bool isOutput = default_output) :
		str(str),
		output_(isOutput)
		{
			if (!isOutput && this->str.empty())
				throw FigException("can't construct an empty input label");
		}

	/// Copy ctor
	Label(const Label& that) = default;
	/// Move ctor
	Label(Label&& that)      = default;

	/// Can't copy assign due to const string member
	Label& operator=(const Label&) = delete;
	/// Can't move assign due to const string member
	Label& operator=(Label&&)      = delete;

public:  // Relational operators

	/**
	 * @brief Tell whether this and that Labels match
	 * @note  Doesn't distinguish between input and output
	 * @see   same_as()
	 */
	inline bool operator==(const Label& that) const noexcept
		{ return str == that.str; }
	inline bool operator!=(const Label& that) const noexcept
		{ return !(*this == that); }

	/**
	 * @brief Tell whether this and that Labels are exactly equal
	 * @note  Distinguishes between input and output
	 * @see   operator==()
	 */
	inline bool same_as(const Label& that) const noexcept
		{ return output_ == that.output_ && str == that.str; }

public:  // Accessors
	inline bool is_tau()    const noexcept { return str.empty(); }
	inline bool is_input()  const noexcept { return !output_; }
	inline bool is_output() const noexcept { return  output_; }
};


} // namespace fig

#endif // LABEL_H
