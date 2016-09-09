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

private:
        /// Type of labels
        enum class LType {input, output, tau, committed};

        /// Label type.
        LType type_;

        /// Private constructor
        Label(const std::string &str, LType type)
            : str {str}, type_ {type} {}

public:
        static Label make_input(const std::string &str) {
            if (str.empty()) {
                throw_FigException("Cannot create an input "
                                   "label from an empty string");
            }
            return Label(str, LType::input);
        }

        static Label make_output(const std::string &str) {
            if (str.empty()) {
                throw_FigException("Cannot create an output "
                                   "label from an empty string");
            }
            return Label(str, LType::output);
        }

        static Label make_tau() {
            return Label(std::string(), LType::tau);
        }

        static Label make_commited(const std::string &str) {
            return Label(str, LType::committed);
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
         * @note  Doesn't distinguish between label types
	 * @see   same_as()
	 */
	inline bool operator==(const Label& that) const noexcept
		{ return str == that.str; }
	inline bool operator!=(const Label& that) const noexcept
		{ return !(*this == that); }

	/**
	 * @brief Tell whether this and that Labels are exactly equal
         * @note  Distinguishes between label types
	 * @see   operator==()
	 */
	inline bool same_as(const Label& that) const noexcept
                { return type_ == that.type_ && str == that.str; }

public:  // Accessors
        inline bool is_tau()    const noexcept {
            return (type_ == LType::tau);
        }
        inline bool is_input()  const noexcept {
            return (type_ == LType::input);
        }
        inline bool is_output() const noexcept {
            return (type_ == LType::output);
        }
        inline bool is_committed() const noexcept {
            return (type_ == LType::committed);
        }
};


} // namespace fig

#endif // LABEL_H
